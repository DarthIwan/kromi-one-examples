# Kromi One programs

Short programs for the Kromi One, one idea each, written to be read.  Each builds with the Kromi One SDK (ABI 2.0,
revision 17) and runs on the machine's firmware, KROM, as any program does: KROM starts it, hands it a description of
the machine and answers its calls.  A program never touches the hardware; it asks KROM for memory and for a queue to a
device, and KROM checks every command before the chipset runs it.

| Example | What it shows |
|---|---|
| [01-hello](01-hello/hello.c) | A program's start and end: KROM's description of the machine, the devices it offers, text out |
| [02-moving-sprite](02-moving-sprite/sprite.c) | The 2D chipset drawing sixty frames a second: a picture cleared, a ball drawn over it, shown; two pictures taking turns so nothing tears |
| [03-textured-triangles](03-textured-triangles/triangles.c) | 2.5D: a card of two textured triangles turning in perspective; the program sends one 4 × 4 matrix a frame, the chipset places the corners, divides by depth and lays the picture on with its perspective right |

[common/kromi.h](common/kromi.h) holds the few helpers they share: text out, the program's end, memory from KROM, and a
queue to the 2D chipset with its commands, triangles included.

![02-moving-sprite in KROM's QEMU runtime](sprite-qemu.png)

![03-textured-triangles in KROM's QEMU runtime: the card mid-turn](triangles-qemu.png)

**Building them:** the Kromi One SDK isn't public yet, so these programs don't build outside it for now. They include the SDK's headers by name (`krom/abi2/...`) and are written to be read: what programming the Kromi One looks like. All three were checked against the SDK's ABI 2.0, revision 17, in KROM's QEMU runtime: `01-hello` lists the devices and ends with status 0; `02-moving-sprite` and `03-textured-triangles` each show 600 frames and end with status 0.

MIT licence, see [LICENSE](LICENSE).
