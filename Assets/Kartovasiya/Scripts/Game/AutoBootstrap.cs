// Ensures GameApp exists when pressing Play on SampleScene
// Гарантирует GameApp при Play на SampleScene
// Chaos Spellweaver (Картовасия)

using UnityEngine;

namespace Kartovasiya.Game
{
    /// <summary>
    /// Runtime auto-hook so prototype works without manually wiring the scene.
    /// Авто-старт прототипа без ручной настройки сцены.
    /// Disable by defining KARTOVASIYA_NO_AUTO_BOOT in Player Settings.
    /// </summary>
    public static class AutoBootstrap
    {
#if !KARTOVASIYA_NO_AUTO_BOOT
        [RuntimeInitializeOnLoadMethod(RuntimeInitializeLoadType.AfterSceneLoad)]
        private static void EnsureGameApp()
        {
            if (Object.FindAnyObjectByType<GameApp>() != null)
                return;

            // Avoid hijacking non-game tooling scenes if any
            var go = new GameObject("Kartovasiya_GameApp");
            Object.DontDestroyOnLoad(go);
            go.AddComponent<GameApp>();
            Debug.Log("[Kartovasiya] AutoBootstrap: GameApp created.");
        }
#endif
    }
}
