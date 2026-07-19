// Craft UI: pick hand spell/effect → mini-game → upgrade
// UI крафта: выбор карты → мини-игра → улучшение
// Chaos Spellweaver (Картовасия)

using System;
using UnityEngine;
using UnityEngine.UI;
using Kartovasiya.Cards;
using Kartovasiya.Core;
using Kartovasiya.Hand;
using Kartovasiya.UI;

namespace Kartovasiya.Crafting
{
    public class CraftPanel : MonoBehaviour
    {
        public static CraftPanel Instance { get; private set; }

        private GameObject _root;
        private Text _title;
        private Text _body;
        private Text _mats;
        private MaterialInventory _inv;
        private HandManager _hand;
        private bool _russian = true;
        private CardInstance _selected;
        private Action _onClosed;
        private Action<string> _log;

        private void Awake() => Instance = this;
        private void OnDestroy()
        {
            if (Instance == this) Instance = null;
        }

        public void Build(Transform canvas)
        {
            _root = ModalPanelFactory.CreateDimRoot(canvas, "CraftPanel");
            var built = ModalPanelFactory.CreateCardPanel(
                _root.transform, "CraftCard",
                new Vector2(0.08f, 0.18f), new Vector2(0.92f, 0.82f));
            _title = built.title;
            _body = built.body;
            _title.text = _russian ? "КУЗНИЦА" : "FORGE";

            _mats = UiFactory.Text(built.panel.transform, "Mats", "", 16, TextAnchor.MiddleCenter, TarotPalette.Gold);
            UiFactory.Anchor(_mats.rectTransform, new Vector2(0.08f, 0.22f), new Vector2(0.92f, 0.3f));

            ModalPanelFactory.CreateButton(built.panel.transform, "BtnPrev", "◀",
                new Vector2(0.08f, 0.08f), new Vector2(0.22f, 0.18f),
                new Color(0.25f, 0.25f, 0.35f), CyclePrev);

            ModalPanelFactory.CreateButton(built.panel.transform, "BtnNext", "▶",
                new Vector2(0.24f, 0.08f), new Vector2(0.38f, 0.18f),
                new Color(0.25f, 0.25f, 0.35f), CycleNext);

            ModalPanelFactory.CreateButton(built.panel.transform, "BtnCraft",
                _russian ? "Ковать" : "Forge",
                new Vector2(0.42f, 0.08f), new Vector2(0.72f, 0.18f),
                new Color(0.35f, 0.2f, 0.45f), StartCraft);

            ModalPanelFactory.CreateButton(built.panel.transform, "BtnClose",
                _russian ? "Закрыть" : "Close",
                new Vector2(0.74f, 0.08f), new Vector2(0.92f, 0.18f),
                new Color(0.4f, 0.2f, 0.2f), Close);
        }

        public void Open(MaterialInventory inv, HandManager hand, bool russian, Action<string> log, Action onClosed)
        {
            _inv = inv;
            _hand = hand;
            _russian = russian;
            _log = log;
            _onClosed = onClosed;
            _selected = FindFirstCraftable();
            Refresh();
            _root.SetActive(true);
            _root.transform.SetAsLastSibling();
            if (_inv != null)
            {
                _inv.Changed -= Refresh;
                _inv.Changed += Refresh;
            }
        }

        public void Close()
        {
            if (_root == null || !_root.activeSelf)
            {
                if (_root != null)
                    _root.SetActive(false);
                return;
            }

            if (_inv != null)
                _inv.Changed -= Refresh;
            _root.SetActive(false);
            var cb = _onClosed;
            _onClosed = null;
            cb?.Invoke();
        }

        public bool IsOpen => _root != null && _root.activeSelf;

        private void Refresh()
        {
            if (_mats != null && _inv != null)
                _mats.text = _inv.Summary(_russian);

            if (_selected == null)
                _selected = FindFirstCraftable();

            if (_selected == null)
            {
                _body.text = _russian
                    ? "Нет подходящих карт в руке\n(нужно заклинание или эффект)."
                    : "No craftable cards in hand\n(spell or effect).";
                return;
            }

            CraftCosts.GetUpgradeCost(_selected.CraftLevel, out int e, out int s, out int d, out int c);
            float preview = CraftCosts.SuccessChance(_selected.CraftLevel, 0.7f);
            _body.text = _russian
                ? $"Карта: {_selected.GetDisplayName(true)}\nТип: {_selected.CardType}  |  Крафт lvl {_selected.CraftLevel}\nСила: {_selected.GetFinalPower()}\n\nСтоимость: Э{e} О{s} П{d} Я{c}\nШанс ≈{GameConstants.BaseCraftSuccessChance:P0}…{GameConstants.MaxCraftSuccessChance:P0}\n(зависит от мини-игры, макс 70%)\n\n◀ ▶ выбор карты"
                : $"Card: {_selected.GetDisplayName(false)}\nType: {_selected.CardType}  |  Craft {_selected.CraftLevel}\nPower: {_selected.GetFinalPower()}\n\nCost: E{e} S{s} D{d} C{c}\nChance depends on mini-game (max 70%)\n\n◀ ▶ select card";
        }

        private CardInstance FindFirstCraftable()
        {
            if (_hand == null) return null;
            for (int i = 0; i < _hand.Cards.Count; i++)
            {
                var c = _hand.Cards[i];
                if (c != null && (c.CardType == CardType.Spell || c.CardType == CardType.Effect))
                    return c;
            }

            return null;
        }

        private void CycleNext()
        {
            Cycle(+1);
        }

        private void CyclePrev()
        {
            Cycle(-1);
        }

        private void Cycle(int dir)
        {
            if (_hand == null || _hand.Count == 0) return;
            int start = 0;
            for (int i = 0; i < _hand.Cards.Count; i++)
            {
                if (_hand.Cards[i] == _selected)
                {
                    start = i;
                    break;
                }
            }

            for (int step = 1; step <= _hand.Count; step++)
            {
                int idx = (start + dir * step + _hand.Count * 8) % _hand.Count;
                var c = _hand.Cards[idx];
                if (c != null && (c.CardType == CardType.Spell || c.CardType == CardType.Effect))
                {
                    _selected = c;
                    Refresh();
                    return;
                }
            }
        }

        private void StartCraft()
        {
            if (_selected == null || _inv == null)
                return;

            if (!CraftService.CanCraft(_selected, _inv, out string fail))
            {
                _log?.Invoke(fail);
                ProceduralSfx.Instance?.PlayDropFail();
                return;
            }

            // Hide craft UI during mini-game
            _root.SetActive(false);

            float green = Mathf.Lerp(0.14f, 0.09f, _selected.CraftLevel / 5f);
            TimingMiniGame.Instance?.Play(MiniGameContext.Craft, _russian, skill =>
            {
                var result = CraftService.TryCraft(_selected, _inv, skill);
                _log?.Invoke(_russian ? result.MessageRu : result.MessageEn);

                if (result.Success)
                {
                    CombatVfxService.Instance?.PlayComboBurst(
                        Vector3.zero, TarotPalette.GoldBright,
                        _russian ? "КОВКА!" : "FORGED!", false);
                    var view = _hand?.FindView(_selected);
                    view?.RefreshVisuals();
                    view?.PlayFuseFeedback(TarotPalette.Gold);
                }
                else
                {
                    var view = _hand?.FindView(_selected);
                    view?.RefreshVisuals();
                    view?.Animator?.PlayReject();
                }

                _root.SetActive(true);
                Refresh();
            }, green);
        }
    }
}
