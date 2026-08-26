# Troika UEFI and ACPI contract

`Mu-troika.img` contains a valid Project Mu/EDK2 firmware volume and includes
`UfsDxe`. Its embedded DSDT describes `UFS0` as `SLSI20A2`, with GSIV 189, but
the memory resource is only `0x13520000..0x135210ff`. That range excludes the
Exynos vendor registers beginning at `0x13521100`, PMA registers at
`0x13524000`, and the separate UniPro block at `0x13510000`.

`slsiufsstor/acpi/DSDT-troika.asl` is the replacement DSDT for a new firmware
build. It makes these material corrections:

- adds UFS class code `01/09/01`, allowing the inbox `storufs.inf` match;
- expands the primary window to `0x13520000`, length `0x5000`;
- declares UniPro at `0x13510000`, length `0x8000`;
- keeps interrupt GSIV `0xBD` (Linux SPI 157 plus the GIC offset of 32);
- omits the old `TSC1` object because it refers to an undefined `\_SB.I2C4`.

The additive `UFS0-CLS.asl` remains useful only as a compatibility experiment.
It supplies `_CLS`, but cannot correct the undersized DSDT resources. The
current driver deliberately rejects that old resource layout.

Replace the DSDT in the UEFI source and rebuild the firmware; do not concatenate
the replacement DSDT with the existing one. The FADT reset register and the
nonstandard MADT GICC entry observed in this particular image should also be
reviewed in the firmware source, but are outside this driver package and have
not been binary-patched here.
