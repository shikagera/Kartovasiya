// Offline run save I/O / Файловый сейв run (оффлайн)
// Chaos Spellweaver (Картовасия)

using System;
using System.IO;
using UnityEngine;

namespace Kartovasiya.Save
{
    /// <summary>
    /// JSON save under Application.persistentDataPath (Android-safe).
    /// JSON в persistentDataPath — работает оффлайн на Android.
    /// </summary>
    public static class RunSaveService
    {
        public const string FileName = "kartovasiya_run.json";

        public static string SavePath =>
            Path.Combine(Application.persistentDataPath, FileName);

        public static bool HasSave()
        {
            try
            {
                return File.Exists(SavePath) && new FileInfo(SavePath).Length > 8;
            }
            catch
            {
                return false;
            }
        }

        public static bool TryLoad(out RunSaveData data)
        {
            data = null;
            if (!HasSave())
                return false;

            try
            {
                string json = File.ReadAllText(SavePath);
                if (string.IsNullOrWhiteSpace(json))
                    return false;

                data = JsonUtility.FromJson<RunSaveData>(json);
                if (data == null)
                    return false;

                if (data.version > RunSaveData.CurrentVersion)
                {
                    Debug.LogWarning($"[RunSave] Save version {data.version} newer than supported {RunSaveData.CurrentVersion}.");
                }

                // Basic integrity
                if (data.playerHp <= 0 && data.phase == (int)Core.TurnPhase.Defeat)
                    return false;

                return true;
            }
            catch (Exception e)
            {
                Debug.LogError($"[RunSave] Load failed: {e.Message}");
                data = null;
                return false;
            }
        }

        public static bool Save(RunSaveData data)
        {
            if (data == null)
                return false;

            try
            {
                data.version = RunSaveData.CurrentVersion;
                data.savedUtcTicks = DateTime.UtcNow.Ticks;

                string json = JsonUtility.ToJson(data, prettyPrint: true);
                string path = SavePath;
                string dir = Path.GetDirectoryName(path);
                if (!string.IsNullOrEmpty(dir) && !Directory.Exists(dir))
                    Directory.CreateDirectory(dir);

                // Atomic-ish write: temp then replace
                string tmp = path + ".tmp";
                File.WriteAllText(tmp, json);
                if (File.Exists(path))
                    File.Delete(path);
                File.Move(tmp, path);

                Debug.Log($"[RunSave] Saved → {path}");
                return true;
            }
            catch (Exception e)
            {
                Debug.LogError($"[RunSave] Save failed: {e.Message}");
                return false;
            }
        }

        public static bool Delete()
        {
            try
            {
                if (File.Exists(SavePath))
                {
                    File.Delete(SavePath);
                    Debug.Log("[RunSave] Save deleted.");
                }

                string tmp = SavePath + ".tmp";
                if (File.Exists(tmp))
                    File.Delete(tmp);

                return true;
            }
            catch (Exception e)
            {
                Debug.LogError($"[RunSave] Delete failed: {e.Message}");
                return false;
            }
        }

        /// <summary>Human-readable age for Continue UI. / Возраст сейва для UI.</summary>
        public static string FormatSaveAge(RunSaveData data, bool russian = true)
        {
            if (data == null || data.savedUtcTicks <= 0)
                return russian ? "неизвестно" : "unknown";

            try
            {
                var saved = new DateTime(data.savedUtcTicks, DateTimeKind.Utc);
                var span = DateTime.UtcNow - saved;
                if (span.TotalMinutes < 1)
                    return russian ? "только что" : "just now";
                if (span.TotalHours < 1)
                    return russian ? $"{(int)span.TotalMinutes} мин. назад" : $"{(int)span.TotalMinutes}m ago";
                if (span.TotalDays < 1)
                    return russian ? $"{(int)span.TotalHours} ч. назад" : $"{(int)span.TotalHours}h ago";
                return russian ? $"{(int)span.TotalDays} дн. назад" : $"{(int)span.TotalDays}d ago";
            }
            catch
            {
                return russian ? "ранее" : "earlier";
            }
        }
    }
}
