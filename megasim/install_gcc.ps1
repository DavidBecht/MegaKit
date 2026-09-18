# install_gcc.ps1 — Laedt die vollstaendige WinLibs-GCC nach megasim\winlibs\
#
# Nur noetig, um gcc_minimal\ neu zusammenzustellen (python build.py --gcc-neu),
# etwa fuer eine neuere GCC-Version. Fuer den normalen Build reicht das
# versionierte gcc_minimal\. winlibs\ wird nicht versioniert und kann danach
# wieder geloescht werden.
#
# Aufruf:  powershell -ExecutionPolicy Bypass -File install_gcc.ps1
# Braucht Internet (ca. 260 MB Download, entpackt ca. 1 GB).

$ErrorActionPreference = "Stop"
$GccDir = Join-Path $PSScriptRoot "winlibs"

if (Test-Path "$GccDir\bin\gcc.exe") {
    Write-Host "WinLibs-GCC liegt bereits in $GccDir"
    exit 0
}

$Url  = "https://github.com/brechtsanders/winlibs_mingw/releases/download/16.1.0posix-14.0.0-ucrt-r3/winlibs-x86_64-posix-seh-gcc-16.1.0-mingw-w64ucrt-14.0.0-r3.zip"
$Zip  = Join-Path $env:TEMP "winlibs-gcc.zip"

Write-Host "Lade WinLibs GCC 16.1.0 (ca. 260 MB) ..."
[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
Invoke-WebRequest -Uri $Url -OutFile $Zip -UseBasicParsing

Write-Host "Entpacke ..."
Add-Type -AssemblyName System.IO.Compression.FileSystem
$TmpDir = Join-Path $env:TEMP "winlibs_extract"
if (Test-Path $TmpDir) { Remove-Item $TmpDir -Recurse -Force }
[System.IO.Compression.ZipFile]::ExtractToDirectory($Zip, $TmpDir)

# Das Archiv enthaelt genau einen Unterordner mingw64\
$Inner = Get-ChildItem $TmpDir -Directory | Select-Object -First 1
if (-not $Inner) { Write-Error "Unerwarteter Aufbau des Archivs"; exit 1 }
Move-Item $Inner.FullName $GccDir

Remove-Item $Zip   -Force
Remove-Item $TmpDir -Recurse -Force -ErrorAction SilentlyContinue

Write-Host ""
Write-Host "Fertig: $GccDir"
Write-Host "Weiter mit:  python build.py --gcc-neu"
