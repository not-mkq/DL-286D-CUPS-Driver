// dl286d-raster.c
// CUPS raster filter for Deli DL-286D label printer
//
// Usage example (paired with a raw queue):
//
//   cupsfilter \
//     -p /usr/share/ppd/cupsfilters/HP-Color_LaserJet_CM3530_MFP-PDF.ppd \
//     -m application/vnd.cups-raster \
//     input.png \
//     | dl286d-raster \
//     | lp -d dl286d-raw
//
// dl286d-raw is a raw queue that simply forwards bytes to the DL-286D.

#include <cups/cups.h>
#include <cups/raster.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define WIDTH_DOTS 384       // Print head width in dots
#define CANVAS_HEIGHT 320    // Buffer height (matches the label height)
#define PRINTABLE_HEIGHT 320 // Visible label height
#define WIDTH_BYTES (WIDTH_DOTS / 8)
#define SEG_ROWS 32                         // Number of rows per GS v 0 block
#define NUM_SEGS (CANVAS_HEIGHT / SEG_ROWS) // 10 segments x 32 rows = 320 rows

// Prefix shared with the Python prototype
static const uint8_t PREFIX_BYTES[] = {0x1f, 0x80, 0x01, 0x20,
                                       0x1f, 0x11, 0x51, 0x00};

// GS v 0 m xL xH yL yH
// m=0, width 0x30=48 bytes=384 dots, height 0x20=32 rows
static const uint8_t GS_V0_HEADER[8] = {0x1d, 0x76, 0x30, 0x00,
                                        0x30, 0x00, 0x20, 0x00};

// Suffix pushes the label all the way out
static const uint8_t SUFFIX_BYTES[] = {0x1f, 0xf0, 0x05, 0x00,
                                       0x1f, 0xf0, 0x03, 0x00};

// 8x8 Bayer matrix (0-63) for ordered dithering
static const uint8_t dither8x8[8][8] = {
    {0, 32, 8, 40, 2, 34, 10, 42},  {48, 16, 56, 24, 50, 18, 58, 26},
    {12, 44, 4, 36, 14, 46, 6, 38}, {60, 28, 52, 20, 62, 30, 54, 22},
    {3, 35, 11, 43, 1, 33, 9, 41},  {51, 19, 59, 27, 49, 17, 57, 25},
    {15, 47, 7, 39, 13, 45, 5, 37}, {63, 31, 55, 23, 61, 29, 53, 21}};

/**
 * dither_is_black
 *
 * - Grayscale range: 0 = white, 255 = black (original driver convention).
 * - Very bright areas (>=250): force white, skip dithering to avoid dirtying the
 *   background.
 * - Very dark areas (<=5): force black to prevent grid artifacts inside solid
 *   fills.
 * - Mid-range tones: apply the Bayer matrix for ordered dithering.
 */
static inline int dither_is_black(uint8_t gray, int x, int y) {
  // 1. Pure white or near-white: never place dots
  if (gray >= 250)
    return 1;

  // 2. Pure black or near-black: draw solid fill
  if (gray <= 5)
    return 0;

  // 3. Mid-range grays: ordered dithering
  int tx = x & 7;
  int ty = y & 7;
  uint8_t threshold = (uint8_t)(dither8x8[ty][tx] << 2); // 0-63 -> 0-252

  // Higher grayscale values cross the threshold more easily -> black dot
  return (gray >= threshold);
    }

int main(int argc, char *argv[]) {
  (void)argc;
  (void)argv;

  cups_raster_t *ras = cupsRasterOpen(0, CUPS_RASTER_READ);
  if (!ras) {
    fprintf(stderr, "dl286d-raster: failed to open cups raster on stdin\n");
    return 1;
  }

  cups_page_header2_t h;
  int page = 0;

  while (cupsRasterReadHeader2(ras, &h)) {
    page++;

    int src_w = h.cupsWidth;
    int src_h = h.cupsHeight;
    int bpp = h.cupsBitsPerPixel;
    int bpl = h.cupsBytesPerLine;

    fprintf(stderr,
            "dl286d-raster: page %d, src_w=%d, src_h=%d, bpp=%d, bpl=%d\n",
            page, src_w, src_h, bpp, bpl);

    if (src_w <= 0 || src_h <= 0) {
      fprintf(stderr, "dl286d-raster: invalid raster size\n");
      break;
    }

    // Decide whether to enable ordered dithering:
    // bpp >= 8 -> treat as grayscale/color images and enable dithering.
    // Low bit-depth images (1 bpp) use a simple threshold suited for UI/work.
    int use_dither = (bpp >= 8);
    fprintf(stderr, "dl286d-raster: use_dither=%d\n", use_dither);

    // Convert the entire page into a grayscale buffer (src_gray[y * src_w + x])
    uint8_t *src_gray = (uint8_t *)malloc((size_t)src_w * src_h);
    if (!src_gray) {
      fprintf(stderr, "dl286d-raster: OOM for src_gray (%dx%d)\n", src_w,
              src_h);
      cupsRasterClose(ras);
      return 1;
    }

    uint8_t *rowbuf = (uint8_t *)malloc(bpl);
    if (!rowbuf) {
      fprintf(stderr, "dl286d-raster: OOM for rowbuf\n");
      free(src_gray);
      cupsRasterClose(ras);
      return 1;
    }

    int actual_h = 0;
    for (int y = 0; y < src_h; y++) {
      if (cupsRasterReadPixels(ras, rowbuf, bpl) != (unsigned)bpl) {
        fprintf(stderr, "dl286d-raster: short read at row %d\n", y);
        break;
      }

      for (int x = 0; x < src_w; x++) {
        uint8_t gray = 255; // Default to black for safety

        if (bpp == 8) {
          // Gray 8bpp
          if (x < bpl) {
            gray = rowbuf[x];
          }
        } else if (bpp == 1) {
          // 1 bpp monochrome where bit=1 means black
          int byte_index = x / 8;
          int bit_index = 7 - (x % 8);
          if (byte_index < bpl) {
            uint8_t bb = rowbuf[byte_index];
            gray = (bb & (1 << bit_index)) ? 255 : 0;
          }
        } else if (bpp == 24 || bpp == 32) {
          // RGB / RGBA -> simple average
          int px_index = x * (bpp / 8);
          if (px_index + 2 < bpl) {
            uint8_t r = rowbuf[px_index + 0];
            uint8_t g = rowbuf[px_index + 1];
            uint8_t b = rowbuf[px_index + 2];
            gray = (uint8_t)((r + g + b) / 3);
          }
        } else {
          // Other exotic formats -> fall back to the first byte
          if (x < bpl) {
            gray = rowbuf[x];
          }
        }

        // Optional: boost very bright tones to avoid dotted light backgrounds
        if (gray > 220) {
          gray = 255;
        }

        src_gray[y * src_w + x] = gray;
      }

      actual_h++;
    }

    free(rowbuf);

    if (actual_h <= 0) {
      fprintf(stderr, "dl286d-raster: no rows read\n");
      free(src_gray);
      continue;
    }
    if (actual_h < src_h) {
      src_h = actual_h;
    }

    // Compute uniform scaling to fit within 384x320 while keeping aspect ratio
    double scale_x = (double)WIDTH_DOTS / (double)src_w;
    double scale_y = (double)PRINTABLE_HEIGHT / (double)src_h;
    double scale = (scale_x < scale_y) ? scale_x : scale_y;

    if (scale <= 0.0) {
      scale = 1.0;
    }

    int dest_w = (int)(src_w * scale + 0.5);
    int dest_h = (int)(src_h * scale + 0.5);
    if (dest_w < 1)
      dest_w = 1;
    if (dest_h < 1)
      dest_h = 1;
    if (dest_w > WIDTH_DOTS)
      dest_w = WIDTH_DOTS;
    if (dest_h > PRINTABLE_HEIGHT)
      dest_h = PRINTABLE_HEIGHT;

    fprintf(stderr, "dl286d-raster: scale=%.4f, dest_w=%d, dest_h=%d\n", scale,
            dest_w, dest_h);

    // Initialize a 320x384 white canvas
    static uint8_t canvas[CANVAS_HEIGHT][WIDTH_DOTS];
    memset(canvas, 0, sizeof(canvas));

    // Center horizontally and vertically within the 320 px height
    int x_offset = (WIDTH_DOTS - dest_w) / 2;
    int y_offset = (PRINTABLE_HEIGHT - dest_h) / 2;
    if (x_offset < 0)
      x_offset = 0;
    if (y_offset < 0)
      y_offset = 0;

    // Nearest-neighbor scaling with writes to the canvas
    for (int dy = 0; dy < dest_h; dy++) {
      int sy = (int)(dy / scale);
      if (sy >= src_h)
        sy = src_h - 1;

      for (int dx = 0; dx < dest_w; dx++) {
        int sx = (int)(dx / scale);
        if (sx >= src_w)
          sx = src_w - 1;

        uint8_t gray = src_gray[sy * src_w + sx];

        int paper_x = x_offset + dx; // Left to right
        int paper_y = y_offset + dy; // Top to bottom

        if (paper_x < 0 || paper_x >= WIDTH_DOTS || paper_y < 0 ||
            paper_y >= PRINTABLE_HEIGHT) {
          continue;
        }

        int raster_x = paper_x; // No mirroring
        int raster_y = paper_y;

        int is_black;
        if (use_dither) {
          is_black = dither_is_black(gray, paper_x, paper_y);
        } else {
          // Simple threshold for layout/line art similar to the legacy logic
          is_black = (gray >= 128);
        }

        if (is_black) {
          canvas[raster_y][raster_x] = 1;
        }
      }
    }

    free(src_gray);

    // Emit ESC/POS: PREFIX + NUM_SEGS GS v 0 blocks + SUFFIX
    fwrite(PREFIX_BYTES, 1, sizeof(PREFIX_BYTES), stdout);

    for (int seg = 0; seg < NUM_SEGS; seg++) {
      fwrite(GS_V0_HEADER, 1, sizeof(GS_V0_HEADER), stdout);
      int base_row = seg * SEG_ROWS;

      for (int row = 0; row < SEG_ROWS; row++) {
        int y = base_row + row;
        for (int bx = 0; bx < WIDTH_BYTES; bx++) {
          uint8_t byte_val = 0;
          for (int bit = 0; bit < 8; bit++) {
            int x = bx * 8 + bit;
            if (canvas[y][x]) {
              byte_val |= (1 << (7 - bit)); // MSB represents the leftmost dot
            }
          }
          fputc(byte_val, stdout);
        }
      }
    }

    // Feed paper to the end of the label
    fwrite(SUFFIX_BYTES, 1, sizeof(SUFFIX_BYTES), stdout);
    fflush(stdout);

    // Continue for additional pages; break here to force single-page output
    // break;
  }

  cupsRasterClose(ras);
  return 0;
}
