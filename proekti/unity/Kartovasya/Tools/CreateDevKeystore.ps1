# Create development Android keystore for Chaos Spellweaver (Kartovasiya)
# Dev keystore — NOT for Google Play production!
#
# Usage (PowerShell from project root):
#   .\Tools\CreateDevKeystore.ps1
#
# Credentials (dev only):
#   store/key password: kartovasiya_dev
#   alias: kartovasiya

$ErrorActionPreference = "Stop"
$ProjectRoot = Split-Path -Parent $PSScriptRoot
$KeystoreDir = Join-Path $ProjectRoot "Keys"
$KeystorePath = Join-Path $KeystoreDir "kartovasiya-dev.keystore"
$Alias = "kartovasiya"
$Pass = "kartovasiya_dev"
$DName = "CN=Kartovasiya Dev, OU=Prototype, O=Kartovasiya, L=Offline, ST=NA, C=RU"

function Find-Keytool {
    $candidates = @()

    # Unity OpenJDK (common paths)
    $unityRoot = "C:\Program Files\Unity\Hub\Editor"
    if (Test-Path $unityRoot) {
        Get-ChildItem $unityRoot -Directory -ErrorAction SilentlyContinue | ForEach-Object {
            $candidates += (Join-Path $_.FullName "Editor\Data\PlaybackEngines\AndroidPlayer\OpenJDK\bin\keytool.exe")
            $candidates += (Join-Path $_.FullName "Editor\Data\PlaybackEngines\AndroidPlayer\OpenJDK\bin\keytool")
        }
    }

    $candidates += @(
        "$env:JAVA_HOME\bin\keytool.exe",
        "$env:JAVA_HOME\bin\keytool",
        "C:\Program Files\Android\Android Studio\jbr\bin\keytool.exe",
        "C:\Program Files\Android\Android Studio\jre\bin\keytool.exe",
        "C:\Program Files\Eclipse Adoptium\jdk-17*\bin\keytool.exe",
        "C:\Program Files\Microsoft\jdk-*\bin\keytool.exe"
    )

    foreach ($c in $candidates) {
        $resolved = Get-Item $c -ErrorAction SilentlyContinue | Select-Object -First 1
        if ($resolved) { return $resolved.FullName }
    }

    $cmd = Get-Command keytool -ErrorAction SilentlyContinue
    if ($cmd) { return $cmd.Source }
    return $null
}

$keytool = Find-Keytool
if (-not $keytool) {
    Write-Host ""
    Write-Host "keytool not found." -ForegroundColor Red
    Write-Host "Install one of:" -ForegroundColor Yellow
    Write-Host "  1) Unity Hub → this Editor → Add Modules → Android Build Support + OpenJDK"
    Write-Host "  2) Android Studio (includes JBR)"
    Write-Host "  3) JDK 17+ and set JAVA_HOME"
    Write-Host ""
    Write-Host "Then re-run: .\Tools\CreateDevKeystore.ps1"
    exit 1
}

Write-Host "Using keytool: $keytool" -ForegroundColor Cyan
New-Item -ItemType Directory -Force -Path $KeystoreDir | Out-Null

if (Test-Path $KeystorePath) {
    Write-Host "Keystore already exists: $KeystorePath" -ForegroundColor Yellow
    Write-Host "Delete it first if you want to regenerate."
    exit 0
}

& $keytool -genkeypair `
    -v `
    -keystore $KeystorePath `
    -alias $Alias `
    -keyalg RSA `
    -keysize 2048 `
    -validity 10000 `
    -storepass $Pass `
    -keypass $Pass `
    -dname $DName

if ($LASTEXITCODE -ne 0) {
    Write-Host "keytool failed." -ForegroundColor Red
    exit $LASTEXITCODE
}

$credPath = Join-Path $KeystoreDir "DEV_KEYSTORE_CREDENTIALS.txt"
@"
Kartovasiya DEV keystore (NOT for production / Play Store)
==========================================================
File:     Keys/kartovasiya-dev.keystore
Alias:    $Alias
Password: $Pass
(store password == key password)

In Unity:
  Player Settings → Android → Publishing Settings
  - Use Custom Keystore = ON
  - Keystore Manager / path: Keys/kartovasiya-dev.keystore
  - Password / Alias as above

Or menu: Kartovasiya → Setup Icons & Dev Keystore

Replace with your own keystore before Google Play release.
"@ | Set-Content -Path $credPath -Encoding UTF8

Write-Host ""
Write-Host "Created: $KeystorePath" -ForegroundColor Green
Write-Host "Credentials: $credPath" -ForegroundColor Green
Write-Host "Password: $Pass  |  Alias: $Alias" -ForegroundColor Cyan
