/* Small helpers the Kromi One examples share: text out, the program's end, memory, and a queue to the 2D chipset.
 * Builds with the Kromi One SDK (ABI 2.0, revision 17).
 *
 * A Kromi One program runs on its own: KROM, the machine's firmware, starts it, hands it a description of the machine
 * (the boot information) and answers its calls.  The program never touches the hardware; it asks KROM for memory and
 * for a queue to a device, writes commands into the queue, and KROM checks each one before the chipset runs it. */
#ifndef KROMI_H
#define KROMI_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "krom/abi2/layouts.h"
#include "krom/abi2/system.h"

/* ---- text and the end ---- */

void kromi_write(const char *text, uint64_t bytes);     /* KROM's diagnostic stream */
_Noreturn void kromi_exit(uint32_t status);             /* the program's end, with its status */
void kromi_say(const char *text);                       /* a zero-terminated text */
void kromi_say_number(const char *label, uint64_t value);   /* label, the number in decimal, a new line */

/* Each example's own start: KROM's description of the machine.  Its status is the program's (0: all went well). */
uint32_t kromi_main(const krom_abi2_boot_info *boot);

/* The machine's device of `family` (KROM_ABI2_FAMILY_...), or none. */
const krom_abi2_device_info *kromi_device(const krom_abi2_boot_info *boot, uint64_t family);

/* ---- memory ---- */

/* A piece of memory from KROM, mapped for the program.  `coherent`: the chipset sees the CPU's writes at once
 * (queues, commands); otherwise the program hands its writes over with kromi_hand_over (pictures). */
struct kromi_memory {
    krom_abi2_object_result object;
    krom_abi2_mapping cpu;
    void *at;
};

uint32_t kromi_memory(struct kromi_memory *memory, uint64_t bytes, uint32_t usage, bool coherent);
uint32_t kromi_hand_over(const struct kromi_memory *memory);   /* the CPU's writes, for the chipset */

/* ---- the 2D chipset ---- */

/* A queue to the 2D chipset (Klara draws, Dora shows): commands are written into `commands`, then run together. */
struct kromi_2d {
    krom_abi2_lease lease;
    krom_abi2_queue_result queue;
    krom_abi2_queue_shared *shared;
    struct kromi_memory ring, commands;
    krom_abi2_queue_submission batch[32];
    uint32_t count;                     /* commands in this batch */
    uint64_t produced, consumed, next_id;
};

uint32_t kromi_2d_open(struct kromi_2d *q, const krom_abi2_boot_info *boot);

/* A picture the chipset may draw into (and, `on_screen`, show): its memory, its handle for commands. */
uint32_t kromi_2d_picture(struct kromi_2d *q, struct kromi_memory *pixels, uint32_t width, uint32_t height,
                          uint16_t format, uint64_t *picture);

/* The next command of the batch, zeroed, its kind set; null when the batch is full (run it first). */
krom_abi2_k2d_command *kromi_2d_command(struct kromi_2d *q, uint16_t opcode);

/* Whether the machine's 2D chipset draws textured triangles (DRAW_TRIANGLES). */
bool kromi_2d_has_triangles(const krom_abi2_boot_info *boot);

/* The next command of the batch as textured triangles, zeroed, its kind set; null when the batch is full. */
krom_abi2_k2d_triangles *kromi_2d_triangles(struct kromi_2d *q);

/* The batch run: handed to KROM, then waited for until the chipset finished every command (a PRESENT finishes when
 * its picture is on screen, so a frame loop runs at the display's rate). */
uint32_t kromi_2d_run(struct kromi_2d *q);

#endif
