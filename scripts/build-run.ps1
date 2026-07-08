param(
    [string]$QtRoot = "D:\Qt\6.11.1\msvc2022_64"
)

$ErrorActionPreference = "Stop"

$ProjectRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$BuildDirectory = Join-Path $ProjectRoot "build-debug"
$VisualStudioTools = "D:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat"
$CMake = "D:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
$Ninja = "D:\ninja\ninja.exe"
$Executable = Join-Path $BuildDirectory "bin\2048_game_qt.exe"

if (-not (Test-Path -LiteralPath $VisualStudioTools)) {
    throw "Visual Studio developer tools not found: $VisualStudioTools"
}
if (-not (Test-Path -LiteralPath $CMake)) {
    throw "CMake not found: $CMake"
}
if (-not (Test-Path -LiteralPath $QtRoot)) {
    throw "Qt not found: $QtRoot"
}

$ExecutablePath = [System.IO.Path]::GetFullPath($Executable)
Get-CimInstance Win32_Process -Filter "Name = '2048_game_qt.exe'" -ErrorAction SilentlyContinue |
    Where-Object { $_.ExecutablePath -eq $ExecutablePath } |
    ForEach-Object { Stop-Process -Id $_.ProcessId -Force }

$Command = 'call "{0}" -arch=x64 -host_arch=x64 >nul && "{1}" -S "{2}" -B "{3}" -G Ninja -DCMAKE_MAKE_PROGRAM="{4}" -DCMAKE_PREFIX_PATH="{5}" -DCMAKE_BUILD_TYPE=Debug && "{1}" --build "{3}"' -f `
    $VisualStudioTools, $CMake, $ProjectRoot, $BuildDirectory, $Ninja, $QtRoot

& $env:ComSpec /d /s /c $Command
if ($LASTEXITCODE -ne 0) {
    throw "Build failed with exit code $LASTEXITCODE"
}

$env:Path = (Join-Path $QtRoot "bin") + ";" + $env:Path
Start-Process -FilePath $Executable -WorkingDirectory (Split-Path $Executable)
Write-Host "Build completed and application started: $Executable" -ForegroundColor Green
