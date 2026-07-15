<#
.SYNOPSIS
    Runs CloudPhone_DuiLib_Demo.exe and captures crash stack traces.
.DESCRIPTION
    This script:
    1. Enables Windows Error Reporting (WER) local crash dumps
    2. Runs the target executable
    3. On crash, locates the generated .dmp file
    4. Uses cdb.exe (Debugging Tools for Windows) to extract the call stack
    5. Saves the stack trace to a text file for review
.PARAMETER ExePath
    Path to the executable. Default: build/debug/Debug/CloudPhone_DuiLib_Demo.exe
.PARAMETER BuildDir
    Root build directory (contains PDB symbols). Default: build/debug/Debug
.PARAMETER OutputDir
    Where to save crash analysis results. Default: crashes
.EXAMPLE
    .\crash_runner.ps1
    .\crash_runner.ps1 -ExePath "D:\...\CloudPhone_DuiLib_Demo.exe"
#>

param(
    [string]$ExePath = "",
    [string]$BuildDir = "",
    [string]$OutputDir = ""
)

$ErrorActionPreference = "Stop"
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path

if (-not $ExePath) {
    $ExePath = Join-Path $ScriptDir "build\debug\Debug\CloudPhone_DuiLib_Demo.exe"
}
if (-not $BuildDir) {
    $BuildDir = Join-Path $ScriptDir "build\debug\Debug"
}
if (-not $OutputDir) {
    $OutputDir = Join-Path $ScriptDir "crashes"
}

$ExePath = [System.IO.Path]::GetFullPath($ExePath)
$BuildDir = [System.IO.Path]::GetFullPath($BuildDir)
$OutputDir = [System.IO.Path]::GetFullPath($OutputDir)
$ExeName = [System.IO.Path]::GetFileNameWithoutExtension($ExePath)

# Symbol path: build output dir + Microsoft symbol server
$SymbolPath = "srv*$BuildDir*https://msdl.microsoft.com/download/symbols"

Write-Host "========================================" -ForegroundColor Cyan
Write-Host " CloudPhone Crash Capture Script" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "  Exe   : $ExePath"
Write-Host "  Sym   : $BuildDir"
Write-Host "  Output: $OutputDir"
Write-Host ""

# --- Step 1: Verify files exist ---
if (-not (Test-Path $ExePath)) {
    Write-Error "Executable not found: $ExePath"
    exit 1
}
if (-not (Test-Path "$BuildDir\$ExeName.pdb")) {
    Write-Warning "PDB not found at: $BuildDir\$ExeName.pdb"
    Write-Warning "Stack traces may lack function names and line numbers."
}

# --- Step 2: Create output directory ---
if (-not (Test-Path $OutputDir)) {
    New-Item -ItemType Directory -Path $OutputDir -Force | Out-Null
}

# --- Step 3: Find cdb.exe (Debugging Tools for Windows) ---
$cdbPaths = @()

# Check Windows SDK via registry (VS 2022 / Build Tools)
try {
    $kitRoot = Get-ItemPropertyValue -Path "HKLM:\SOFTWARE\Microsoft\Windows Kits\Installed Roots" -Name "KitsRoot10" -ErrorAction SilentlyContinue
    if ($kitRoot) {
        $cdbPaths += Join-Path $kitRoot "Debuggers\x64\cdb.exe"
    }
}
catch { }

# Fallback paths
$cdbPaths += "C:\Program Files (x86)\Windows Kits\10\Debuggers\x64\cdb.exe"
$cdbPaths += "C:\Program Files\Windows Kits\10\Debuggers\x64\cdb.exe"
$cdbPaths += "C:\Program Files (x86)\Microsoft Visual Studio\2022\Community\Common7\IDE\Remote Debugger\x64\cdb.exe"

$cdbExe = $null
foreach ($p in $cdbPaths) {
    if (Test-Path $p) {
        $cdbExe = $p
        break
    }
}

if (-not $cdbExe) {
    Write-Warning "cdb.exe not found. Stack trace analysis will be skipped."
    Write-Warning "Install 'Debugging Tools for Windows' from the Windows SDK."
    Write-Warning "Dump files will still be collected."
}

# --- Step 4: Enable WER LocalDumps ---
Write-Host "[1/4] Enabling Windows Error Reporting local dumps..." -ForegroundColor Yellow

$dumpRegPath = "HKCU:\SOFTWARE\Microsoft\Windows\Windows Error Reporting\LocalDumps"
if (-not (Test-Path $dumpRegPath)) {
    New-Item -Path $dumpRegPath -Force | Out-Null
}

# Set system-wide dump settings
Set-ItemProperty -Path $dumpRegPath -Name "DumpFolder" -Value $OutputDir -Type String -Force
Set-ItemProperty -Path $dumpRegPath -Name "DumpCount" -Value 10 -Type DWord -Force
Set-ItemProperty -Path $dumpRegPath -Name "DumpType" -Value 2 -Type DWord -Force  # 2 = Full dump
Set-ItemProperty -Path $dumpRegPath -Name "CustomDumpFlags" -Value 0 -Type DWord -Force

# Also set per-application key for more targeted capture
$appRegPath = "$dumpRegPath\$ExeName"
if (-not (Test-Path $appRegPath)) {
    New-Item -Path $appRegPath -Force | Out-Null
}
Set-ItemProperty -Path $appRegPath -Name "DumpFolder" -Value $OutputDir -Type String -Force
Set-ItemProperty -Path $appRegPath -Name "DumpCount" -Value 10 -Type DWord -Force
Set-ItemProperty -Path $appRegPath -Name "DumpType" -Value 2 -Type DWord -Force

Write-Host "  Dump folder: $OutputDir" -ForegroundColor Gray
Write-Host "  Dump type  : Full dump (2)" -ForegroundColor Gray

# --- Step 5: Record existing dump files to detect new ones ---
$existingDumps = @()
if (Test-Path $OutputDir) {
    $existingDumps = Get-ChildItem -Path $OutputDir -Filter "$ExeName*.dmp" -ErrorAction SilentlyContinue | Select-Object -ExpandProperty FullName
}

# --- Step 6: Run the executable ---
Write-Host "[2/4] Launching $ExeName..." -ForegroundColor Yellow
Write-Host "  Working dir: $([System.IO.Path]::GetDirectoryName($ExePath))" -ForegroundColor Gray
Write-Host ""

$startTime = Get-Date
$process = Start-Process -FilePath $ExePath `
    -WorkingDirectory ([System.IO.Path]::GetDirectoryName($ExePath)) `
    -PassThru

# Wait for the process to exit
$process.WaitForExit()
$exitCode = $process.ExitCode
$runDuration = (Get-Date) - $startTime

Write-Host ""
Write-Host "[3/4] Process exited" -ForegroundColor Yellow
Write-Host "  Exit code   : $exitCode (0x$('{0:X8}' -f $exitCode))" -ForegroundColor $(if ($exitCode -eq 0) { "Green" } else { "Red" })
Write-Host "  Run duration: $($runDuration.ToString('hh\:mm\:ss'))" -ForegroundColor Gray

# --- Step 7: Check for new crash dumps ---
Write-Host "[4/4] Checking for crash dumps..." -ForegroundColor Yellow

# Wait a moment for WER to finish writing the dump
Start-Sleep -Seconds 2

$newDumps = @()
if (Test-Path $OutputDir) {
    $allDumps = Get-ChildItem -Path $OutputDir -Filter "$ExeName*.dmp" -ErrorAction SilentlyContinue | Select-Object -ExpandProperty FullName
    $newDumps = $allDumps | Where-Object { $_ -notin $existingDumps }
}

# Also check the default WER crash dumps location
$werDefaultPath = "$env:LOCALAPPDATA\CrashDumps"
if (Test-Path $werDefaultPath) {
    $werDumps = Get-ChildItem -Path $werDefaultPath -Filter "$ExeName*.dmp" -ErrorAction SilentlyContinue |
        Where-Object { $_.LastWriteTime -gt $startTime } |
        Select-Object -ExpandProperty FullName
    $newDumps += $werDumps
}

if ($newDumps.Count -eq 0) {
    Write-Host "  No crash dump found." -ForegroundColor Gray
    if ($exitCode -ne 0) {
        Write-Host "  Non-zero exit code but no dump was generated." -ForegroundColor Yellow
        Write-Host "  The process may have exited cleanly with an error code." -ForegroundColor Yellow
    }
    else {
        Write-Host "  Process exited normally (exit code 0)." -ForegroundColor Green
    }
}
else {
    # Pick the most recent dump
    $latestDump = $newDumps | Sort-Object { (Get-Item $_).LastWriteTime } -Descending | Select-Object -First 1
    $dumpSizeMB = [math]::Round((Get-Item $latestDump).Length / 1MB, 1)

    Write-Host ""
    Write-Host "========================================" -ForegroundColor Red
    Write-Host "  CRASH DETECTED" -ForegroundColor Red
    Write-Host "  Dump file: $latestDump" -ForegroundColor Red
    Write-Host "  Dump size: $dumpSizeMB MB" -ForegroundColor Red
    Write-Host "========================================" -ForegroundColor Red
    Write-Host ""

    # --- Step 8: Analyze dump with cdb ---
    if ($cdbExe -and (Test-Path $latestDump)) {
        Write-Host "Analyzing crash dump with cdb..." -ForegroundColor Yellow
        Write-Host "  cdb path: $cdbExe" -ForegroundColor Gray
        Write-Host ""

        $stackLog = Join-Path $OutputDir "$ExeName_crash_stack_$(Get-Date -Format 'yyyyMMdd_HHmmss').txt"

        # Simple, fast cdb commands:
        #   .ecxr  -> switch to exception context (the actual crash point)
        #   k 50   -> stack trace of current thread (50 frames)
        #   lm     -> list loaded modules
        #   q      -> quit
        # Using -y for symbol path (local only, no Microsoft server) to avoid hanging
        $cdbCommands = ".reload; .ecxr; k 50; lm; q"

        $cdbArgs = @(
            "-z", $latestDump,
            "-y", $BuildDir,          # Local symbols only, no network
            "-c", $cdbCommands,
            "-logo", $stackLog,
            "-lines"
        )

        Write-Host "  Running cdb analysis (should finish in < 30 seconds)..." -ForegroundColor Gray

        $cdbProc = Start-Process -FilePath $cdbExe `
            -ArgumentList $cdbArgs `
            -NoNewWindow `
            -Wait `
            -PassThru

        Write-Host ""
        Write-Host "========================================" -ForegroundColor Green
        Write-Host "  Stack trace saved to:" -ForegroundColor Green
        Write-Host "  $stackLog" -ForegroundColor Green
        Write-Host "========================================" -ForegroundColor Green
        Write-Host ""

        # --- Step 9: Display key parts of the analysis ---
        if (Test-Path $stackLog) {
            Write-Host "--- Crash Stack Trace ---" -ForegroundColor Cyan
            Write-Host ""

            # Output the complete file
            Get-Content $stackLog | ForEach-Object {
                if ($_ -match '^Child-SP|Call Site|ntdll!Rtl|TcrSdk!|CloudPhone_DuiLib_Demo!|quit:') {
                    Write-Host $_ -ForegroundColor White
                } elseif ($_ -match 'WARNING:|error |Error ') {
                    Write-Host $_ -ForegroundColor Yellow
                } else {
                    Write-Host $_ -ForegroundColor Gray
                }
            }

            Write-Host ""
            Write-Host "Full analysis saved to: $stackLog" -ForegroundColor Gray
            Write-Host ""
            Write-Host "To run detailed analysis manually:" -ForegroundColor Gray
            Write-Host "  $cdbExe -z $latestDump -y $BuildDir -c `".ecxr; k 50; q`" -lines" -ForegroundColor Gray
        }
    }
    elseif ($latestDump) {
        $dumpCopy = Join-Path $OutputDir "$ExeName_crash_$(Get-Date -Format 'yyyyMMdd_HHmmss').dmp"
        Copy-Item $latestDump $dumpCopy -Force
        Write-Host "  Dump copied to: $dumpCopy" -ForegroundColor Gray
        Write-Host "  To analyze manually, open in WinDbg:" -ForegroundColor Gray
        Write-Host "    File -> Open Crash Dump -> $dumpCopy" -ForegroundColor Gray
        Write-Host "    Then run: .sympath srv*$BuildDir*https://msdl.microsoft.com/download/symbols" -ForegroundColor Gray
        Write-Host "    Then run: .reload; !analyze -v" -ForegroundColor Gray
    }
}

Write-Host ""
Write-Host "Done." -ForegroundColor Cyan
