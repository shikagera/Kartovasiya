// Editor: assign app icons + wire dev keystore paths
// Редактор: иконки рабочего стола Android + dev keystore
// Chaos Spellweaver (Картовасия)

#if UNITY_EDITOR
using System.IO;
using System.Linq;
using UnityEditor;
using UnityEditor.Build;
using UnityEngine;

namespace Kartovasiya.EditorTools
{
    public static class IconAndKeystoreSetup
    {
        private const string IconPath = "Assets/Kartovasiya/Art/Icons/AppIcon.png";
        private const string Icon1024Path = "Assets/Kartovasiya/Art/Icons/AppIcon_1024.png";
        private const string KeystoreRelative = "Keys/kartovasiya-dev.keystore";
        private const string KeyAlias = "kartovasiya";
        private const string KeyPass = "kartovasiya_dev";

        [MenuItem("Kartovasiya/Setup Icons & Dev Keystore", priority = 20)]
        public static void SetupAll()
        {
            SetupIcons();
            SetupKeystore();
            AssetDatabase.SaveAssets();
            Debug.Log("[Kartovasiya] Icons + keystore PlayerSettings applied. Check Player Settings → Android → Icons.");
        }

        [MenuItem("Kartovasiya/Setup App Icons Only", priority = 21)]
        public static void SetupIcons()
        {
            ConfigureIconImporters();

            var tex = LoadBestIcon();
            if (tex == null)
            {
                Debug.LogError(
                    $"[Kartovasiya] Icon not found at {IconPath}.\n" +
                    "Run Tools/GenerateKtvIcons.ps1 or Kartovasiya → Generate KTV App Icon.");
                return;
            }

            // Default / legacy single-slot API
            var simple = new[] { tex };
            PlayerSettings.SetIcons(NamedBuildTarget.Android, simple, IconKind.Application);
            try
            {
                PlayerSettings.SetIcons(NamedBuildTarget.Standalone, simple, IconKind.Application);
            }
            catch (System.Exception e)
            {
                Debug.LogWarning("[Kartovasiya] Standalone icon skip: " + e.Message);
            }

            // Full Android platform icons (Legacy + Round + Adaptive) — what shows on home screen
            AssignAllAndroidPlatformIcons(tex);

            EditorUtility.SetDirty(tex);
            AssetDatabase.SaveAssets();
            Debug.Log($"[Kartovasiya] App icon assigned for Android desktop: {AssetDatabase.GetAssetPath(tex)}");
        }

        private static Texture2D LoadBestIcon()
        {
            var tex = AssetDatabase.LoadAssetAtPath<Texture2D>(IconPath);
            if (tex == null)
                tex = AssetDatabase.LoadAssetAtPath<Texture2D>(Icon1024Path);
            if (tex == null)
                tex = AssetDatabase.LoadAssetAtPath<Texture2D>("Assets/Kartovasiya/Art/Icons/AppIcon_512.png");
            return tex;
        }

        private static void ConfigureIconImporters()
        {
            string[] paths =
            {
                IconPath,
                Icon1024Path,
                "Assets/Kartovasiya/Art/Icons/AppIcon_512.png",
                "Assets/Kartovasiya/Art/Icons/AppIcon_192.png",
                "Assets/Kartovasiya/Art/Icons/AppIcon_144.png",
                "Assets/Kartovasiya/Art/Icons/AppIcon_96.png",
                "Assets/Kartovasiya/Art/Icons/AppIcon_72.png",
                "Assets/Kartovasiya/Art/Icons/AppIcon_48.png"
            };

            foreach (var path in paths)
            {
                if (!File.Exists(Path.Combine(Application.dataPath, "..", path)))
                {
                    // relative from Assets
                }

                var importer = AssetImporter.GetAtPath(path) as TextureImporter;
                if (importer == null) continue;

                bool dirty = false;
                if (importer.textureType != TextureImporterType.Default)
                {
                    importer.textureType = TextureImporterType.Default;
                    dirty = true;
                }

                if (importer.mipmapEnabled)
                {
                    importer.mipmapEnabled = false;
                    dirty = true;
                }

                if (!importer.alphaIsTransparency)
                {
                    importer.alphaIsTransparency = true;
                    dirty = true;
                }

                if (importer.npotScale != TextureImporterNPOTScale.None)
                {
                    importer.npotScale = TextureImporterNPOTScale.None;
                    dirty = true;
                }

                if (importer.maxTextureSize < 1024)
                {
                    importer.maxTextureSize = 1024;
                    dirty = true;
                }

                // Readable not required for icons; keep off for mobile
                if (dirty)
                    importer.SaveAndReimport();
            }
        }

        private static void AssignAllAndroidPlatformIcons(Texture2D tex)
        {
            var kinds = PlayerSettings.GetSupportedIconKinds(NamedBuildTarget.Android);
            if (kinds == null || kinds.Length == 0)
            {
                Debug.LogWarning("[Kartovasiya] No Android icon kinds reported — using SetIcons only.");
                return;
            }

            foreach (var kind in kinds)
            {
                var icons = PlayerSettings.GetPlatformIcons(NamedBuildTarget.Android, kind);
                if (icons == null || icons.Length == 0)
                    continue;

                for (int i = 0; i < icons.Length; i++)
                {
                    // Adaptive: background + foreground. Same art is fine for prototype.
                    // Other kinds: single texture.
                    int layerCount = icons[i].maxLayerCount;
                    if (layerCount <= 1)
                    {
                        icons[i].SetTexture(tex);
                    }
                    else
                    {
                        // Foreground + background layers
                        var layers = new Texture2D[layerCount];
                        for (int L = 0; L < layerCount; L++)
                            layers[L] = tex;
                        icons[i].SetTextures(layers);
                    }
                }

                PlayerSettings.SetPlatformIcons(NamedBuildTarget.Android, kind, icons);
                Debug.Log($"[Kartovasiya] Android icons set for kind '{kind}' ({icons.Length} sizes).");
            }
        }

        [MenuItem("Kartovasiya/Setup Dev Keystore Only", priority = 22)]
        public static void SetupKeystore()
        {
            string projectRoot = Path.GetDirectoryName(Application.dataPath);
            string keystoreAbs = Path.Combine(projectRoot ?? "", KeystoreRelative).Replace('\\', '/');

            if (!File.Exists(keystoreAbs))
            {
                Debug.LogWarning(
                    "[Kartovasiya] Dev keystore missing.\n" +
                    "Run PowerShell from project root:\n" +
                    "  .\\Tools\\CreateDevKeystore.ps1\n" +
                    "Requires Android OpenJDK / JAVA_HOME.");
            }

            PlayerSettings.Android.useCustomKeystore = true;
            PlayerSettings.Android.keystoreName = keystoreAbs;
            PlayerSettings.Android.keyaliasName = KeyAlias;
            PlayerSettings.Android.keystorePass = KeyPass;
            PlayerSettings.Android.keyaliasPass = KeyPass;

            Debug.Log(
                $"[Kartovasiya] Keystore settings:\n" +
                $"  useCustomKeystore=true\n" +
                $"  path={keystoreAbs}\n" +
                $"  alias={KeyAlias}\n" +
                $"  password={(File.Exists(keystoreAbs) ? "set (dev)" : "set, but FILE MISSING")}");
        }

        [MenuItem("Kartovasiya/Open Icons Folder", priority = 23)]
        public static void OpenIconsFolder()
        {
            var obj = AssetDatabase.LoadAssetAtPath<Object>("Assets/Kartovasiya/Art/Icons");
            if (obj != null)
            {
                EditorGUIUtility.PingObject(obj);
                Selection.activeObject = obj;
            }
        }
    }
}
#endif
