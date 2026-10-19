[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
$version = '23.1.3'
$clang = Join-Path $env:ProgramFiles 'LLVM\bin\clang-cl.exe'
$expected = "clang version $([regex]::Escape($version))(?:\s|$)"
if (Test-Path -LiteralPath $clang) {
    $installed = & $clang --version
    if (($installed -join ' ') -match $expected) {
        Write-Host "LLVM $version is already available."
        return
    }
}
if ($env:GITHUB_ACTIONS -ne 'true') {
    throw "This installer is for ephemeral GitHub Actions runners. Install LLVM $version locally."
}

# Pinned official release asset and its GitHub-published sha256 digest.
# https://github.com/llvm/llvm-project/releases/tag/llvmorg-23.1.3
$url = "https://github.com/llvm/llvm-project/releases/download/llvmorg-$version/LLVM-$version-win64.msi"
$sha256 = '5153f848ac87553118340f16f8d3cd4351d729bcba6e4785c6a4a9af29676de9'
$package = Join-Path $env:RUNNER_TEMP "LLVM-$version-win64.msi"
$installLog = Join-Path $env:RUNNER_TEMP 'llvm-install.log'
Invoke-WebRequest -Uri $url -OutFile $package -MaximumRetryCount 3 -RetryIntervalSec 10
if ((Get-FileHash -LiteralPath $package -Algorithm SHA256).Hash -ne $sha256) {
    throw 'LLVM release archive checksum mismatch.'
}
$process = Start-Process -FilePath msiexec.exe -WindowStyle Hidden -Wait -PassThru -ArgumentList @(
    '/i', "`"$package`"", '/qn', '/norestart', '/l*v', "`"$installLog`"")
if ($process.ExitCode -notin @(0, 3010)) {
    throw "LLVM installation failed with code $($process.ExitCode). See $installLog."
}
$installed = & $clang --version
if ($LASTEXITCODE -or ($installed -join ' ') -notmatch $expected) {
    throw "LLVM $version was not installed at the expected path: $clang"
}
Write-Host "Installed LLVM $version from its pinned official release."
