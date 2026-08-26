[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$MountDirectory,
    [string]$DriverPackageDirectory = "$PSScriptRoot\out\windows\Release\package",
    [switch]$ForceUnsigned
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$mountRoot = [IO.Path]::GetFullPath($MountDirectory)
$packageRoot = [IO.Path]::GetFullPath($DriverPackageDirectory)
$system32 = Join-Path $mountRoot "Windows\System32"
$inf = Join-Path $packageRoot "slsiufsstor.inf"
$driver = Join-Path $packageRoot "slsiufsstor.sys"
$sourceRoot = Join-Path $PSScriptRoot "winpe"

foreach ($requiredFile in @(
    (Join-Path $system32 "cmd.exe"),
    $inf,
    $driver,
    (Join-Path $sourceRoot "startnet.cmd"),
    (Join-Path $sourceRoot "ufs-smoke-test.cmd")
)) {
    if (-not (Test-Path -LiteralPath $requiredFile -PathType Leaf)) {
        throw "Required file was not found: $requiredFile"
    }
}

$dism = Get-Command dism.exe -ErrorAction Stop
$arguments = @(
    "/Image:$mountRoot",
    "/Add-Driver",
    "/Driver:$inf"
)
if ($ForceUnsigned) {
    $arguments += "/ForceUnsigned"
}

Write-Host "Injecting the ARM64 UFS package into $mountRoot"
& $dism.Source @arguments
if ($LASTEXITCODE -ne 0) {
    throw "DISM failed with exit code $LASTEXITCODE"
}

$startnet = Join-Path $system32 "startnet.cmd"
$backup = Join-Path $system32 "startnet.before-ufs-smoke-test.cmd"
if ((Test-Path -LiteralPath $startnet -PathType Leaf) -and
    -not (Test-Path -LiteralPath $backup)) {
    Copy-Item -LiteralPath $startnet -Destination $backup
}

Copy-Item -LiteralPath (Join-Path $sourceRoot "startnet.cmd") `
    -Destination $startnet -Force
Copy-Item -LiteralPath (Join-Path $sourceRoot "ufs-smoke-test.cmd") `
    -Destination $system32 -Force

Write-Host "WinPE smoke test installed." -ForegroundColor Green
Write-Host "Review the DISM result, then unmount the image with /Commit."
