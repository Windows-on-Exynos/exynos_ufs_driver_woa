/*
 * Minimal replacement DSDT for the Motorola One Action (troika).
 *
 * This fixes the UFS resource contract from Mu-troika.img.  In particular,
 * the Exynos vendor and PMA registers through 0x13524fff are part of the
 * controller window, and the UniPro block is declared separately.
 *
 * Do not add the old TSC1 node here: it references a missing \_SB.I2C4.
 */
DefinitionBlock ("", "DSDT", 2, "SAMSUN", "S5E9610 ", 0x00000004)
{
    Scope (\_SB)
    {
        Device (UFS0)
        {
            Method (_STA, 0, NotSerialized)
            {
                Return (0x0F)
            }

            Name (_HID, "SLSI20A2")
            Name (_UID, Zero)
            Name (_CCA, One)
            Name (_CLS, Package (0x03)
            {
                0x01,
                0x09,
                0x01
            })

            Name (_CRS, ResourceTemplate ()
            {
                Memory32Fixed (ReadWrite, 0x13520000, 0x00005000)
                Memory32Fixed (ReadWrite, 0x13510000, 0x00008000)
                Interrupt (ResourceConsumer, Level, ActiveHigh, Exclusive)
                {
                    0x000000BD
                }
            })

            Device (DEV0)
            {
                Method (_ADR, 0, NotSerialized)
                {
                    Return (0x08)
                }

                Method (_RMV, 0, NotSerialized)
                {
                    Return (Zero)
                }
            }
        }

        Device (CPU0) { Name (_HID, "ACPI0007") Name (_UID, Zero) Method (_STA, 0, NotSerialized) { Return (0x0F) } }
        Device (CPU1) { Name (_HID, "ACPI0007") Name (_UID, One)  Method (_STA, 0, NotSerialized) { Return (0x0F) } }
        Device (CPU2) { Name (_HID, "ACPI0007") Name (_UID, 0x02) Method (_STA, 0, NotSerialized) { Return (0x0F) } }
        Device (CPU3) { Name (_HID, "ACPI0007") Name (_UID, 0x03) Method (_STA, 0, NotSerialized) { Return (0x0F) } }
        Device (CPU4) { Name (_HID, "ACPI0007") Name (_UID, 0x04) Method (_STA, 0, NotSerialized) { Return (0x0F) } }
        Device (CPU5) { Name (_HID, "ACPI0007") Name (_UID, 0x05) Method (_STA, 0, NotSerialized) { Return (0x0F) } }
        Device (CPU6) { Name (_HID, "ACPI0007") Name (_UID, 0x06) Method (_STA, 0, NotSerialized) { Return (0x0F) } }
        Device (CPU7) { Name (_HID, "ACPI0007") Name (_UID, 0x07) Method (_STA, 0, NotSerialized) { Return (0x0F) } }
    }
}
