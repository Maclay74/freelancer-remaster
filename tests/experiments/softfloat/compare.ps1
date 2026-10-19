[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
$originalEnvironment = @{}
Get-ChildItem Env: | ForEach-Object { $originalEnvironment[$_.Name] = $_.Value }
try {
    $repoRoot = Split-Path (Split-Path (Split-Path $PSScriptRoot -Parent) -Parent) -Parent
    $output = Join-Path $repoRoot 'out\experiments\softfloat'
    New-Item -ItemType Directory -Force -Path $output | Out-Null
    $dependency = Join-Path $output 'dependency'
    $commit = 'a0c6494cdc11865811dec815d5c0049fba9d82a8'
    if (!(Test-Path -LiteralPath $dependency)) {
        & git clone --no-checkout --filter=blob:none https://github.com/ucb-bar/berkeley-softfloat-3.git $dependency
        if ($LASTEXITCODE) { throw 'SoftFloat checkout failed' }
        & git -C $dependency fetch --depth 1 origin $commit
        if ($LASTEXITCODE) { throw 'Pinned SoftFloat fetch failed' }
        & git -C $dependency checkout --detach $commit
        if ($LASTEXITCODE) { throw 'Pinned SoftFloat checkout failed' }
    }
    $actualCommit = & git -C $dependency rev-parse HEAD
    if ($LASTEXITCODE -or $actualCommit -ne $commit) { throw "Expected pinned SoftFloat commit $commit at $dependency" }
    $changes = & git -C $dependency status --porcelain
    if ($LASTEXITCODE -or $changes) { throw "SoftFloat checkout has local changes; preserve them before rerunning: $dependency" }

    # Resolve only the routines used by this prototype, preserving the upstream
    # files and notices. 8086 selects x87 NaN rules, not SSE NaN rules.
    $mapping = @{}
    foreach ($directory in @((Join-Path $dependency 'source'), (Join-Path $dependency 'source\8086'))) {
        foreach ($file in Get-ChildItem -LiteralPath $directory -Filter '*.c') {
            $name = $file.BaseName -replace '^s_', 'softfloat_'
            $mapping[$name] = $file.FullName
        }
    }
    $todo = [Collections.Generic.Stack[string]]::new()
    foreach ($name in 'f32_to_extF80', 'extF80_add', 'extF80_sub', 'extF80_mul', 'extF80_div', 'extF80_sqrt',
        'extF80_to_f32', 'softfloat_raiseFlags', 'softfloat_countLeadingZeros8', 'softfloat_approxRecip_1Ks', 'softfloat_approxRecipSqrt_1Ks') { $todo.Push($name) }
    $selected = @{}
    while ($todo.Count) {
        $name = $todo.Pop()
        if ($selected.ContainsKey($name) -or !$mapping.ContainsKey($name)) { continue }
        $path = $mapping[$name]
        $selected[$name] = $path
        foreach ($match in [regex]::Matches((Get-Content -LiteralPath $path -Raw), '\b(softfloat_\w+|extF80_\w+|f32_\w+)\s*\(')) {
            if (!$selected.ContainsKey($match.Groups[1].Value)) { $todo.Push($match.Groups[1].Value) }
        }
    }
    $selected['state'] = Join-Path $dependency 'source\softfloat_state.c'
    $includes = foreach ($path in ($selected.Values | Sort-Object)) { '#include "' + ($path -replace '\\', '/') + '"' }
    Set-Content -LiteralPath (Join-Path $output 'softfloat-sources.h') -Value $includes

    $locator = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    $vsRoot = & $locator -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    if (!$vsRoot) { throw 'Install Visual Studio C++ x86 tools and a Windows SDK.' }
    $devCmd = Join-Path $vsRoot 'Common7\Tools\VsDevCmd.bat'
    $lines = & cmd /d /s /c "`"$devCmd`" -arch=x86 -host_arch=x64 >nul && set"
    if ($LASTEXITCODE) { throw 'Could not prepare the x86 environment.' }
    foreach ($line in $lines) {
        if ($line -match '^([^=]+)=(.*)$') { [Environment]::SetEnvironmentVariable($Matches[1], $Matches[2], 'Process') }
    }
    $results = Join-Path $output 'results.txt'
    Set-Content -LiteralPath $results -Value "SoftFloat numerical prototype; upstream commit $commit"
    foreach ($compilerName in 'clang', 'msvc') {
        $compiler = if ($compilerName -eq 'clang') { Join-Path $env:ProgramFiles 'LLVM\bin\clang-cl.exe' } else { (Get-Command cl.exe).Source }
        foreach ($configuration in 'debug', 'release') {
            $name = "$compilerName-$configuration"
            $exe = Join-Path $output "$name.exe"
            $options = @('/nologo', '/std:c++17', '/EHsc', '/MD', '/arch:IA32', '/fp:precise',
                "/I$PSScriptRoot", "/I$output", "/I$dependency/source/8086", "/I$dependency/source/include",
                "/Fo$output/$name.obj", "/Fe$exe")
            $options += if ($configuration -eq 'debug') { '/Od' } else { '/O2' }
            if ($compilerName -eq 'clang') { $options += @('--target=i686-pc-windows-msvc', '/clang:-ffp-model=precise') }
            & $compiler @options (Join-Path $PSScriptRoot 'arithmetic.cpp') '/link' '/INCREMENTAL:NO'
            if ($LASTEXITCODE) { throw "Prototype compilation failed: $name" }
            "`n=== $name ===" | Tee-Object -FilePath $results -Append
            & $exe | Tee-Object -FilePath $results -Append
            if ($LASTEXITCODE) { throw "Prototype numerical comparison failed: $name" }
        }
    }
    Write-Host "Results: $results"
}
finally {
    Get-ChildItem Env: | Where-Object { !$originalEnvironment.ContainsKey($_.Name) } | ForEach-Object { [Environment]::SetEnvironmentVariable($_.Name, $null, 'Process') }
    foreach ($entry in $originalEnvironment.GetEnumerator()) { [Environment]::SetEnvironmentVariable($entry.Key, $entry.Value, 'Process') }
}
