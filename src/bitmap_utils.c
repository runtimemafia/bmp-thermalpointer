#include "printer_utils.h"

unsigned char *convert_to_1bit_bmp(cairo_surface_t *surface, int *width,
                                   int *height, int *size) {
  *width = cairo_image_surface_get_width(surface);
  *height = cairo_image_surface_get_height(surface);

  unsigned char *data = cairo_image_surface_get_data(surface);
  int stride = cairo_image_surface_get_stride(surface);

  // Calculate number of bytes needed for 1-bit bitmap (8 pixels per byte)
  int bytes_per_row = (*width + 7) / 8; // Round up to nearest byte
  *size = bytes_per_row * (*height);

  unsigned char *bitmap = malloc(*size);
  if (!bitmap) {
    printf("Error: Could not allocate memory for bitmap\n");
    return NULL;
  }

  // Convert ARGB32 to 1-bit bitmap (black and white)
  for (int y = 0; y < *height; y++) {
    for (int x = 0; x < *width; x++) {
      // Get pixel from ARGB32 surface (in BGRA format)
      int pixel_offset = y * stride + x * 4; // 4 bytes per pixel

      // Extract RGB values (ARGB32 format)
      unsigned char b = data[pixel_offset];
      unsigned char g = data[pixel_offset + 1];
      unsigned char r = data[pixel_offset + 2];

      // Calculate grayscale value (luminance)
      int gray = (int)(0.299 * r + 0.587 * g + 0.114 * b);

      // Convert to 1-bit (white=1, black=0)
      int bit_pos = x % 8;
      int byte_idx = y * bytes_per_row + (x / 8);

      if (gray < 128) {                        // Black threshold
        bitmap[byte_idx] |= (0x80 >> bit_pos); // Set bit (MSB first format)
      } else {
        bitmap[byte_idx] &= ~(0x80 >> bit_pos); // Clear bit
      }
    }
  }

  return bitmap;
}
