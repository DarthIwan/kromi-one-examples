/* Hello, Kromi One: a program's first steps.  KROM starts it with a description of the machine; it says hello,
 * names the devices KROM offers it, and ends.  Builds with the Kromi One SDK (ABI 2.0, revision 17). */
#include "kromi.h"

static const char *family_name(uint16_t family)
{
    switch (family) {
    case KROM_ABI2_FAMILY_KCHIPSET_2D:
        return "the 2D chipset (drawing and the display)";
    case KROM_ABI2_FAMILY_KAUDIO:
        return "sound";
    case KROM_ABI2_FAMILY_KINPUT:
        return "keyboard and mouse";
    case KROM_ABI2_FAMILY_KBLOCK:
        return "storage";
    case KROM_ABI2_FAMILY_KNET:
        return "network";
    default:
        return "another device";
    }
}

uint32_t kromi_main(const krom_abi2_boot_info *boot)
{
    const uint8_t *table = (const uint8_t *)boot + boot->device_table_offset;

    kromi_say("Hello from the Kromi One.\n");
    kromi_say_number("Devices KROM offers this program: ", boot->device_count);
    for (uint32_t i = 0; i < boot->device_count; ++i) {
        const krom_abi2_device_info *device = (const void *)(table + (uint64_t)i * boot->device_entry_bytes);

        kromi_say("  ");
        kromi_say(family_name(device->family));
        kromi_say_number(", version 2.", device->abi_minor);
    }
    kromi_say("Goodbye.\n");
    return 0;
}
