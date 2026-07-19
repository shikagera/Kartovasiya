// Bake procedural KTV app icon PNGs into Art/Icons
// Chaos Spellweaver (Картовасия)

#if UNITY_EDITOR
using System.IO;
using UnityEditor;
using UnityEngine;
using Kartovasiya.UI;

namespace Kartovasiya.EditorTools
{
    public static class AppIconGenerator
    {
        private const string IconsDir = "Assets/Kartovasiya/Art/Icons";

        [MenuItem("Kartovasiya/Generate KTV App Icon", priority = 15)]
        public static void GenerateAndAssign()
        {
            Directory.CreateDirectory(Path.Combine(Application.dataPath, "Kartovasiya/Art/Icons"));

            // Master 1024
            WriteIcon(1024, "AppIcon_1024.png");
            WriteIcon(512, "AppIcon_512.png");
            WriteIcon(192, "AppIcon_192.png");
            WriteIcon(144, "AppIcon_144.png");
            WriteIcon(96, "AppIcon_96.png");
            WriteIcon(72, "AppIcon_72.png");
            WriteIcon(48, "AppIcon_48.png");

            // Main AppIcon.png = 512 (Unity default friendly)
            string masterPath = Path.Combine(Application.dataPath, "Kartovasiya/Art/Icons/AppIcon.png");
            var master = ProceduralBrandArt.BuildKtvIconTexture(512, opaqueBackground: true);
            File.WriteAllBytes(masterPath, master.EncodeToPNG());
            Object.DestroyImmediate(master);

            AssetDatabase.Refresh();

            // Import settings
            ConfigureImporter($"{IconsDir}/AppIcon.png");
            ConfigureImporter($"{IconsDir}/AppIcon_1024.png");

            // Assign to Player Settings
            IconAndKeystoreSetup.SetupIcons();

            Debug.Log("[Kartovasiya] KTV app icons generated → Art/Icons and assigned in Player Settings.");
        }

        private static void WriteIcon(int size, string fileName)
        {
            var tex = ProceduralBrandArt.BuildKtvIconTexture(size, opaqueBackground: true);
            string abs = Path.Combine(Application.dataPath, "Kartovasiya/Art/Icons", fileName);
            File.WriteAllBytes(abs, tex.EncodeToPNG());
            Object.DestroyImmediate(tex);
        }

        private static void ConfigureImporter(string assetPath)
        {
            var importer = AssetImporter.GetAtPath(assetPath) as TextureImporter;
            if (importer == null) return;
            importer.textureType = TextureImporterType.Default;
            importer.alphaIsTransparency = true;
            importer.mipmapEnabled = false;
            importer.npotScale = TextureImporterNPOTScale.None;
            importer.maxTextureSize = 1024;
            importer.SaveAndReimport();
        }
    }
}
#endif
