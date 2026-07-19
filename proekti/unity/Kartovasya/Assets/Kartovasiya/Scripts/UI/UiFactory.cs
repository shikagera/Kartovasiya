// Shared uGUI builders / Общие билдеры uGUI
// Chaos Spellweaver (Картовасия)

using UnityEngine;
using UnityEngine.UI;

namespace Kartovasiya.UI
{
    public static class UiFactory
    {
        public static Font DefaultFont()
        {
            var f = Resources.GetBuiltinResource<Font>("LegacyRuntime.ttf");
            if (f == null)
                f = Resources.GetBuiltinResource<Font>("Arial.ttf");
            return f;
        }

        public static Text Text(
            Transform parent,
            string name,
            string value,
            int size,
            TextAnchor anchor,
            Color color,
            FontStyle style = FontStyle.Normal)
        {
            var go = new GameObject(name, typeof(RectTransform), typeof(Text));
            go.transform.SetParent(parent, false);
            var t = go.GetComponent<Text>();
            t.font = DefaultFont();
            t.text = value;
            t.fontSize = size;
            t.alignment = anchor;
            t.color = color;
            t.fontStyle = style;
            t.horizontalOverflow = HorizontalWrapMode.Wrap;
            t.verticalOverflow = VerticalWrapMode.Truncate;
            t.raycastTarget = false;
            // Soft shadow via second text optional — keep light for mobile
            return t;
        }

        // Method name Image collides with UnityEngine.UI.Image type — use full type name below.
        public static UnityEngine.UI.Image Image(Transform parent, string name, Sprite sprite, Color color, bool raycast = false)
        {
            var go = new GameObject(name, typeof(RectTransform), typeof(UnityEngine.UI.Image));
            go.transform.SetParent(parent, false);
            var img = go.GetComponent<UnityEngine.UI.Image>();
            img.sprite = sprite;
            img.color = color;
            img.raycastTarget = raycast;
            img.type = sprite != null && sprite.border.sqrMagnitude > 0
                ? UnityEngine.UI.Image.Type.Sliced
                : UnityEngine.UI.Image.Type.Simple;
            img.preserveAspect = false;
            return img;
        }

        public static void Stretch(RectTransform r, float pad = 0f)
        {
            r.anchorMin = Vector2.zero;
            r.anchorMax = Vector2.one;
            r.offsetMin = new Vector2(pad, pad);
            r.offsetMax = new Vector2(-pad, -pad);
        }

        public static void Anchor(RectTransform r, Vector2 min, Vector2 max, Vector2? offsetMin = null, Vector2? offsetMax = null)
        {
            r.anchorMin = min;
            r.anchorMax = max;
            r.offsetMin = offsetMin ?? Vector2.zero;
            r.offsetMax = offsetMax ?? Vector2.zero;
        }
    }
}
