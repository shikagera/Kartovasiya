// Legacy entry → redirects to GameApp (full prototype)
// Старый entry → перенаправление на GameApp
// Chaos Spellweaver (Картовасия)

using UnityEngine;
using Kartovasiya.Game;

namespace Kartovasiya.Combat
{
    /// <summary>
    /// Kept for compatibility. Prefer adding <see cref="GameApp"/> instead.
    /// Сохранён для совместимости. Лучше вешать GameApp.
    /// </summary>
    public class CombatPrototypeBootstrap : MonoBehaviour
    {
        [SerializeField] private bool _buildOnAwake = true;

        private void Awake()
        {
            if (!_buildOnAwake)
                return;

            if (GameApp.Instance != null)
                return;

            // If already has GameApp on same object, do nothing
            if (GetComponent<GameApp>() != null)
                return;

            gameObject.AddComponent<GameApp>();
            // GameApp builds on its own Awake — order may run same frame after this
            // Force build if it already ran empty
            var app = GetComponent<GameApp>();
            // GameApp with buildOnAwake will run Awake when added... AddComponent calls Awake immediately
        }
    }
}
