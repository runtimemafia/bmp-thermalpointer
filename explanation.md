# Thermal Printer Application Explanation

## Overview

This thermal printer application is a proof-of-concept that demonstrates how to render text (including Unicode) to a thermal printer using ESC/POS commands. The application takes text input, renders it as a visual preview (PNG), converts it to ESC/POS binary format, and can send it directly to a USB thermal printer.

## Architecture

The application follows a multi-stage pipeline:

1. **Input Processing** - Receives text input and optional printer device path
2. **Text Layout & Rendering** - Uses Pango/Cairo to render text with proper Unicode support
3. **Dynamic Sizing** - Calculates the exact height needed for the text content
4. **Bitmap Conversion** - Converts the rendered image to 1-bit thermal printer format
5. **ESC/POS Encoding** - Generates proper ESC/POS commands for the printer
6. **Output Generation** - Creates both binary output and PNG preview

## Key Components

### 1. Text Rendering with Pango/Cairo

The application uses Pango and Cairo libraries to handle complex text rendering:

- **Pango** handles text shaping, wrapping, and Unicode support
- **Cairo** provides the 2D graphics surface for rendering
- The `calculate_text_height()` function creates a dummy surface to measure text dimensions before actual rendering
- Supports complex scripts like Malayalam through fonts like "Noto Sans Malayalam"

### 2. Dynamic Sizing

Instead of using fixed dimensions, the application calculates the required height dynamically:

```c
int text_height = calculate_text_height(text, text_width);
int surface_height = text_height + TOP_PADDING + BOTTOM_PADDING;
```

This makes the application more efficient by reducing unnecessary blank space and saving thermal paper.

### 3. 1-Bit Bitmap Conversion

Thermal printers require 1-bit (monochrome) images. The application converts the ARGB32 Cairo surface to 1-bit format:

- Each pixel is converted to grayscale using luminance calculation
- Pixels below the threshold (128) become black, others become white
- Uses bit-packing format with 8 pixels per byte in MSB format
- Efficient memory usage for thermal printer compatibility

### 4. ESC/POS Command Generation

The application generates proper ESC/POS commands for thermal printer communication:

- **Initialization Command**: `ESC @` - Resets the printer
- **Raster Image Command**: `GS v 0` - Sends bitmap data to printer
- **Feed Command**: `ESC d n` - Feeds n lines of paper
- **Cut Command**: `GS V 1` - Performs a full paper cut

The command structure includes:
- Width and height in bytes (xL, xH, yL, yH)
- Proper byte packing for the bitmap data
- Mode settings for normal printing

### 5. Output Options

The application supports multiple output modes:

- **File Output**: Saves ESC/POS commands to `output.bin` when no printer is specified
- **Direct Printing**: Sends commands directly to USB printer via device path (e.g., `/dev/usb/lp0`)
- **Preview Generation**: Creates `preview.png` for visual verification before printing

## Configuration Constants

The application uses several configurable constants:

```c
#define DEFAULT_FONT_NAME "Noto Sans Malayalam"
#define DEFAULT_FONT_SIZE 24
#define DEFAULT_FONT_WEIGHT "Light"
#define PRINTER_WIDTH_MM 48
#define PRINTER_WIDTH_DOTS 384
#define TOP_PADDING 0
#define BOTTOM_PADDING 140
#define SIDE_MARGIN 10
```

These can be easily modified to adjust the appearance and behavior of the printed output.

## Usage

### Building the Application
```bash
make
```

Or directly with:
```bash
gcc -Wall -Wextra -O2 `pkg-config --cflags pangocairo cairo` -o poc_printer src/poc_printer.c `pkg-config --libs pangocairo cairo`
```

### Running the Application
```bash
# Generate output.bin only
./poc_printer "ഹലോ World"

# Print directly to USB printer
./poc_printer "ഹലോ World" /dev/usb/lp0
```

## Integration for Your Main App

To integrate this functionality into your main application:

### 1. Library Extraction
You can extract the key functions into a reusable library:
- `render_text_to_surface()` - Text rendering logic
- `convert_to_1bit_bmp()` - Bitmap conversion
- `create_escpos_raster_command()` - ESC/POS encoding
- `send_to_usb_printer()` - Direct printing

### 2. API Integration
Consider creating a clean C API like:
```c
typedef struct {
    int width_dots;
    int font_size;
    char font_name[256];
    char font_weight[32];
} printer_config_t;

unsigned char* render_text_to_escpos(const char* text, printer_config_t* config, int* output_size);
int send_to_printer(unsigned char* data, int size, const char* device_path);
```

### 3. JSON-Based Input
As shown in the plan.md, you can extend this to support JSON-based input for more complex receipts with:
- Multiple text blocks with different formatting
- Tables and layouts
- QR codes
- Images
- Horizontal rules

### 4. Cross-Platform Considerations
- The current implementation targets Linux (specifically for Raspberry Pi)
- Device paths on other systems may differ
- Consider abstracting the printing interface for cross-platform compatibility

## Key Benefits

- **Unicode Support**: Proper handling of complex scripts like Malayalam, Hindi, Tamil
- **Efficient Memory Usage**: Dynamic sizing reduces memory overhead
- **Visual Verification**: PNG preview allows checking before printing
- **Direct Printing**: Can send commands directly to printers without intermediate software
- **Configurable**: Easy to adjust fonts, sizing, and layout through constants
- **ESC/POS Compatible**: Works with most thermal printers that support ESC/POS commands

## Future Extensions

Based on the plan.md, potential extensions include:
- JSON-based input for complex receipt layouts
- Support for tables, QR codes, and images
- Device output abstraction
- Job queue management
- Multiple output formats