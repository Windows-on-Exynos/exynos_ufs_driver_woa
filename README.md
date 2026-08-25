# Exynos 9610 UFS for Windows on ARM

Boot-critical UFS support for the Motorola One Action (`troika`) and other
Exynos9610 devices using the same UFS integration.

## Architecture

The package uses Microsoft's inbox `storufs.sys` for the UFSHCI, UPIU, SCSI,
DMA, interrupt, and Storport data paths. `slsiufsstor.sys` is a boot-start WDM
lower filter that runs before StorUFS handles `IRP_MN_START_DEVICE` and performs
the Exynos-only platform initialization:

- UFS VCC, pinmux, reset, and MPHY isolation bypass
- FSYS I/O-coherency programming required by ACPI `_CCA = 1`
- Exynos vendor-HCI reset, clock, PRDT, nexus, and AXI setup
- the Samsung Exynos9610 PHY/PCS/UniPro calibration sequence
- link startup and post-link calibration

This design intentionally does not implement a second UFS/SCSI stack. The
earlier KMDF prototype remains in the tree as reference, but it is not compiled:
it did not register a Storport miniport or implement a Windows request queue and
therefore could not expose a disk to Windows Setup.

## Supported Windows versions

The minimum supported target is Windows 10 version 2004, build 19041, ARM64.
The INF uses the `NTarm64.10.0...19041` decoration and the source builds with
`NTDDI_WIN10_VB`.

## Firmware contract

The existing DSDT device is expected to contain:

- `_HID = "SLSI20A2"`
- `_CCA = 1`
- UFSHCI memory at `0x13520000`, length at least `0x200`
- ACPI interrupt GSIV `0xBD` (189), Level/ActiveHigh/Exclusive

The Linux Device Tree value is SPI 157. ACPI uses the GIC interrupt ID, so the
correct conversion is `157 + 32 = 189`. Do not change the DSDT interrupt to 157.

`slsiufsstor/acpi/UFS0-CLS.asl` is an additive SSDT that adds only:

```asl
Name (_CLS, Package () { 0x01, 0x09, 0x01 })
```

It must be loaded alongside the existing DSDT. It does not redefine
`\_SB.UFS0`, so it avoids the duplicate-device namespace failure caused by a
second complete UFS SSDT.

The firmware must leave the FSYS/UFS source clocks enabled. The existing
maestro9610 LK port also relies on the clocks configured by earlier Samsung boot
stages.

## Building

### Visual Studio/WDK 19041

Open `slsiufsstor/slsiufsstor.sln`, choose `Release|ARM64`, and build.

For a complete command-line build and package on Windows, run PowerShell as a
normal developer shell from the repository root:

```powershell
Set-ExecutionPolicy -Scope Process Bypass
.\build-windows.ps1
```

The script discovers Visual Studio and the installed WDK, prefers WDK
`10.0.19041.0`, builds `Release|ARM64`, runs Inf2Cat when available, compiles
the additive SSDT when `iasl.exe` is available, writes SHA-256 checksums, and
creates a ZIP below `out/windows/Release`.

Useful options:

```powershell
# Select an installed newer WDK while retaining Windows 10 2004 API targeting.
.\build-windows.ps1 -KitVersion 10.0.26100.0

# Provide ACPICA IASL explicitly.
.\build-windows.ps1 -IaslPath C:\Tools\iasl.exe

# Sign the SYS and catalog with a certificate already in the Windows store.
.\build-windows.ps1 -CertificateThumbprint YOUR_CERTIFICATE_THUMBPRINT
```

Required components are Visual Studio 2019 or newer with C++ build tools, the
Windows Driver Kit with ARM64 libraries, and PowerShell 5.1 or newer. IASL is
optional; without it the script stages the ASL source but does not emit AML.

### Linux cross-build

Use Clang plus the official `Microsoft.Windows.WDK.ARM64` NuGet package:

```sh
export WDK_ROOT=/path/to/wdk/c
export SDK_ROOT=/path/to/sdk-common/c
export CLANG_ROOT=/path/to/llvm/bin
./build-linux.sh
```

Output is written to `out/ARM64/Release`.

Compile the additive ACPI table with:

```sh
iasl -ve slsiufsstor/acpi/UFS0-CLS.asl
```

## WinPE and Windows Setup

The package is unsigned. Disable Secure Boot and use a test-signed catalog, or
use integrity-check bypass only for bring-up.

Add the driver to both WinPE and the target offline Windows image:

```bat
dism /Mount-Image /ImageFile:boot.wim /Index:1 /MountDir:C:\mount
dism /Image:C:\mount /Add-Driver /Driver:slsiufsstor.inf /ForceUnsigned
dism /Unmount-Image /MountDir:C:\mount /Commit

dism /Image:W:\ /Add-Driver /Driver:slsiufsstor.inf /ForceUnsigned
```

For an already running test PE:

```bat
drvload slsiufsstor.inf
diskpart
rescan
list disk
```

## First hardware validation

The INF starts conservatively at one lane, HS gear 1, series B. Capture serial
or kernel-debug output containing the `slsiufsstor:` prefix.

Useful PE checks:

```bat
pnputil /enum-devices /deviceid "ACPI\SLSI20A2" /drivers
reg query HKLM\SYSTEM\CurrentControlSet\Enum\ACPI\SLSI20A2 /s
sc query storufs
sc query slsiufsstor
diskpart
rescan
list disk
```

Do not increase the gear or lane count until repeated reads, writes, image
application, and reboot from UFS all succeed.

## Current validation boundary

The code compiles cleanly for native ARM64, the PE imports resolve against the
WDK, and the ACPI table compiles without warnings. Actual storage enumeration
and boot still require testing on the phone; no static build can prove PHY
electrical behavior or firmware clock state.
