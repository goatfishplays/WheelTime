param(
    [string]$QtPrefix = "C:\Qt\6.11.1\msvc2022_64",
    [string]$BuildDir = "build-package",
    [string]$ArtifactsDir = "artifacts"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$buildPath = Join-Path $repoRoot $BuildDir
$artifactPath = Join-Path $repoRoot $ArtifactsDir
$stagePath = Join-Path $artifactPath "WheelTime-portable-windows"
$zipPath = Join-Path $artifactPath "WheelTime-portable-windows.zip"
$installerPath = Join-Path $artifactPath "WheelTimeSetup.exe"

function Invoke-Native {
    param(
        [string]$FilePath,
        [string[]]$Arguments
    )

    & $FilePath @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "$FilePath failed with exit code $LASTEXITCODE"
    }
}

function Resolve-CMake {
    $fromPath = Get-Command cmake.exe -ErrorAction SilentlyContinue
    if ($fromPath) {
        return $fromPath.Source
    }

    $defaultCMake = "C:\Program Files\CMake\bin\cmake.exe"
    if (Test-Path $defaultCMake) {
        return $defaultCMake
    }

    throw "Could not find cmake.exe. Install CMake or add it to PATH."
}

function Resolve-CPack {
    $fromPath = Get-Command cpack.exe -ErrorAction SilentlyContinue
    if ($fromPath) {
        return $fromPath.Source
    }

    $defaultCPack = "C:\Program Files\CMake\bin\cpack.exe"
    if (Test-Path $defaultCPack) {
        return $defaultCPack
    }

    throw "Could not find cpack.exe. Install CMake or add it to PATH."
}

$cmake = Resolve-CMake
$cpack = Resolve-CPack

if (!(Test-Path $QtPrefix)) {
    throw "Qt prefix was not found: $QtPrefix"
}

New-Item -ItemType Directory -Force -Path $artifactPath | Out-Null
if (Test-Path $stagePath) {
    Remove-Item -Recurse -Force -LiteralPath $stagePath
}
if (Test-Path $zipPath) {
    Remove-Item -Force -LiteralPath $zipPath
}
if (Test-Path $installerPath) {
    Remove-Item -Force -LiteralPath $installerPath
}

Write-Host "Configuring Release build..."
Invoke-Native $cmake @("-S", $repoRoot, "-B", $buildPath, "-G", "Visual Studio 17 2022", "-A", "x64", "-DCMAKE_PREFIX_PATH=$QtPrefix")

Write-Host "Building WheelTime Release..."
Invoke-Native $cmake @("--build", $buildPath, "--config", "Release", "--target", "WheelTime")

Write-Host "Installing portable staging folder..."
Invoke-Native $cmake @("--install", $buildPath, "--config", "Release", "--prefix", $stagePath)

Write-Host "Creating portable zip..."
Compress-Archive -Path (Join-Path $stagePath "*") -DestinationPath $zipPath -Force
Write-Host "Portable package: $zipPath"

$makensis = Get-Command makensis.exe -ErrorAction SilentlyContinue
if ($makensis) {
    Write-Host "Creating NSIS installer..."
    Invoke-Native $cpack @("-G", "NSIS", "-C", "Release", "--config", (Join-Path $buildPath "CPackConfig.cmake"), "-B", $artifactPath)

    $generatedInstaller = Get-ChildItem -Path $artifactPath -Filter "*.exe" -Recurse |
        Where-Object { $_.Name -like "WheelTime*.exe" } |
        Sort-Object LastWriteTime -Descending |
        Select-Object -First 1

    if (!$generatedInstaller) {
        throw "NSIS was available, but CPack did not produce an installer."
    }

    Copy-Item -LiteralPath $generatedInstaller.FullName -Destination $installerPath -Force
    Write-Host "Installer package: $installerPath"
}
else {
    Write-Host "NSIS was not found. Skipping installer; portable zip is ready."
    Write-Host "Install NSIS and rerun this script to produce WheelTimeSetup.exe."
}
