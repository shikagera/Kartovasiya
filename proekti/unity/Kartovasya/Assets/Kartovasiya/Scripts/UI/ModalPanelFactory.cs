// Shared modal UI helpers / Общие модальные панели
// Chaos Spellweaver (Картовасия)

using UnityEngine;
using UnityEngine.Events;
using UnityEngine.UI;

namespace Kartovasiya.UI
{
    public static class ModalPanelFactory
    {
        public static GameObject CreateDimRoot(Transform canvas, string name, float alpha = 0.75f)
        {
            var go = new GameObject(name, typeof(RectTransform), typeof(Image), typeof(CanvasGroup));
            go.transform.SetParent(canvas, false);
            UiFactory.Stretch(go.GetComponent<RectTransform>());
            var img = go.GetComponent<Image>();
            img.color = new Color(0f, 0f, 0f, alpha);
            img.raycastTarget = true;
            go.SetActive(false);
            return go;
        }

        public static (GameObject panel, Text title, Text body) CreateCardPanel(
            Transform parent, string name, Vector2 anchorMin, Vector2 anchorMax)
        {
            var panel = new GameObject(name, typeof(RectTransform), typeof(Image));
            panel.transform.SetParent(parent, false);
            var pr = panel.GetComponent<RectTransform>();
            pr.anchorMin = anchorMin;
            pr.anchorMax = anchorMax;
            pr.offsetMin = Vector2.zero;
            pr.offsetMax = Vector2.zero;
            var img = panel.GetComponent<Image>();
            img.sprite = ProceduralSpriteFactory.RoundedRect(128, 96, 12, TarotPalette.VoidPurple, TarotPalette.Gold, 3);
            img.color = Color.white;
            img.raycastTarget = true;

            var title = UiFactory.Text(panel.transform, "Title", "", 28, TextAnchor.UpperCenter, TarotPalette.GoldBright, FontStyle.Bold);
            UiFactory.Anchor(title.rectTransform, new Vector2(0.06f, 0.78f), new Vector2(0.94f, 0.95f));

            var body = UiFactory.Text(panel.transform, "Body", "", 18, TextAnchor.UpperLeft, TarotPalette.Parchment);
            UiFactory.Anchor(body.rectTransform, new Vector2(0.08f, 0.28f), new Vector2(0.92f, 0.76f));

            return (panel, title, body);
        }

        public static Button CreateButton(
            Transform parent, string name, string label,
            Vector2 aMin, Vector2 aMax, Color fill, UnityAction onClick)
        {
            var go = new GameObject(name, typeof(RectTransform), typeof(Image), typeof(Button));
            go.transform.SetParent(parent, false);
            var r = go.GetComponent<RectTransform>();
            r.anchorMin = aMin;
            r.anchorMax = aMax;
            r.offsetMin = Vector2.zero;
            r.offsetMax = Vector2.zero;
            var img = go.GetComponent<Image>();
            img.sprite = ProceduralSpriteFactory.RoundedRect(64, 32, 8, fill, TarotPalette.Gold, 2);
            img.color = Color.white;
            img.raycastTarget = true;
            var btn = go.GetComponent<Button>();
            btn.targetGraphic = img;
            btn.onClick.AddListener(() =>
            {
                ProceduralSfx.Instance?.PlayUi();
                onClick?.Invoke();
            });
            var text = UiFactory.Text(go.transform, "L", label, 18, TextAnchor.MiddleCenter, TarotPalette.Parchment, FontStyle.Bold);
            UiFactory.Stretch(text.rectTransform);
            return btn;
        }
    }
}
