[CmdletBinding()]
param(
    [ValidateSet("Debug", "Release")]
    [string]$Configuration = "Release",
    [string]$KitVersion,
    [string]$OutDir,
    [string]$IaslPath,
    [switch]$SkipCatalog,
    [string]$CertificateThumbprint,
    [string]$TimestampUrl = "http://timestamp.digicert.com"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Write-Step {
    param([string]$Message)
    Write-Host ""
    Write-Host "==> $Message" -ForegroundColor Cyan
}

function Resolve-WindowsKitVersion {
    param([string]$KitsRoot, [string]$RequestedVersion)

    $includeRoot = Join-Path $KitsRoot "Include"
    if (-not (Test-Path -LiteralPath $includeRoot -PathType Container)) {
        throw "Windows 10 SDK/WDK Include directory was not found: $includeRoot"
    }

    $versions = Get-ChildItem -LiteralPath $includeRoot -Directory |
        Where-Object {
            Test-Path -LiteralPath (Join-Path $_.FullName "km") -PathType Container
        } |
        ForEach-Object {
            try {
                [PSCustomObject]@{
                    Text = $_.Name
                    Version = [version]$_.Name
                }
            }
            catch {
                $null
            }
        } |
        Sort-Object Version -Descending

    if (-not $versions) {
        throw "No WDK kernel headers were found below $includeRoot"
    }

    if ($RequestedVersion) {
        $match = $versions | Where-Object { $_.Text -eq $RequestedVersion } |
            Select-Object -First 1
        if (-not $match) {
            $available = ($versions.Text -join ", ")
            throw "WDK $RequestedVersion is not installed. Available: $available"
        }
        return $match.Text
    }

    $win10Vb = $versions | Where-Object { $_.Text -eq "10.0.19041.0" } |
        Select-Object -First 1
    if ($win10Vb) {
        return $win10Vb.Text
    }

    Write-Warning (("WDK 10.0.19041.0 is not installed; using {0}. " +
        "The source and INF still target Windows 10 2004 as the minimum.") -f
        $versions[0].Text)
    return $versions[0].Text
}

function Resolve-VsWhere {
    $programFilesX86 = [Environment]::GetFolderPath("ProgramFilesX86")
    $candidate = Join-Path $programFilesX86 "Microsoft Visual Studio\Installer\vswhere.exe"
    if (Test-Path -LiteralPath $candidate -PathType Leaf) {
        return $candidate
    }

    $command = Get-Command "vswhere.exe" -ErrorAction SilentlyContinue
    if ($command) {
        return $command.Source
    }

    throw "vswhere.exe was not found. Install Visual Studio 2019+ or Build Tools."
}

function Resolve-MsBuild {
    $vswhere = Resolve-VsWhere
    $installPath = & $vswhere -latest -products * -requires Microsoft.Component.MSBuild -property installationPath |
        Select-Object -First 1

    if (-not $installPath) {
        throw "No Visual Studio installation containing MSBuild was found."
    }

    $candidate = Join-Path $installPath "MSBuild\Current\Bin\MSBuild.exe"
    if (-not (Test-Path -LiteralPath $candidate -PathType Leaf)) {
        throw "MSBuild was not found at $candidate"
    }

    return $candidate
}

function Resolve-KitTool {
    param([string]$KitsRoot, [string]$Version, [string]$Name)

    $candidates = @(
        (Join-Path $KitsRoot "bin\$Version\x64\$Name"),
        (Join-Path $KitsRoot "bin\$Version\x86\$Name"),
        (Join-Path $KitsRoot "bin\x64\$Name"),
        (Join-Path $KitsRoot "bin\x86\$Name")
    )

    foreach ($candidate in $candidates) {
        if (Test-Path -LiteralPath $candidate -PathType Leaf) {
            return $candidate
        }
    }

    return $null
}

function Resolve-Iasl {
    param([string]$RequestedPath, [string]$RepositoryRoot)

    if ($RequestedPath) {
        $resolved = Resolve-Path -LiteralPath $RequestedPath -ErrorAction Stop
        return $resolved.Path
    }

    $repoTool = Join-Path $RepositoryRoot "tools\iasl.exe"
    if (Test-Path -LiteralPath $repoTool -PathType Leaf) {
        return $repoTool
    }

    $command = Get-Command "iasl.exe" -ErrorAction SilentlyContinue
    if ($command) {
        return $command.Source
    }

    return $null
}

$repoRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$project = Join-Path $repoRoot "slsiufsstor\slsiufsstor.vcxproj"
$inf = Join-Path $repoRoot "slsiufsstor\slsiufsstor.inf"
$acpiSourceDir = Join-Path $repoRoot "slsiufsstor\acpi"
$aslSources = @(
    (Join-Path $acpiSourceDir "DSDT-troika.asl"),
    (Join-Path $acpiSourceDir "UFS0-CLS.asl")
)
$readme = Join-Path $repoRoot "README.md"

foreach ($requiredFile in @($project, $inf, $readme) + $aslSources) {
    if (-not (Test-Path -LiteralPath $requiredFile -PathType Leaf)) {
        throw "Required source file was not found: $requiredFile"
    }
}

$programFilesX86 = [Environment]::GetFolderPath("ProgramFilesX86")
$kitsRoot = Join-Path $programFilesX86 "Windows Kits\10"
$selectedKit = Resolve-WindowsKitVersion -KitsRoot $kitsRoot -RequestedVersion $KitVersion
$msbuild = Resolve-MsBuild

if (-not $OutDir) {
    $OutDir = Join-Path $repoRoot "out\windows\$Configuration"
}
$outRoot = [IO.Path]::GetFullPath($OutDir)
$binaryDir = Join-Path $outRoot "bin"
$objectDir = Join-Path $outRoot "obj"
$packageDir = Join-Path $outRoot "package"

if (Test-Path -LiteralPath $packageDir -PathType Container) {
    Remove-Item -LiteralPath $packageDir -Recurse -Force
}
New-Item -ItemType Directory -Force -Path $binaryDir, $objectDir, $packageDir |
    Out-Null

Write-Step "Building $Configuration|ARM64 with WDK $selectedKit"
$msbuildArguments = @(
    $project,
    "/nologo",
    "/m",
    "/t:Rebuild",
    "/p:Configuration=$Configuration",
    "/p:Platform=ARM64",
    "/p:WindowsTargetPlatformVersion=$selectedKit",
    "/p:OutDir=$binaryDir\",
    "/p:IntDir=$objectDir\",
    "/p:EnableInf2Cat=false",
    "/p:SignMode=Off"
)

& $msbuild @msbuildArguments
if ($LASTEXITCODE -ne 0) {
    throw "MSBuild failed with exit code $LASTEXITCODE"
}

$driver = Join-Path $binaryDir "slsiufsstor.sys"
if (-not (Test-Path -LiteralPath $driver -PathType Leaf)) {
    $driver = Get-ChildItem -LiteralPath $outRoot -Filter "slsiufsstor.sys" -File -Recurse |
        Sort-Object LastWriteTimeUtc -Descending |
        Select-Object -First 1 -ExpandProperty FullName
}
if (-not $driver -or -not (Test-Path -LiteralPath $driver -PathType Leaf)) {
    throw "MSBuild succeeded but slsiufsstor.sys was not found below $outRoot"
}

Write-Step "Staging the driver package"
Copy-Item -LiteralPath $driver -Destination $packageDir -Force
Copy-Item -LiteralPath $inf -Destination $packageDir -Force
Copy-Item -LiteralPath $aslSources -Destination $packageDir -Force
Copy-Item -LiteralPath $readme -Destination $packageDir -Force

$iasl = Resolve-Iasl -RequestedPath $IaslPath -RepositoryRoot $repoRoot
if ($iasl) {
    Write-Step "Compiling the ACPI tables"
    foreach ($asl in $aslSources) {
        $amlPrefix = Join-Path $packageDir ([IO.Path]::GetFileNameWithoutExtension($asl))
        & $iasl -ve -p $amlPrefix $asl
        if ($LASTEXITCODE -ne 0) {
            throw "IASL failed for $asl with exit code $LASTEXITCODE"
        }
    }
}
else {
    Write-Warning "iasl.exe was not found; ACPI sources were staged without AML."
}

if (-not $SkipCatalog) {
    $inf2Cat = Resolve-KitTool -KitsRoot $kitsRoot -Version $selectedKit -Name "Inf2Cat.exe"
    if ($inf2Cat) {
        Write-Step "Generating the Windows 10 2004 ARM64 catalog"
        & $inf2Cat "/driver:$packageDir" "/os:10_VB_ARM64" /uselocaltime
        if ($LASTEXITCODE -ne 0) {
            throw "Inf2Cat failed with exit code $LASTEXITCODE"
        }
    }
    else {
        Write-Warning "Inf2Cat.exe was not found; the package remains unsigned."
    }
}

if ($CertificateThumbprint) {
    $signTool = Resolve-KitTool -KitsRoot $kitsRoot -Version $selectedKit -Name "signtool.exe"
    if (-not $signTool) {
        throw "A certificate was requested, but signtool.exe was not found."
    }

    $catalog = Join-Path $packageDir "slsiufsstor.cat"
    if (-not (Test-Path -LiteralPath $catalog -PathType Leaf)) {
        throw "A signing certificate was supplied, but Inf2Cat did not produce slsiufsstor.cat."
    }
    $filesToSign = @((Join-Path $packageDir "slsiufsstor.sys"), $catalog)

    Write-Step "Signing the driver package"
    foreach ($file in $filesToSign) {
        & $signTool sign /sha1 $CertificateThumbprint /fd SHA256 /tr $TimestampUrl /td SHA256 $file
        if ($LASTEXITCODE -ne 0) {
            throw "SignTool failed for $file with exit code $LASTEXITCODE"
        }
    }
}

Write-Step "Writing checksums and archive"
$checksumFile = Join-Path $packageDir "SHA256SUMS.txt"
$checksumLines = Get-ChildItem -LiteralPath $packageDir -File |
    Where-Object { $_.Name -ne "SHA256SUMS.txt" } |
    Sort-Object Name |
    ForEach-Object {
        $hash = Get-FileHash -Algorithm SHA256 -LiteralPath $_.FullName
        "{0}  {1}" -f $hash.Hash, $_.Name
    }
[IO.File]::WriteAllLines($checksumFile, $checksumLines)

$archive = Join-Path $outRoot ("exynos9610-ufs-woa-{0}-ARM64.zip" -f $Configuration)
if (Test-Path -LiteralPath $archive -PathType Leaf) {
    Remove-Item -LiteralPath $archive -Force
}
Compress-Archive -Path (Join-Path $packageDir "*") -DestinationPath $archive -CompressionLevel Optimal

Write-Host ""
Write-Host "Build completed successfully." -ForegroundColor Green
Write-Host "Driver : $(Join-Path $packageDir 'slsiufsstor.sys')"
Write-Host "Package: $packageDir"
Write-Host "Archive: $archive"
