// Lightweight device tuning for mid/low Android
// Лёгкая подстройка под средние/слабые Android
// Chaos Spellweaver (Картовасия)

using UnityEngine;

namespace Kartovasiya.Core
{
    public static class MobilePerformance
    {
        public static bool IsLowEnd { get; private set; }

        public static void Apply()
        {
#if UNITY_ANDROID && !UNITY_EDITOR
            int ram = SystemInfo.systemMemorySize; // MB
            int cores = SystemInfo.processorCount;
            IsLowEnd = ram > 0 && (ram < 3500 || cores <= 4);

            Application.targetFrameRate = IsLowEnd ? BalanceConfig.LowEndFps : BalanceConfig.TargetFps;
            QualitySettings.vSyncCount = 0;

            // Prefer a cheaper quality tier on weak devices
            if (IsLowEnd)
            {
                int low = Mathf.Min(1, QualitySettings.names.Length - 1);
                QualitySettings.SetQualityLevel(low, true);
                // Slightly reduce resolution scale if supported
                if (QualitySettings.resolutionScalingFixedDPIFactor > 0.85f)
                    QualitySettings.resolutionScalingFixedDPIFactor = 0.85f;
            }
            else
            {
                Application.targetFrameRate = BalanceConfig.TargetFps;
            }

            // Reduce stutter from GC on mid devices
            if (IsLowEnd)
                QualitySettings.asyncUploadTimeSlice = 1;

            Debug.Log($"[Kartovasiya] MobilePerformance lowEnd={IsLowEnd} ram={ram}MB cores={cores} fps={Application.targetFrameRate}");
#else
            IsLowEnd = false;
            Application.targetFrameRate = BalanceConfig.TargetFps;
            QualitySettings.vSyncCount = 0;
#endif
        }
    }
}
