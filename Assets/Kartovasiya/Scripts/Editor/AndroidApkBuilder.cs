// Batch / menu Android APK build → projectRoot/apk/
// Chaos Spellweaver (Картовасия)

#if UNITY_EDITOR
using System;
using System.IO;
using System.Linq;
using UnityEditor;
using UnityEditor.Build.Reporting;
using UnityEngine;

namespace Kartovasiya.EditorTools
{
    public static class AndroidApkBuilder
    {
        private const string DefaultApkName = "ChaosSpellweaver.apk";

        /// <summary>
        /// CLI: Unity -quit -batchmode -projectPath ... -executeMethod Kartovasiya.EditorTools.AndroidApkBuilder.BuildApk
        /// Optional: -apkPath "C:\full\path\out.apk"
        /// </summary>
        public static void BuildApk()
        {
            try
            {
                // Ensure publishing settings for installable APK
                IconAndKeystoreSetup.SetupKeystore();

                string projectRoot = Path.GetDirectoryName(Application.dataPath) ?? ".";
                string apkDir = Path.Combine(projectRoot, "apk");
                Directory.CreateDirectory(apkDir);

                string apkPath = ResolveApkPath(apkDir);
                string[] scenes = EditorBuildSettings.scenes
                    .Where(s => s.enabled && !string.IsNullOrEmpty(s.path))
                    .Select(s => s.path)
                    .ToArray();

                if (scenes.Length == 0)
                {
                    Fail("No scenes enabled in File → Build Settings / EditorBuildSettings.");
                    return;
                }

                // APK (not AAB) so the file can be sideloaded without Play
                EditorUserBuildSettings.buildAppBundle = false;
                EditorUserBuildSettings.androidBuildSystem = AndroidBuildSystem.Gradle;

                var options = new BuildPlayerOptions
                {
                    scenes = scenes,
                    locationPathName = apkPath,
                    target = BuildTarget.Android,
                    options = BuildOptions.None
                };

                Debug.Log($"[Kartovasiya] Building Android APK → {apkPath}");
                Debug.Log($"[Kartovasiya] Scenes: {string.Join(", ", scenes)}");

                BuildReport report = BuildPipeline.BuildPlayer(options);
                BuildSummary summary = report.summary;

                if (summary.result == BuildResult.Succeeded)
                {
                    long sizeMb = summary.totalSize > 0
                        ? (long)(summary.totalSize / (1024.0 * 1024.0))
                        : (File.Exists(apkPath) ? new FileInfo(apkPath).Length / (1024 * 1024) : 0);

                    Debug.Log(
                        $"[Kartovasiya] APK build OK in {summary.totalTime.TotalSeconds:F1}s, ~{sizeMb} MB\n" +
                        $"  {apkPath}");

                    if (Application.isBatchMode)
                        EditorApplication.Exit(0);
                }
                else
                {
                    Fail($"Build result: {summary.result}. Errors: {summary.totalErrors}. See Editor.log.");
                }
            }
            catch (Exception e)
            {
                Fail(e.ToString());
            }
        }

        [MenuItem("Kartovasiya/Build APK to apk/", priority = 10)]
        public static void BuildApkFromMenu()
        {
            BuildApk();
        }

        private static string ResolveApkPath(string apkDir)
        {
            string[] args = Environment.GetCommandLineArgs();
            for (int i = 0; i < args.Length - 1; i++)
            {
                if (string.Equals(args[i], "-apkPath", StringComparison.OrdinalIgnoreCase))
                    return Path.GetFullPath(args[i + 1]);
            }

            return Path.GetFullPath(Path.Combine(apkDir, DefaultApkName));
        }

        private static void Fail(string message)
        {
            Debug.LogError("[Kartovasiya] APK build failed: " + message);
            if (Application.isBatchMode)
                EditorApplication.Exit(1);
        }
    }
}
#endif
