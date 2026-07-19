# App icons (иконка на рабочем столе телефона)

| File | Use |
|------|-----|
| `AppIcon.png` | Main icon (512) — **assigned in Player Settings** |
| `AppIcon_1024.png` | High-res master |
| `AppIcon_192` … `_48` | Density sizes |

Art: three bright KTV cards (K · T · V) on dark purple.

## Already wired

`ProjectSettings` → Android icons (Adaptive / Round / Legacy) point to `AppIcon.png`.

After building APK and installing, the phone home screen shows this icon.

## Re-assign in Unity (if reset)

Menu: **Kartovasiya → Setup App Icons Only**  
or **Kartovasiya → Setup Icons & Dev Keystore**

## Regenerate PNG

```powershell
.\Tools\GenerateKtvIcons.ps1
```

Then run **Setup App Icons Only** again.
