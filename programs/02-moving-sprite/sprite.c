/* A ball bounces across the screen.  The program only describes each frame; the 2D chipset clears the picture, draws
 * the ball into it and shows it, sixty times a second.  Two pictures take turns: the chipset draws into the one not
 * on screen, so nothing tears.  Builds with the Kromi One SDK (ABI 2.0, revision 17). */
#include "kromi.h"

#define WIDTH 1920U
#define HEIGHT 1080U
#define BALL 128U
#define FRAMES 600U                     /* ten seconds at 60 frames a second */

/* the ball: a disc whose edge fades out over four pixels, yellow at the top, orange below, its colour premultiplied
 * by its coverage (what SOURCE_OVER expects) */
static void draw_ball(uint32_t *pixels)
{
    const int32_t r = (int32_t)BALL / 2 - 3, inner = (r - 2) * (r - 2), outer = (r + 2) * (r + 2);

    for (uint32_t y = 0; y < BALL; ++y) {
        for (uint32_t x = 0; x < BALL; ++x) {
            const int32_t dx = (int32_t)x - (int32_t)BALL / 2, dy = (int32_t)y - (int32_t)BALL / 2;
            const int32_t d2 = dx * dx + dy * dy;
            const uint32_t a = (d2 <= inner) ? 255U
                             : (d2 >= outer) ? 0U : (uint32_t)(255 * (outer - d2) / (outer - inner));
            const uint32_t red = (a * 0xF9U) / 255U, green = (a * (0x7DU + (uint32_t)(64 - dy) / 2U)) / 255U;

            pixels[y * BALL + x] = (a << 24) | (red << 16) | (green << 8) | ((a * 0x05U) / 255U);
        }
    }
}

uint32_t kromi_main(const krom_abi2_boot_info *boot)
{
    static struct kromi_2d q;
    static struct kromi_memory screen[2], ball;
    uint64_t picture[2], ball_picture;
    int32_t x = 100, y = 100, step_x = 9, step_y = 6;
    uint32_t status;

    if (((status = kromi_2d_open(&q, boot)) != KROM_ABI2_STATUS_OK)
        || ((status = kromi_2d_picture(&q, &screen[0], WIDTH, HEIGHT, KROM_ABI2_K2D_FORMAT_XRGB8888, &picture[0]))
            != KROM_ABI2_STATUS_OK)
        || ((status = kromi_2d_picture(&q, &screen[1], WIDTH, HEIGHT, KROM_ABI2_K2D_FORMAT_XRGB8888, &picture[1]))
            != KROM_ABI2_STATUS_OK)
        || ((status = kromi_2d_picture(&q, &ball, BALL, BALL, KROM_ABI2_K2D_FORMAT_ARGB8888_PREMULT, &ball_picture))
            != KROM_ABI2_STATUS_OK)) {
        kromi_say_number("Could not set up the 2D chipset, status ", status);
        return 1;
    }
    draw_ball(ball.at);
    if (kromi_hand_over(&ball) != KROM_ABI2_STATUS_OK) {   /* the CPU drew it; the chipset reads it */
        return 2;
    }
    for (uint32_t frame = 0; frame < FRAMES; ++frame) {
        const uint64_t target = picture[frame & 1U];        /* the picture not on screen */
        krom_abi2_k2d_command *clear = kromi_2d_command(&q, KROM_ABI2_K2D_OPCODE_FILL);
        krom_abi2_k2d_command *draw = kromi_2d_command(&q, KROM_ABI2_K2D_OPCODE_SOURCE_OVER);
        krom_abi2_k2d_command *show = kromi_2d_command(&q, KROM_ABI2_K2D_OPCODE_PRESENT);

        clear->destination_surface = target;                 /* the whole picture, night blue */
        clear->width = WIDTH;
        clear->height = HEIGHT;
        clear->color = 0x0014121CU;
        draw->destination_surface = target;                  /* the ball over it, where it is now */
        draw->source_surface = ball_picture;
        draw->destination_x = x;
        draw->destination_y = y;
        draw->width = BALL;
        draw->height = BALL;
        show->destination_surface = target;                  /* on screen at the next frame */
        show->present_id = frame + 1U;
        if ((status = kromi_2d_run(&q)) != KROM_ABI2_STATUS_OK) {
            kromi_say_number("A frame failed, status ", status);
            return 3;
        }
        /* move, bouncing off the edges */
        x += step_x;
        y += step_y;
        step_x = ((x <= 0) || (x >= (int32_t)(WIDTH - BALL))) ? -step_x : step_x;
        step_y = ((y <= 0) || (y >= (int32_t)(HEIGHT - BALL))) ? -step_y : step_y;
    }
    kromi_say_number("Frames shown: ", FRAMES);
    return 0;
}
