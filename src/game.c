#include "game.h"

void render_box(const struct Rectangle *box, const struct BackBuffer *buffer)
{
    int size = box->width;

    u8 *row = buffer->data;
    for (int y = 0; y < buffer->height; y += 1)
    {
        u32 *pixel = (u32 *)row;
        for (int x = 0; x < buffer->width; x += 1)
        {
            u8 r = 0;
            u8 g = 0;
            u8 b = 0;
            u8 a = 255;

            if (x >= box->x && x <= box->x + size)
            {
                if (y >= box->y && y <= box->y + size)
                {
                    r = 255;
                }
            }

            *pixel = (r | g << 8 | b << 16 | a << 24);
            pixel += 1;
        }
        row += buffer->pitch;
    }
}

void game_update_and_render(struct BackBuffer *buffer, struct Rectangle *box)
{
    render_box(box, buffer);
}
