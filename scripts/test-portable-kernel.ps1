[CmdletBinding()]
param([Parameter(Mandatory)][ValidatePattern('^[a-z0-9_-]+$')][string]$Test,
      [switch]$ContinueOnFailure)
$ErrorActionPreference = 'Stop'
$originalEnvironment = @{}
Get-ChildItem Env: | ForEach-Object { $originalEnvironment[$_.Name] = $_.Value }
try {
    $repoRoot = Split-Path $PSScriptRoot -Parent
    $source = Join-Path $repoRoot "tests\portability\$Test.cpp"
    if (!(Test-Path -LiteralPath $source)) { throw "Unknown kernel test: $source" }
    $locator = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    $vsRoot = & $locator -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    if (!$vsRoot) { throw 'Install Visual Studio C++ x86 tools and a Windows SDK.' }
    $devCmd = Join-Path $vsRoot 'Common7\Tools\VsDevCmd.bat'
    $lines = & cmd /d /s /c "`"$devCmd`" -arch=x86 -host_arch=x64 >nul && set"
    if ($LASTEXITCODE) { throw 'Could not prepare the x86 environment.' }
    foreach ($line in $lines) {
        if ($line -match '^([^=]+)=(.*)$') { [Environment]::SetEnvironmentVariable($Matches[1], $Matches[2], 'Process') }
    }
    $output = Join-Path $repoRoot "out\tests\portability\$Test"
    New-Item -ItemType Directory -Force -Path $output | Out-Null
    $results = Join-Path $output 'results.txt'
    Set-Content -LiteralPath $results -Value "Isolated kernel: $Test"
    $comparisonFailures = 0
    foreach ($compilerName in 'clang', 'msvc') {
        $compiler = if ($compilerName -eq 'clang') { Join-Path $env:ProgramFiles 'LLVM\bin\clang-cl.exe' } else { (Get-Command cl.exe).Source }
        foreach ($configuration in 'debug', 'release') {
            $name = "$compilerName-$configuration"
            $exe = Join-Path $output "$name.exe"
            $object = Join-Path $output "$name.obj"
            $options = @('/nologo', '/std:c++17', '/EHsc', '/MD', '/W4', '/arch:IA32', '/fp:precise', "/Fo$object", "/Fe$exe")
            $options += if ($configuration -eq 'debug') { '/Od' } else { '/O2' }
            if ($compilerName -eq 'clang') { $options += @('--target=i686-pc-windows-msvc', '/clang:-ffp-model=precise') }
            Write-Host "Compiling only $Test.cpp: $name"
            & $compiler @options $source '/link' '/INCREMENTAL:NO'
            if ($LASTEXITCODE) { throw "Kernel compilation failed: $name" }
            "`n=== $name ===" | Tee-Object -FilePath $results -Append
            & $exe | Tee-Object -FilePath $results -Append
            if ($LASTEXITCODE) {
                ++$comparisonFailures
                if (!$ContinueOnFailure) { throw "Kernel comparison failed: $name" }
                Write-Warning "Kernel comparison failed: $name; collecting the remaining configurations."
            }
        }
    }
    Write-Host "Results: $results"
    if ($comparisonFailures) { throw "$comparisonFailures compiler/configuration comparisons failed." }
}
finally {
    Get-ChildItem Env: | Where-Object { !$originalEnvironment.ContainsKey($_.Name) } | ForEach-Object { [Environment]::SetEnvironmentVariable($_.Name, $null, 'Process') }
    foreach ($entry in $originalEnvironment.GetEnumerator()) { [Environment]::SetEnvironmentVariable($entry.Key, $entry.Value, 'Process') }
}
