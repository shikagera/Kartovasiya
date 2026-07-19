// Persistent music / SFX volume (PlayerPrefs)
// Громкость музыки и SFX с сохранением
// Chaos Spellweaver (Картовасия)

using UnityEngine;

namespace Kartovasiya.UI
{
    /// <summary>
    /// Global audio levels. Named GameAudioSettings to avoid clash with UnityEngine.AudioSettings.
    /// </summary>
    public static class GameAudioSettings
    {
        private const string KeyMusic = "kartovasiya.vol.music";
        private const string KeySfx = "kartovasiya.vol.sfx";

        /// <summary>0..1 music master (menu + run).</summary>
        public static float MusicVolume { get; private set; } = 0.7f;

        /// <summary>0..1 SFX master.</summary>
        public static float SfxVolume { get; private set; } = 0.8f;

        private const float BaseMenu = 0.28f;
        private const float BaseRun = 0.26f;
        private const float BaseSfx = 0.35f;

        public static void Load()
        {
            MusicVolume = Mathf.Clamp01(PlayerPrefs.GetFloat(KeyMusic, 0.7f));
            SfxVolume = Mathf.Clamp01(PlayerPrefs.GetFloat(KeySfx, 0.8f));
            Apply();
        }

        public static void SetMusicVolume(float v)
        {
            MusicVolume = Mathf.Clamp01(v);
            PlayerPrefs.SetFloat(KeyMusic, MusicVolume);
            PlayerPrefs.Save();
            ApplyMusic();
        }

        public static void SetSfxVolume(float v)
        {
            SfxVolume = Mathf.Clamp01(v);
            PlayerPrefs.SetFloat(KeySfx, SfxVolume);
            PlayerPrefs.Save();
            ApplySfx();
        }

        public static void Apply()
        {
            ApplyMusic();
            ApplySfx();
        }

        private static void ApplyMusic()
        {
            ProceduralMenuMusic.Instance?.SetVolume(BaseMenu * MusicVolume);
            ProceduralRunMusic.Instance?.SetVolume(BaseRun * MusicVolume);
        }

        private static void ApplySfx()
        {
            ProceduralSfx.Instance?.SetVolume(BaseSfx * SfxVolume);
            ProceduralSfx.Instance?.SetMuted(SfxVolume <= 0.001f);
        }
    }
}
