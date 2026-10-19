[CmdletBinding()]
param(
    [ValidateSet('Debug','Release')][string]$Configuration = 'Debug',
    [string]$Dx8SdkDir = '',
    [string]$DataDir = '',
    [switch]$ConfigureOnly,
    [switch]$Clean,
    [switch]$SkipInstall
)
# Do not grow PATH or leak VsDevCmd variables into the caller on repeated builds.
$originalEnvironment = @{}
Get-ChildItem Env: | ForEach-Object { $originalEnvironment[$_.Name] = $_.Value }
try {
    $ErrorActionPreference = 'Stop'
    $repoRoot = Split-Path $PSScriptRoot -Parent
    $locator = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    if (!(Test-Path $locator)) { throw 'Install Visual Studio 2022 Build Tools with C++ x86/x64, MFC, and Windows SDK.' }
    $vsRoot = & $locator -latest -version '[17.0,18.0)' -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 Microsoft.VisualStudio.Component.VC.ATLMFC -property installationPath
    if (!$vsRoot) { throw 'VS 2022 C++ tools and MFC are required.' }
    $devCmd = Join-Path $vsRoot 'Common7\Tools\VsDevCmd.bat'
    $environmentLines = & cmd /d /s /c "`"$devCmd`" -arch=x86 -host_arch=x64 >nul && set"
    if ($LASTEXITCODE) { throw 'Could not initialize the x86 SDK environment.' }
    foreach ($line in $environmentLines) {
        if ($line -match '^([^=]+)=(.*)$') { [Environment]::SetEnvironmentVariable($matches[1], $matches[2], 'Process') }
    }
    $cmakeRoot = Join-Path $vsRoot 'Common7\IDE\CommonExtensions\Microsoft\CMake'
    $llvmBin = Join-Path $env:ProgramFiles 'LLVM\bin'
    $env:Path = "$llvmBin;$cmakeRoot\CMake\bin;$cmakeRoot\Ninja;$env:Path"
    foreach ($tool in 'clang-cl','cmake','ninja','link','rc') {
        if (!(Get-Command $tool -ErrorAction SilentlyContinue)) { throw "Missing $tool. Install LLVM and the C++ CMake tools/Windows SDK components." }
    }
    $preset = 'windows-x86-clang-' + $Configuration.ToLowerInvariant()
    Push-Location $repoRoot
    try {
        $configureArgs = @('--preset', $preset)
        if ($Dx8SdkDir) { $configureArgs += "-DFL_DX8_SDK_DIR=$Dx8SdkDir" }
        if ($DataDir) { $configureArgs += "-DFL_DATA_DIR=$DataDir" }
        & cmake @configureArgs
        if ($LASTEXITCODE) { throw 'CMake configuration failed.' }
        if ($ConfigureOnly) { return }
        $buildArgs = @('--build', '--preset', $preset)
        if ($Clean) { $buildArgs += '--clean-first' }
        & cmake @buildArgs
        if ($LASTEXITCODE) { throw 'Build failed.' }
        & ctest --test-dir "out/build/$preset" --output-on-failure
        if ($LASTEXITCODE) { throw 'Build-contract tests failed.' }
        if (!$SkipInstall) {
            & cmake --install "out/build/$preset"
            if ($LASTEXITCODE) { throw 'Runtime staging failed.' }
        }
    } finally { Pop-Location }
} finally {
    Get-ChildItem Env: | Where-Object { !$originalEnvironment.ContainsKey($_.Name) } | ForEach-Object {
        [Environment]::SetEnvironmentVariable($_.Name, $null, 'Process')
    }
    foreach ($entry in $originalEnvironment.GetEnumerator()) {
        [Environment]::SetEnvironmentVariable($entry.Key, $entry.Value, 'Process')
    }
}
