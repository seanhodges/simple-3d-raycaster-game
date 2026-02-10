/*  gen_sprites.c  –  generate a placeholder sprite atlas BMP
 *  ─────────────────────────────────────────────────────────
 *  Outputs assets/sprites.bmp: 256x64 pixels (4 tiles of 64x64).
 *  Each tile has a distinct coloured shape on a #980088 magenta background.
 *  Run from the project root: ./tools/gen_sprites
 */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

#define TEX_SIZE 64
#define TEX_COUNT 4
#define WIDTH  (TEX_COUNT * TEX_SIZE)
#define HEIGHT TEX_SIZE

/* BMP is BGR, bottom-up.  We write 24-bit BMP for simplicity. */

#pragma pack(push, 1)
typedef struct {
    uint16_t type;        /* 'BM' */
    uint32_t file_size;
    uint16_t reserved1;
    uint16_t reserved2;
    uint32_t offset;
    uint32_t header_size; /* 40 */
    int32_t  width;
    int32_t  height;
    uint16_t planes;
    uint16_t bpp;
    uint32_t compression;
    uint32_t image_size;
    int32_t  x_ppm;
    int32_t  y_ppm;
    uint32_t colors_used;
    uint32_t colors_important;
} BMPHeader;
#pragma pack(pop)

/* Alpha key: #980088 (R=0x98, G=0x00, B=0x88) */
static const uint8_t BG_R = 0x98, BG_G = 0x00, BG_B = 0x88;

static void set_pixel(uint8_t *row, int x, uint8_t r, uint8_t g, uint8_t b)
{
    row[x * 3 + 0] = b;  /* BMP is BGR */
    row[x * 3 + 1] = g;
    row[x * 3 + 2] = r;
}

int main(void)
{
    int row_bytes = WIDTH * 3;
    int padding = (4 - (row_bytes % 4)) % 4;
    int stride = row_bytes + padding;
    uint32_t image_size = (uint32_t)(stride * HEIGHT);

    BMPHeader hdr;
    memset(&hdr, 0, sizeof(hdr));
    hdr.type         = 0x4D42;  /* 'BM' */
    hdr.file_size    = (uint32_t)(sizeof(BMPHeader) + image_size);
    hdr.offset       = sizeof(BMPHeader);
    hdr.header_size  = 40;
    hdr.width        = WIDTH;
    hdr.height       = HEIGHT;  /* positive = bottom-up */
    hdr.planes       = 1;
    hdr.bpp          = 24;
    hdr.image_size   = image_size;

    FILE *fp = fopen("assets/sprites.bmp", "wb");
    if (!fp) {
        fprintf(stderr, "gen_sprites: cannot create assets/sprites.bmp\n");
        return 1;
    }

    fwrite(&hdr, sizeof(hdr), 1, fp);

    uint8_t row[WIDTH * 3 + 4]; /* +4 for padding */
    memset(row, 0, sizeof(row));

    /* Write rows bottom-up (BMP convention) */
    for (int bmp_row = 0; bmp_row < HEIGHT; bmp_row++) {
        /* Convert bottom-up to top-down y */
        int y = (HEIGHT - 1) - bmp_row;

        /* Fill entire row with alpha key background */
        for (int x = 0; x < WIDTH; x++)
            set_pixel(row, x, BG_R, BG_G, BG_B);

        /* Tile 0: Red diamond */
        {
            for (int x = 0; x < TEX_SIZE; x++) {
                int ax = (x > TEX_SIZE / 2) ? (x - TEX_SIZE / 2) : (TEX_SIZE / 2 - x);
                int ay = (y > TEX_SIZE / 2) ? (y - TEX_SIZE / 2) : (TEX_SIZE / 2 - y);
                if (ax + ay < TEX_SIZE / 2 - 4)
                    set_pixel(row, 0 * TEX_SIZE + x, 0xFF, 0x33, 0x33);
            }
        }

        /* Tile 1: Green circle */
        {
            for (int x = 0; x < TEX_SIZE; x++) {
                int dx = x - TEX_SIZE / 2;
                int dy = y - TEX_SIZE / 2;
                if (dx * dx + dy * dy < (TEX_SIZE / 2 - 4) * (TEX_SIZE / 2 - 4))
                    set_pixel(row, 1 * TEX_SIZE + x, 0x33, 0xCC, 0x33);
            }
        }

        /* Tile 2: Blue column (vertical bar with shading) */
        {
            for (int x = 0; x < TEX_SIZE; x++) {
                int dx = x - TEX_SIZE / 2;
                if (dx > -12 && dx < 12) {
                    /* Simple cylindrical shading */
                    int shade = 200 - (dx * dx) / 2;
                    if (shade < 60) shade = 60;
                    set_pixel(row, 2 * TEX_SIZE + x,
                              (uint8_t)(shade / 4),
                              (uint8_t)(shade / 4),
                              (uint8_t)(shade));
                }
            }
        }

        /* Tile 3: Yellow star (6-pointed) */
        {
            for (int x = 0; x < TEX_SIZE; x++) {
                float fx = (float)(x - TEX_SIZE / 2);
                float fy = (float)(y - TEX_SIZE / 2);
                float r = sqrtf(fx * fx + fy * fy);
                float angle = atan2f(fy, fx);
                /* Star pattern: radius modulated by angle */
                float star_r = 20.0f + 8.0f * cosf(angle * 6.0f);
                if (r < star_r)
                    set_pixel(row, 3 * TEX_SIZE + x, 0xFF, 0xDD, 0x33);
            }
        }

        fwrite(row, 1, (size_t)stride, fp);
    }

    fclose(fp);
    printf("Generated assets/sprites.bmp (%dx%d, %d tiles)\n",
           WIDTH, HEIGHT, TEX_COUNT);
    return 0;
}
