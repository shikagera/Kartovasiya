// Editor utility: generate 16 ElementSO + ElementDatabase
// Утилита редактора: создать 16 стихий и каталог
// Chaos Spellweaver (Картовасия)

#if UNITY_EDITOR
using System.Collections.Generic;
using System.IO;
using UnityEditor;
using UnityEngine;
using Kartovasiya.Elements;

namespace Kartovasiya.EditorTools
{
    /// <summary>
    /// Menu: Kartovasiya → Generate Default Elements
    /// Creates assets under Assets/Kartovasiya/Data/Elements/
    /// </summary>
    public static class ElementDatabaseGenerator
    {
        private const string ElementsFolder = "Assets/Kartovasiya/Data/Elements";
        private const string DatabasePath = "Assets/Kartovasiya/Data/Elements/ElementDatabase.asset";

        [MenuItem("Kartovasiya/Generate Default Elements", priority = 0)]
        public static void GenerateDefaultElements()
        {
            EnsureFolder("Assets/Kartovasiya");
            EnsureFolder("Assets/Kartovasiya/Data");
            EnsureFolder(ElementsFolder);

            var list = new List<ElementSO>(ElementDefaults.All.Length);

            foreach (var def in ElementDefaults.All)
            {
                string assetPath = $"{ElementsFolder}/Element_{def.Id}.asset";
                var existing = AssetDatabase.LoadAssetAtPath<ElementSO>(assetPath);
                ElementSO so = existing != null ? existing : ScriptableObject.CreateInstance<ElementSO>();

                so.EditorApply(
                    def.Id,
                    def.Tier,
                    def.NameEn,
                    def.NameRu,
                    def.Primary,
                    def.Accent,
                    def.Opposed,
                    def.SortOrder,
                    def.DescEn,
                    def.DescRu);

                if (existing == null)
                    AssetDatabase.CreateAsset(so, assetPath);
                else
                    EditorUtility.SetDirty(so);

                list.Add(so);
            }

            var db = AssetDatabase.LoadAssetAtPath<ElementDatabaseSO>(DatabasePath);
            if (db == null)
            {
                db = ScriptableObject.CreateInstance<ElementDatabaseSO>();
                AssetDatabase.CreateAsset(db, DatabasePath);
            }

            db.EditorSetElements(list);
            EditorUtility.SetDirty(db);

            AssetDatabase.SaveAssets();
            AssetDatabase.Refresh();

            Selection.activeObject = db;
            EditorGUIUtility.PingObject(db);

            Debug.Log($"[Kartovasiya] Generated {list.Count} elements + ElementDatabase at {DatabasePath}");
        }

        private static void EnsureFolder(string path)
        {
            if (AssetDatabase.IsValidFolder(path))
                return;

            string parent = Path.GetDirectoryName(path)?.Replace('\\', '/');
            string name = Path.GetFileName(path);
            if (!string.IsNullOrEmpty(parent) && !AssetDatabase.IsValidFolder(parent))
                EnsureFolder(parent);

            if (!string.IsNullOrEmpty(parent) && !string.IsNullOrEmpty(name))
                AssetDatabase.CreateFolder(parent, name);
        }
    }
}
#endif
