# Automated Build and Packaging Script for FinTrack
# This script configures environment variables, builds the application in Release mode,
# runs windeployqt, and packages everything into an installer using Inno Setup.

Write-Host "=== Starting FinTrack Build and Packaging Process ===" -ForegroundColor Cyan

# 1. Setup paths to Qt6, CMake, Ninja, and MinGW
$QtBin = "C:\Qt\6.10.2\mingw_64\bin"
$CMakeBin = "C:\Qt\Tools\CMake_64\bin"
$NinjaBin = "C:\Qt\Tools\Ninja"
$MinGwBin = "C:\Qt\Tools\mingw1310_64\bin"

if (-not (Test-Path $QtBin)) { Write-Error "Qt bin directory not found at $QtBin"; exit 1 }
if (-not (Test-Path $CMakeBin)) { Write-Error "CMake bin directory not found at $CMakeBin"; exit 1 }
if (-not (Test-Path $NinjaBin)) { Write-Error "Ninja directory not found at $NinjaBin"; exit 1 }
if (-not (Test-Path $MinGwBin)) { Write-Error "MinGW bin directory not found at $MinGwBin"; exit 1 }

# Prepend paths to environment PATH
$env:PATH = "$QtBin;$CMakeBin;$NinjaBin;$MinGwBin;" + $env:PATH
Write-Host "Paths successfully configured." -ForegroundColor Green

# 2. Cleanup old build directories
$BuildDir = "build_release"
$DistDir = "dist"
$SetupDir = "dist_setup"

Write-Host "Cleaning up old build folders..." -ForegroundColor Yellow
if (Test-Path $BuildDir) { Remove-Item -Recurse -Force $BuildDir }
if (Test-Path $DistDir) { Remove-Item -Recurse -Force $DistDir }
if (Test-Path $SetupDir) { Remove-Item -Recurse -Force $SetupDir }

New-Item -ItemType Directory -Path $BuildDir | Out-Null
New-Item -ItemType Directory -Path $DistDir | Out-Null
New-Item -ItemType Directory -Path $SetupDir | Out-Null

# 3. Configure and compile release build using CMake and Ninja
Write-Host "Configuring build using CMake and Ninja..." -ForegroundColor Yellow
cmake -S . -B $BuildDir -G "Ninja" -DCMAKE_BUILD_TYPE=Release
if ($LASTEXITCODE -ne 0) { Write-Error "CMake configuration failed."; exit 1 }

Write-Host "Building application in Release mode..." -ForegroundColor Yellow
cmake --build $BuildDir
if ($LASTEXITCODE -ne 0) { Write-Error "Compilation failed."; exit 1 }

# 4. Locate and copy EXE to dist directory
$ExeFile = Get-ChildItem -Path $BuildDir -Filter "FinTrack.exe" -Recurse | Select-Object -First 1
if (-not $ExeFile) {
    Write-Error "Could not find FinTrack.exe. Check compiler errors."
    exit 1
}

Write-Host "Copying FinTrack.exe to dist directory..." -ForegroundColor Yellow
Copy-Item $ExeFile.FullName -Destination "$DistDir\FinTrack.exe"

# 5. Run windeployqt to copy Qt6 DLLs and dependencies
Write-Host "Running windeployqt to deploy Qt dependencies..." -ForegroundColor Yellow
& "$QtBin\windeployqt.exe" --release --compiler-runtime "$DistDir\FinTrack.exe"
if ($LASTEXITCODE -ne 0) { Write-Error "windeployqt failed."; exit 1 }

# 6. Locate Inno Setup compiler (ISCC.exe) and compile installer
Write-Host "Locating Inno Setup (ISCC.exe)..." -ForegroundColor Yellow
$Iscc = "ISCC.exe"
if (-not (Get-Command $Iscc -ErrorAction SilentlyContinue)) {
    $PotentialPaths = @(
        "C:\Program Files (x86)\Inno Setup 6\ISCC.exe",
        "C:\Program Files\Inno Setup 6\ISCC.exe",
        "$env:LocalAppData\Programs\Inno Setup 6\ISCC.exe"
    )
    foreach ($Path in $PotentialPaths) {
        if (Test-Path $Path) {
            $Iscc = $Path
            break
        }
    }
}

if (-not (Get-Command $Iscc -ErrorAction SilentlyContinue) -and -not (Test-Path $Iscc)) {
    Write-Warning "Inno Setup compiler (ISCC.exe) could not be found."
    Write-Warning "Please verify that Inno Setup is installed. You can compile 'setup.iss' manually once installed."
    Write-Host "`n--- Packaging completed WITHOUT installer generation ---" -ForegroundColor Yellow
    Write-Host "The standalone folder is ready at: $DistDir" -ForegroundColor Green
    exit 0
}

Write-Host "Compiling installer using Inno Setup (ISCC)..." -ForegroundColor Yellow
& $Iscc "setup.iss"
if ($LASTEXITCODE -ne 0) { Write-Error "Inno Setup compilation failed."; exit 1 }

Write-Host "`n=== Build and Packaging Completed Successfully! ===" -ForegroundColor Green
Write-Host "Standalone distribution folder: $DistDir" -ForegroundColor Cyan
Write-Host "Installer executable file: $SetupDir\FinTrack_Setup.exe" -ForegroundColor Cyan
