#include "printer_utils.h"

unsigned char* create_escpos_raster_command(unsigned char *bitmap_data, int width, int height, int *cmd_size) {
    // Calculate bytes needed for ESC/POS command
    // Format: GS v 0 m xL xH yL yH [bitmap data]
    int bytes_per_row = (width + 7) / 8;
    int bitmap_size = bytes_per_row * height;

    // Command size includes the ESC/POS header + the bitmap data
    *cmd_size = 10 + bitmap_size;  // 6 bytes for header + 2 bytes each for x and y + bitmap data + 2 bytes for footer

    unsigned char *command = malloc(*cmd_size);
    if (!command) {
        printf("Error: Could not allocate memory for ESC/POS command\n");
        return NULL;
    }

    int idx = 0;

    // Initialize printer command
    command[idx++] = 0x1B;  // ESC
    command[idx++] = '@';   // @ (initialize)

    // GS v 0 command (raster bit image)
    command[idx++] = 0x1D;  // GS
    command[idx++] = 'v';   // v
    command[idx++] = '0';   // 0 (raster format)

    // Width and height (in bytes)
    unsigned char xL = bytes_per_row & 0xFF;      // Lower byte
    unsigned char xH = (bytes_per_row >> 8) & 0xFF;  // Higher byte
    unsigned char yL = height & 0xFF;             // Lower byte
    unsigned char yH = (height >> 8) & 0xFF;      // Higher byte

    command[idx++] = 0;      // m (mode - 0 for normal)
    command[idx++] = xL;
    command[idx++] = xH;
    command[idx++] = yL;
    command[idx++] = yH;

    // Copy bitmap data
    memcpy(&command[idx], bitmap_data, bitmap_size);
    idx += bitmap_size;

    // Add a feed and cut command at the end
    command[idx++] = 0x1B;  // ESC
    command[idx++] = 'd';   // d (feed n lines)
    command[idx++] = 3;     // Feed 3 lines

    command[idx++] = 0x1D;  // GS
    command[idx++] = 'V';   // V
    command[idx++] = 1;     // Full cut

    *cmd_size = idx;
    return command;
}