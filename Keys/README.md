# Android signing keys (Kartovasiya)

## Dev keystore (prototype builds)

Generate once:

```powershell
cd path\to\картовасия
.\Tools\CreateDevKeystore.ps1
```

Creates:

- `Keys/kartovasiya-dev.keystore`
- `Keys/DEV_KEYSTORE_CREDENTIALS.txt`

| Field | Value |
|--------|--------|
| Alias | `kartovasiya` |
| Password | `kartovasiya_dev` |

In Unity after generation:

**Kartovasiya → Setup Icons & Dev Keystore**

Or manually: Player Settings → Android → Publishing Settings → Custom Keystore.

## Production

Do **not** ship with the dev keystore.

1. Create your own keystore (unique password, backup offline).
2. Point Unity Publishing Settings to it.
3. Never commit production passwords to git.

## Git

- Dev keystore *may* be committed for team prototype convenience.
- Prefer ignoring production `*.keystore` and password files.
