param(
    [string]$QtRoot = "D:\Qt\6.11.1\msvc2022_64"
)

$ErrorActionPreference = "Stop"

$ProjectRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$BuildDirectory = Join-Path $ProjectRoot "build-release"
$DistRoot = [System.IO.Path]::GetFullPath((Join-Path $ProjectRoot "dist"))
$PackageDirectory = [System.IO.Path]::GetFullPath((Join-Path $DistRoot "2048Game-Windows-x64"))
$Archive = Join-Path $DistRoot "2048Game-Windows-x64.zip"
$VisualStudioTools = "D:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat"
$CMake = "D:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
$Ninja = "D:\ninja\ninja.exe"
$DeployQt = Join-Path $QtRoot "bin\windeployqt.exe"

if (-not $PackageDirectory.StartsWith($DistRoot + [System.IO.Path]::DirectorySeparatorChar,
        [System.StringComparison]::OrdinalIgnoreCase)) {
    throw "Package directory is outside the project dist directory: $PackageDirectory"
}
foreach ($RequiredFile in @($VisualStudioTools, $CMake, $Ninja, $DeployQt)) {
    if (-not (Test-Path -LiteralPath $RequiredFile)) {
        throw "Required packaging tool not found: $RequiredFile"
    }
}

$BuildCommand = 'call "{0}" -arch=x64 -host_arch=x64 >nul && "{1}" -S "{2}" -B "{3}" -G Ninja -DCMAKE_MAKE_PROGRAM="{4}" -DCMAKE_PREFIX_PATH="{5}" -DCMAKE_BUILD_TYPE=Release && "{1}" --build "{3}"' -f `
    $VisualStudioTools, $CMake, $ProjectRoot, $BuildDirectory, $Ninja, $QtRoot
& $env:ComSpec /d /s /c $BuildCommand
if ($LASTEXITCODE -ne 0) {
    throw "Release build failed with exit code $LASTEXITCODE"
}

if (Test-Path -LiteralPath $PackageDirectory) {
    Remove-Item -LiteralPath $PackageDirectory -Recurse -Force
}
New-Item -ItemType Directory -Path (Join-Path $PackageDirectory "Data\DLC") -Force | Out-Null
Copy-Item -LiteralPath (Join-Path $BuildDirectory "bin\2048_game_qt.exe") -Destination $PackageDirectory
Copy-Item -LiteralPath (Join-Path $ProjectRoot "README.md") -Destination $PackageDirectory
Copy-Item -Path (Join-Path $ProjectRoot "DLC\*.json") -Destination (Join-Path $PackageDirectory "Data\DLC") -Force

$DeployCommand = 'call "{0}" -arch=x64 -host_arch=x64 >nul && "{1}" --release --compiler-runtime --no-translations "{2}"' -f `
    $VisualStudioTools, $DeployQt, (Join-Path $PackageDirectory "2048_game_qt.exe")
& $env:ComSpec /d /s /c $DeployCommand
if ($LASTEXITCODE -ne 0) {
    throw "Qt deployment failed with exit code $LASTEXITCODE"
}

$CrtRoot = Get-ChildItem -LiteralPath "D:\Program Files\Microsoft Visual Studio\2022\Community\VC\Redist\MSVC" -Directory |
    Sort-Object Name -Descending |
    ForEach-Object { Join-Path $_.FullName "x64\Microsoft.VC143.CRT" } |
    Where-Object { Test-Path -LiteralPath $_ } |
    Select-Object -First 1
if (-not $CrtRoot) {
    throw "MSVC x64 redistributable runtime was not found."
}
Copy-Item -Path (Join-Path $CrtRoot "*") -Destination $PackageDirectory -Force

$RedistributableInstaller = Join-Path $PackageDirectory "vc_redist.x64.exe"
if (Test-Path -LiteralPath $RedistributableInstaller) {
    Remove-Item -LiteralPath $RedistributableInstaller -Force
}
if (Test-Path -LiteralPath $Archive) {
    Remove-Item -LiteralPath $Archive -Force
}
Compress-Archive -LiteralPath $PackageDirectory -DestinationPath $Archive -CompressionLevel Optimal

Write-Host "Portable directory: $PackageDirectory" -ForegroundColor Green
Write-Host "Portable archive: $Archive" -ForegroundColor Green
