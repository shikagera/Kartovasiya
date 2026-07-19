# Rebuild KTV app icons + menu backdrop (Windows, .NET Framework)
$csc = "$env:WINDIR\Microsoft.NET\Framework64\v4.0.30319\csc.exe"
$cs = Join-Path $PSScriptRoot "GenKtvIcon.cs"
$exe = Join-Path $env:TEMP "GenKtvIcon.exe"
& $csc /nologo /t:exe /r:System.Drawing.dll /out:$exe $cs
if ($LASTEXITCODE -ne 0) { throw "csc failed" }
& $exe
