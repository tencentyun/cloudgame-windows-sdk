<# 
.SYNOPSIS
    Build script for CloudPhone_DuiLib_Demo

.DESCRIPTION
    Bootstraps vcpkg, configures CMake, and builds the project.

.PARAMETER Config
    Build configuration: "debug" or "release". Default: "debug"

.PARAMETER Clean
    Remove build directory before configuring.

.PARAMETER Open
    Open the Visual Studio solution after build.

.EXAMPLE
    .\build.ps1
    .\build.ps1 -Config release
    .\build.ps1 -Clean -Open
#>

param(
    [ValidateSet("debug", "release")]
    [string]$Config = "debug",

    [switch]$Clean,
    [switch]$Open
)

$ErrorActionPreference = "Stop"
$ProjectRoot = $PSScriptRoot

Write-Host "========================================" -ForegroundColor Cyan
Write-Host " CloudPhone_DuiLib_Demo Build Script" -ForegroundColor Cyan
Write-Host " Configuration: $Config" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan

# ---------------------------------------------------------
# Step 0: Initialize git submodules if needed
# ---------------------------------------------------------
if (-not (Test-Path (Join-Path $ProjectRoot "vcpkg\.git"))) {
    Write-Host "`n[0/3] Initializing git submodules..." -ForegroundColor Yellow
    Push-Location $ProjectRoot
    git submodule update --init --recursive
    Pop-Location
}

# ---------------------------------------------------------
# Step 1: Bootstrap vcpkg
# ---------------------------------------------------------
$vcpkgExe = Join-Path $ProjectRoot "vcpkg\vcpkg.exe"
if (-not (Test-Path $vcpkgExe)) {
    Write-Host "`n[1/3] Bootstrapping vcpkg..." -ForegroundColor Yellow
    Push-Location (Join-Path $ProjectRoot "vcpkg")
    & .\bootstrap-vcpkg.bat -disableMetrics
    Pop-Location
} else {
    Write-Host "`n[1/3] vcpkg already bootstrapped." -ForegroundColor Green
}

# ---------------------------------------------------------
# Step 2: CMake Configure
# ---------------------------------------------------------
$presetName = "windows-x64-$Config"
$buildDir = Join-Path $ProjectRoot "build\$Config"

if ($Clean -and (Test-Path $buildDir)) {
    Write-Host "Cleaning build directory: $buildDir" -ForegroundColor Yellow
    Remove-Item -Recurse -Force $buildDir
}

Write-Host "`n[2/3] Configuring CMake with preset: $presetName" -ForegroundColor Yellow
cmake --preset $presetName
if ($LASTEXITCODE -ne 0) {
    Write-Error "CMake configure failed!"
    exit 1
}

# ---------------------------------------------------------
# Step 3: CMake Build
# ---------------------------------------------------------
$cmakeConfig = if ($Config -eq "debug") { "Debug" } else { "Release" }
Write-Host "`n[3/3] Building ($cmakeConfig)..." -ForegroundColor Yellow
cmake --build $buildDir --config $cmakeConfig
if ($LASTEXITCODE -ne 0) {
    Write-Error "CMake build failed!"
    exit 1
}

# ---------------------------------------------------------
# Done
# ---------------------------------------------------------
$outputDir = Join-Path $buildDir $cmakeConfig
Write-Host "`n========================================" -ForegroundColor Green
Write-Host " Build succeeded!" -ForegroundColor Green
Write-Host " Output: $outputDir" -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Green

# Open VS solution if requested
if ($Open) {
    $sln = Get-ChildItem -Path $buildDir -Filter "*.sln" -Recurse | Select-Object -First 1
    if ($sln) {
        Write-Host "Opening solution: $($sln.FullName)" -ForegroundColor Cyan
        Start-Process $sln.FullName
    } else {
        Write-Warning "No .sln file found in $buildDir"
    }
}
