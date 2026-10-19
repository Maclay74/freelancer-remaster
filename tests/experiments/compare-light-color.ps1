[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
$originalEnvironment = @{}
Get-ChildItem Env: | ForEach-Object { $originalEnvironment[$_.Name] = $_.Value }
try {
    $repoRoot = Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
    $locator = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    $vsRoot = & $locator -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    if (!$vsRoot) { throw 'Install Visual Studio C++ x86 tools and a Windows SDK.' }
    $devCmd = Join-Path $vsRoot 'Common7\Tools\VsDevCmd.bat'
    $environmentLines = & cmd /d /s /c "`"$devCmd`" -arch=x86 -host_arch=x64 >nul && set"
    if ($LASTEXITCODE) { throw 'Could not prepare the x86 environment.' }
    foreach ($line in $environmentLines) {
        if ($line -match '^([^=]+)=(.*)$') { [Environment]::SetEnvironmentVariable($Matches[1], $Matches[2], 'Process') }
    }
    $source = Join-Path $PSScriptRoot 'light_color_comparison.cpp'
    $output = Join-Path $repoRoot 'out\experiments\light-color'
    New-Item -ItemType Directory -Force -Path $output | Out-Null
    $results = Join-Path $output 'results.txt'
    Set-Content -LiteralPath $results -Value 'Standalone light-color comparison (no game build).'
    foreach ($compilerName in 'clang', 'msvc') {
        $compiler = if ($compilerName -eq 'clang') { Join-Path $env:ProgramFiles 'LLVM\bin\clang-cl.exe' } else { (Get-Command cl.exe).Source }
        foreach ($configuration in 'debug', 'release') {
            $name = "$compilerName-$configuration"
            $exe = Join-Path $output "$name.exe"
            $object = Join-Path $output "$name.obj"
            $options = @('/nologo', '/std:c++17', '/EHsc', '/MD', '/W4', '/arch:IA32', '/fp:precise', "/Fo$object", "/Fe$exe")
            $options += if ($configuration -eq 'debug') { '/Od' } else { '/O2' }
            if ($compilerName -eq 'clang') { $options += @('--target=i686-pc-windows-msvc', '/clang:-ffp-model=precise') }
            Write-Host "Compiling only light_color_comparison.cpp: $name"
            & $compiler @options $source '/link' '/INCREMENTAL:NO'
            if ($LASTEXITCODE) { throw "Standalone compilation failed: $name" }
            "`n=== $name ===" | Tee-Object -FilePath $results -Append
            & $exe | Tee-Object -FilePath $results -Append
            if ($LASTEXITCODE) { throw "Standalone comparison failed: $name" }
        }
    }
    Write-Host "Results: $results"
}
finally {
    Get-ChildItem Env: | Where-Object { !$originalEnvironment.ContainsKey($_.Name) } | ForEach-Object { [Environment]::SetEnvironmentVariable($_.Name, $null, 'Process') }
    foreach ($entry in $originalEnvironment.GetEnumerator()) { [Environment]::SetEnvironmentVariable($entry.Key, $entry.Value, 'Process') }
}
