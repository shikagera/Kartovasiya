# Build Android APK into projectRoot/apk/ via Unity batchmode.
# Usage (from project root or anywhere):
#   .\Tools\BuildApk.ps1
# Optional:
#   .\Tools\BuildApk.ps1 -ApkName "MyBuild.apk"
#   .\Tools\BuildApk.ps1 -UnityPath "C:\Program Files\Unity\Hub\Editor\6000.5.4f1\Editor\Unity.exe"

param(
    [string]$ApkName = "ChaosSpellweaver.apk",
    [string]$UnityPath = ""
)

$ErrorActionPreference = "Stop"
$ProjectRoot = Split-Path -Parent $PSScriptRoot
$ApkDir = Join-Path $ProjectRoot "apk"
$ApkPath = Join-Path $ApkDir $ApkName
$LogPath = Join-Path $ProjectRoot "Logs\BuildApk.log"

function Find-Unity {
    if ($UnityPath -and (Test-Path $UnityPath)) { return $UnityPath }

    $versionFile = Join-Path $ProjectRoot "ProjectSettings\ProjectVersion.txt"
    $version = $null
    if (Test-Path $versionFile) {
        $line = Get-Content $versionFile | Where-Object { $_ -match 'm_EditorVersion:\s*(.+)' } | Select-Object -First 1
        if ($line -match 'm_EditorVersion:\s*(.+)') { $version = $Matches[1].Trim() }
    }

    if ($version) {
        $candidate = "C:\Program Files\Unity\Hub\Editor\$version\Editor\Unity.exe"
        if (Test-Path $candidate) { return $candidate }
    }

    $hub = "C:\Program Files\Unity\Hub\Editor"
    if (Test-Path $hub) {
        $latest = Get-ChildItem $hub -Directory | Sort-Object Name -Descending | Select-Object -First 1
        if ($latest) {
            $exe = Join-Path $latest.FullName "Editor\Unity.exe"
            if (Test-Path $exe) { return $exe }
        }
    }

    return $null
}

$unity = Find-Unity
if (-not $unity) {
    Write-Host "Unity Editor not found. Install matching version via Unity Hub." -ForegroundColor Red
    exit 1
}

# Project must not be open in another Unity instance
$lock = Join-Path $ProjectRoot "Temp\UnityLockfile"
if (Test-Path $lock) {
    Write-Host "Project is locked (Unity already open)." -ForegroundColor Yellow
    Write-Host "Close the Unity Editor for this project, then re-run:" -ForegroundColor Yellow
    Write-Host "  .\Tools\BuildApk.ps1" -ForegroundColor Cyan
    Write-Host ""
    Write-Host "Or in open Editor: menu Kartovasiya → Build APK to apk/" -ForegroundColor Cyan
    exit 2
}

New-Item -ItemType Directory -Force -Path $ApkDir | Out-Null
New-Item -ItemType Directory -Force -Path (Split-Path $LogPath) | Out-Null

Write-Host "Unity:  $unity" -ForegroundColor Cyan
Write-Host "Project: $ProjectRoot" -ForegroundColor Cyan
Write-Host "Output:  $ApkPath" -ForegroundColor Cyan
Write-Host "Log:     $LogPath" -ForegroundColor Cyan
Write-Host "Building (this can take several minutes)..." -ForegroundColor Yellow

$args = @(
    "-batchmode",
    "-nographics",
    "-quit",
    "-projectPath", $ProjectRoot,
    "-buildTarget", "Android",
    "-executeMethod", "Kartovasiya.EditorTools.AndroidApkBuilder.BuildApk",
    "-apkPath", $ApkPath,
    "-logFile", $LogPath
)

$p = Start-Process -FilePath $unity -ArgumentList $args -Wait -PassThru
$code = $p.ExitCode

if ($code -eq 0 -and (Test-Path $ApkPath)) {
    $size = [math]::Round((Get-Item $ApkPath).Length / 1MB, 1)
    Write-Host ""
    Write-Host "SUCCESS: $ApkPath ($size MB)" -ForegroundColor Green
    exit 0
}

Write-Host ""
Write-Host "Build failed (exit $code). Check log: $LogPath" -ForegroundColor Red
if (Test-Path $LogPath) {
    Write-Host "--- last 40 log lines ---" -ForegroundColor Yellow
    Get-Content $LogPath -Tail 40
}
exit $(if ($code -ne 0) { $code } else { 1 })
