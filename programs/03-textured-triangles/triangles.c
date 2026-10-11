/* A card turns in space, seen in perspective.  It is two triangles with a picture on them: the program gives the
 * chipset the card's four corners once, and each frame only a new 4 x 4 matrix, the card's turn and the camera in
 * one.  The chipset places the corners on the screen, divides by the depth and lays the picture over the triangles
 * with its perspective right.  Builds with the Kromi One SDK (ABI 2.0, revision 17). */
#include "kromi.h"

#define WIDTH 1920U
#define HEIGHT 1080U
#define TEXTURE 256U                    /* the picture on the card: 256 x 256 pixels */
#define FRAMES 600U                     /* ten seconds at 60 frames a second */
#define ONE 65536                       /* 1.0 in Q16.16, the triangles' numbers: 16 bits of whole, 16 of fraction */

/* the picture: eight by eight squares, light and dark, tinted red to the right and blue downwards, so its corners
 * tell which way the card faces */
static void draw_picture(uint32_t *pixels)
{
    for (uint32_t y = 0; y < TEXTURE; ++y) {
        for (uint32_t x = 0; x < TEXTURE; ++x) {
            const uint32_t light = (((x / 32U) + (y / 32U)) % 2U == 0U) ? 200U : 90U;
            const uint32_t red = light * (128U + x / 2U) / 256U, blue = light * (128U + y / 2U) / 256U;

            pixels[y * TEXTURE + x] = (red << 16) | ((light * 3U / 4U) << 8) | blue;
        }
    }
}

/* one corner: its place on the card (x, y; z 0) and the picture's pixel there (u, v), all Q16.16 */
static void corner(krom_abi2_k2d_triangle_vertex *at, int32_t x, int32_t y, int32_t u, int32_t v)
{
    at->x = x;
    at->y = y;
    at->z = 0;
    at->u = u * ONE;
    at->v = v * ONE;
    at->color = 0xFFFFFFFFU;            /* white: the picture's own colours */
}

uint32_t kromi_main(const krom_abi2_boot_info *boot)
{
    static struct kromi_2d q;
    static struct kromi_memory screen[2], picture, mesh;
    uint64_t target[2], texture;
    krom_abi2_k2d_triangle_vertex *corners;
    uint16_t *order;
    int64_t c = 1 << 30, s = 0;         /* the card's turn as its cosine and sine, Q2.30 */
    uint32_t status;

    if (!kromi_2d_has_triangles(boot)) {
        kromi_say("This machine's 2D chipset draws no triangles.\n");
        return 1;
    }
    if (((status = kromi_2d_open(&q, boot)) != KROM_ABI2_STATUS_OK)
        || ((status = kromi_2d_picture(&q, &screen[0], WIDTH, HEIGHT, KROM_ABI2_K2D_FORMAT_XRGB8888, &target[0]))
            != KROM_ABI2_STATUS_OK)
        || ((status = kromi_2d_picture(&q, &screen[1], WIDTH, HEIGHT, KROM_ABI2_K2D_FORMAT_XRGB8888, &target[1]))
            != KROM_ABI2_STATUS_OK)
        || ((status = kromi_2d_picture(&q, &picture, TEXTURE, TEXTURE, KROM_ABI2_K2D_FORMAT_XRGB8888, &texture))
            != KROM_ABI2_STATUS_OK)
        || ((status = kromi_memory(&mesh, 4096, KROM_ABI2_OBJECT_USAGE_GENERAL, true)) != KROM_ABI2_STATUS_OK)) {
        kromi_say_number("Could not set up the 2D chipset, status ", status);
        return 2;
    }
    draw_picture(picture.at);
    if (kromi_hand_over(&picture) != KROM_ABI2_STATUS_OK) {    /* the CPU drew it; the chipset reads it */
        return 3;
    }

    /* the card: four corners 1.6 wide and 1.2 high around its middle, and two triangles over them, three corner
     * numbers each */
    corners = mesh.at;
    corner(&corners[0], -ONE * 8 / 10, -ONE * 6 / 10, 0, 0);
    corner(&corners[1], ONE * 8 / 10, -ONE * 6 / 10, (int32_t)TEXTURE, 0);
    corner(&corners[2], ONE * 8 / 10, ONE * 6 / 10, (int32_t)TEXTURE, (int32_t)TEXTURE);
    corner(&corners[3], -ONE * 8 / 10, ONE * 6 / 10, 0, (int32_t)TEXTURE);
    order = (uint16_t *)(void *)((uint8_t *)mesh.at + 256U);
    order[0] = 0;
    order[1] = 1;
    order[2] = 2;
    order[3] = 0;
    order[4] = 2;
    order[5] = 3;

    for (uint32_t frame = 0; frame < FRAMES; ++frame) {
        const uint64_t into = target[frame & 1U];            /* the picture not on screen */
        krom_abi2_k2d_command *clear = kromi_2d_command(&q, KROM_ABI2_K2D_OPCODE_FILL);
        krom_abi2_k2d_triangles *card = kromi_2d_triangles(&q);
        krom_abi2_k2d_command *show = kromi_2d_command(&q, KROM_ABI2_K2D_OPCODE_PRESENT);
        /* the camera: 900 pixels' focal length, the screen's middle at (960, 540), the card 2.5 units away.  A corner
         * (x, y, 0) turned about the upright axis lands at x' = c x, z' = 2.5 - s x; on the screen at
         * (960 + 900 x' / z', 540 + 900 y / z').  The matrix holds that before the division: the rows X, Y and W. */
        const int64_t cos16 = c >> 14, sin16 = s >> 14;     /* Q16.16 */

        clear->destination_surface = into;                   /* the whole picture, night blue */
        clear->width = WIDTH;
        clear->height = HEIGHT;
        clear->color = 0x0014121CU;

        card->destination_surface = into;                    /* drawn anywhere on the screen */
        card->width = WIDTH;
        card->height = HEIGHT;
        card->source_surface = texture;                      /* the whole picture on it, smoothly sampled */
        card->source_width = TEXTURE;
        card->source_height = TEXTURE;
        card->flags = (uint32_t)KROM_ABI2_K2D_SAMPLING_BILINEAR;
        card->composite = (uint32_t)KROM_ABI2_K2D_COMPOSITE_COPY;
        card->m00 = (int32_t)(900 * cos16 - 960 * sin16);    /* X = 900 x' + 960 z' */
        card->m03 = 960 * ONE * 5 / 2;
        card->m10 = (int32_t)(-540 * sin16);                 /* Y = 900 y + 540 z' */
        card->m11 = 900 * ONE;
        card->m13 = 540 * ONE * 5 / 2;
        card->m30 = (int32_t)-sin16;                         /* W = z' */
        card->m33 = ONE * 5 / 2;
        card->vertex_object = mesh.object.object_handle;     /* the card's corners and triangles */
        card->vertex_count = 4;
        card->vertex_stride = sizeof(krom_abi2_k2d_triangle_vertex);
        card->index_object = mesh.object.object_handle;
        card->index_offset = 256;
        card->count = 2;

        show->destination_surface = into;                    /* on screen at the next frame */
        show->present_id = frame + 1U;
        if ((status = kromi_2d_run(&q)) != KROM_ABI2_STATUS_OK) {
            kromi_say_number("A frame failed, status ", status);
            return 4;
        }
        /* turn the card by one degree: (c, s) rotated by cos 1 degree, sin 1 degree (Q2.30) */
        {
            const int64_t step_c = 1073578039, step_s = 18739328;
            const int64_t next_c = (c * step_c - s * step_s) >> 30;

            s = (s * step_c + c * step_s) >> 30;
            c = next_c;
        }
    }
    kromi_say_number("Frames shown: ", FRAMES);
    return 0;
}
