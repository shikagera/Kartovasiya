// Post-process generated Gradle project to align Kotlin stdlib versions.
// Fixes: :launcher:checkReleaseDuplicateClasses
//   Duplicate class ... found in kotlin-stdlib-1.8.22 and kotlin-stdlib-jdk7/jdk8-1.6.21
// Chaos Spellweaver (Картовасия)

#if UNITY_EDITOR
using System.IO;
using UnityEditor.Android;
using UnityEngine;

namespace Kartovasiya.EditorTools
{
    public sealed class AndroidKotlinDuplicateFix : IPostGenerateGradleAndroidProject
    {
        public int callbackOrder => 1000;

        private const string Marker = "// KARTOVASIYA_KOTLIN_ALIGN";

        private const string ResolutionBlock = @"
// KARTOVASIYA_KOTLIN_ALIGN
// Align Kotlin stdlib — avoid duplicate classes between kotlin-stdlib 1.8+ and jdk7/jdk8 1.6.x
configurations.configureEach {
    resolutionStrategy {
        force 'org.jetbrains.kotlin:kotlin-stdlib:1.8.22'
        force 'org.jetbrains.kotlin:kotlin-stdlib-jdk7:1.8.22'
        force 'org.jetbrains.kotlin:kotlin-stdlib-jdk8:1.8.22'
        force 'org.jetbrains.kotlin:kotlin-stdlib-common:1.8.22'
    }
}
";

        public void OnPostGenerateGradleAndroidProject(string path)
        {
            // path = .../Gradle/unityLibrary
            string root = Directory.GetParent(path)?.FullName;
            if (string.IsNullOrEmpty(root))
            {
                Debug.LogWarning("[Kartovasiya] Kotlin fix: could not resolve Gradle root from " + path);
                return;
            }

            PatchModule(Path.Combine(root, "unityLibrary", "build.gradle"));
            PatchModule(Path.Combine(root, "launcher", "build.gradle"));
            PatchRoot(Path.Combine(root, "build.gradle"));

            Debug.Log("[Kartovasiya] Kotlin stdlib alignment applied under: " + root);
        }

        private static void PatchModule(string gradleFile)
        {
            if (!File.Exists(gradleFile))
                return;

            string text = File.ReadAllText(gradleFile);
            if (text.Contains(Marker))
                return;

            File.WriteAllText(gradleFile, text.TrimEnd() + "\n" + ResolutionBlock);
        }

        private static void PatchRoot(string gradleFile)
        {
            if (!File.Exists(gradleFile))
                return;

            string text = File.ReadAllText(gradleFile);
            if (text.Contains(Marker))
                return;

            const string rootBlock = @"
// KARTOVASIYA_KOTLIN_ALIGN
subprojects { subproject ->
    subproject.configurations.configureEach {
        resolutionStrategy {
            force 'org.jetbrains.kotlin:kotlin-stdlib:1.8.22'
            force 'org.jetbrains.kotlin:kotlin-stdlib-jdk7:1.8.22'
            force 'org.jetbrains.kotlin:kotlin-stdlib-jdk8:1.8.22'
            force 'org.jetbrains.kotlin:kotlin-stdlib-common:1.8.22'
        }
    }
}
";
            File.WriteAllText(gradleFile, text.TrimEnd() + "\n" + rootBlock);
        }
    }
}
#endif
