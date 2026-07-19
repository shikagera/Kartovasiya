// Editor: seed ComboDatabase from ComboDefaults
// Редактор: сид ComboDatabase из ComboDefaults
// Chaos Spellweaver (Картовасия)

#if UNITY_EDITOR
using System.IO;
using UnityEditor;
using UnityEngine;
using Kartovasiya.Combos;

namespace Kartovasiya.EditorTools
{
    public static class ComboDatabaseGenerator
    {
        private const string Folder = "Assets/Kartovasiya/Data/Combos";
        private const string Path = Folder + "/ComboDatabase.asset";

        [MenuItem("Kartovasiya/Generate Default Combos", priority = 1)]
        public static void Generate()
        {
            EnsureFolder("Assets/Kartovasiya");
            EnsureFolder("Assets/Kartovasiya/Data");
            EnsureFolder(Folder);

            var db = AssetDatabase.LoadAssetAtPath<ComboDatabaseSO>(Path);
            if (db == null)
            {
                db = ScriptableObject.CreateInstance<ComboDatabaseSO>();
                AssetDatabase.CreateAsset(db, Path);
            }

            db.EditorSetRecipes(ComboDefaults.CreateAll());
            EditorUtility.SetDirty(db);
            AssetDatabase.SaveAssets();
            AssetDatabase.Refresh();

            Selection.activeObject = db;
            EditorGUIUtility.PingObject(db);
            Debug.Log($"[Kartovasiya] ComboDatabase generated with {db.Recipes.Count} recipes → {Path}");
        }

        private static void EnsureFolder(string path)
        {
            if (AssetDatabase.IsValidFolder(path))
                return;

            string parent = System.IO.Path.GetDirectoryName(path)?.Replace('\\', '/');
            string name = System.IO.Path.GetFileName(path);
            if (!string.IsNullOrEmpty(parent) && !AssetDatabase.IsValidFolder(parent))
                EnsureFolder(parent);

            if (!string.IsNullOrEmpty(parent) && !string.IsNullOrEmpty(name))
                AssetDatabase.CreateFolder(parent, name);
        }
    }
}
#endif
