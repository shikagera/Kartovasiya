// Hand layout math (bottom row / side stack)
// Раскладка руки (низ / боковой столбец)
// Chaos Spellweaver (Картовасия)

using UnityEngine;
using Kartovasiya.Core;

namespace Kartovasiya.Hand
{
    public enum HandLayoutStyle
    {
        /// <summary>Cards in a bottom row (StS-like, for vertical combat). / Ряд снизу.</summary>
        BottomRow = 0,

        /// <summary>Vertical stack on the side (landscape option). / Столбец сбоку.</summary>
        SideStack = 1
    }

    /// <summary>
    /// Pure layout helper: positions card RectTransforms inside a hand container.
    /// Чистая раскладка: позиции карт в контейнере руки.
    /// </summary>
    public static class HandLayout
    {
        public static void Apply(
            RectTransform container,
            CardView[] views,
            int count,
            HandLayoutStyle style,
            ScreenLayoutMode screen,
            Vector2 cardSize,
            float spacing,
            float fanAngleMax)
        {
            if (container == null || views == null || count <= 0)
                return;

            // Slightly denser on portrait phones
            float space = spacing;
            if (screen == ScreenLayoutMode.Portrait)
                space *= 0.85f;

            if (style == HandLayoutStyle.SideStack)
                LayoutSideStack(container, views, count, cardSize, space);
            else
                LayoutBottomRow(container, views, count, cardSize, space, fanAngleMax);
        }

        private static void LayoutBottomRow(
            RectTransform container,
            CardView[] views,
            int count,
            Vector2 cardSize,
            float spacing,
            float fanAngleMax)
        {
            float totalWidth = count * cardSize.x + Mathf.Max(0, count - 1) * spacing;
            float containerW = container.rect.width;
            if (containerW > 1f && totalWidth > containerW)
            {
                // Overlap more when hand is full
                float overflow = totalWidth - containerW;
                spacing -= overflow / Mathf.Max(1, count - 1);
                spacing = Mathf.Max(-cardSize.x * 0.45f, spacing);
                totalWidth = count * cardSize.x + Mathf.Max(0, count - 1) * spacing;
            }

            float startX = -totalWidth * 0.5f + cardSize.x * 0.5f;
            float mid = (count - 1) * 0.5f;

            for (int i = 0; i < count; i++)
            {
                var view = views[i];
                if (view == null || view.Rect == null)
                    continue;

                // Normalize position across hand (-1..1); avoid div-by-zero when count==1
                float t = (count <= 1 || Mathf.Abs(mid) < 0.001f) ? 0f : (i - mid) / mid;
                float angle = -t * fanAngleMax;
                float yArc = -Mathf.Abs(t) * cardSize.y * 0.06f;

                view.Rect.anchorMin = new Vector2(0.5f, 0.5f);
                view.Rect.anchorMax = new Vector2(0.5f, 0.5f);
                view.Rect.pivot = new Vector2(0.5f, 0.5f);
                view.Rect.sizeDelta = cardSize;
                var pos = new Vector2(startX + i * (cardSize.x + spacing), yArc);
                var rot = Quaternion.Euler(0f, 0f, angle);
                view.Rect.anchoredPosition = pos;
                view.Rect.localRotation = rot;
                view.Rect.localScale = Vector3.one;
                view.HandIndex = i;
                view.transform.SetSiblingIndex(i);
                view.Animator?.SetBaseFromLayout(pos, rot, Vector3.one);
            }
        }

        private static void LayoutSideStack(
            RectTransform container,
            CardView[] views,
            int count,
            Vector2 cardSize,
            float spacing)
        {
            float totalH = count * cardSize.y + Mathf.Max(0, count - 1) * spacing;
            float startY = totalH * 0.5f - cardSize.y * 0.5f;

            for (int i = 0; i < count; i++)
            {
                var view = views[i];
                if (view == null || view.Rect == null)
                    continue;

                view.Rect.anchorMin = new Vector2(0.5f, 0.5f);
                view.Rect.anchorMax = new Vector2(0.5f, 0.5f);
                view.Rect.pivot = new Vector2(0.5f, 0.5f);
                view.Rect.sizeDelta = cardSize;
                var pos = new Vector2(0f, startY - i * (cardSize.y + spacing));
                view.Rect.anchoredPosition = pos;
                view.Rect.localRotation = Quaternion.identity;
                view.Rect.localScale = Vector3.one;
                view.HandIndex = i;
                view.transform.SetSiblingIndex(i);
                view.Animator?.SetBaseFromLayout(pos, Quaternion.identity, Vector3.one);
            }
        }
    }
}
