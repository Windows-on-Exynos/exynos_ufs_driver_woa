/*
 * Additive SSDT for the existing minimal DSDT.
 *
 * The DSDT already defines \_SB.UFS0 with _HID SLSI20A2, the UFSHCI MMIO
 * resource, and GSIV 0xBD. Do not define a second UFS0 device. This table only
 * supplies the UFS class code used by the Windows inbox storufs.inf fallback.
 * It does not repair the original 0x1100 _CRS window. Use DSDT-troika.asl
 * when building a corrected firmware image for this driver.
 */

DefinitionBlock ("", "SSDT", 2, "WOEXYN", "UFS0CLS", 0x00000001)
{
    External (\_SB.UFS0, DeviceObj)

    Scope (\_SB.UFS0)
    {
        Name (_CLS, Package (0x03)
        {
            0x01, // Mass storage controller
            0x09, // Universal Flash Storage
            0x01  // UFSHCI programming interface
        })
    }
}
