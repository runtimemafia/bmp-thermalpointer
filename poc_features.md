# Thermal Printer POC - Features Documentation

This document describes all the features implemented in the thermal printer proof of concept (POC) that can be incorporated into the core product.

## 1. Dynamic Font Size Control

### Feature
Font size is now controlled by a configurable constant instead of being hardcoded.

### Implementation Details
- Added `#define DEFAULT_FONT_SIZE 24` at the top of the file
- Modified the font string creation in both `calculate_text_height()` and `render_text_to_surface()` functions
- Uses `snprintf` to format the font string: `"%s %s %d"` where:
  - %s is the font name
  - %s is the font weight
  - %d is the font size

### Benefits
- Easy to modify font size without changing core logic
- Consistent font size across text rendering and height calculation

## 2. Dynamic Height Calculation

### Feature
Text height is calculated dynamically based on actual content, reducing unnecessary padding.

### Implementation Details
- Added `calculate_text_height()` function that creates a dummy surface to measure text dimensions
- Uses Pango layout functions to calculate actual text height: `pango_layout_get_pixel_size()`
- Surface height is calculated as: `text_height + TOP_PADDING + BOTTOM_PADDING`
- Eliminates the hardcoded 800px height

### Benefits
- Reduces file size of output
- Eliminates excessive blank space at bottom
- More efficient thermal paper usage

## 3. PNG Preview Generation

### Feature
Generates both ESC/POS binary output and PNG preview for visual verification.

### Implementation Details
- Added `save_to_png()` function using Cairo's `cairo_surface_write_to_png()` 
- PNG preview is saved as "preview.png" in the working directory
- Includes the same content as the ESC/POS output for accurate preview
- Added necessary Cairo PNG includes: `<cairo.h>` (PNG support is built into base Cairo)

### Benefits
- Visual verification without printing
- Easier debugging and testing
- Can see exact output before sending to printer

## 4. Configurable Font Weight

### Feature
Font weight is controlled by a configurable constant instead of being hardcoded.

### Implementation Details
- Added `#define DEFAULT_FONT_WEIGHT "Light"` at the top of the file
- Modified the font string creation to include the weight variable
- Uses format: `"%s %s %d"` for font name, weight, and size
- Supports values like "Regular", "Bold", "Light", "Thin", etc.

### Benefits
- Easy to adjust text appearance
- Fine-tune readability vs. boldness
- Consistent weight across all text elements

## 5. Configurable Padding Options

### Feature
Top, bottom, and side padding are controlled by configurable constants.

### Implementation Details
- Added `TOP_PADDING`, `BOTTOM_PADDING`, and `SIDE_MARGIN` constants
- Surface height calculation: `text_height + TOP_PADDING + BOTTOM_PADDING`
- Text rendering position uses: `render_text_to_surface(surface, text, SIDE_MARGIN, TOP_PADDING)`
- Margin calculation: `PRINTER_WIDTH_DOTS - (2 * SIDE_MARGIN)`

### Benefits
- Customizable spacing around content
- Consistent padding across all outputs
- Easy to adjust for different receipt layouts

## 6. ESC/POS Command Generation

### Feature
Generates proper ESC/POS commands for thermal printer output.

### Implementation Details
- Uses `create_escpos_raster_command()` function to generate commands
- Implements GS v 0 raster bitmap format
- Includes printer initialization command (`ESC @`)
- Adds feed and cut commands at the end
- Properly formats width/height bytes in ESC/POS format

### Benefits
- Compatible with most ESC/POS thermal printers
- Proper binary output format
- Includes paper cutting functionality

## 7. Unicode Text Support

### Feature
Full Unicode support for complex scripts like Malayalam.

### Implementation Details
- Uses Pango for text rendering and shaping
- Font configuration supports complex script fonts like "Noto Sans Malayalam"
- UTF-8 text input is processed correctly
- Text wrapping works with Unicode characters

### Benefits
- Supports multiple languages
- Proper text shaping for complex scripts
- Maintains readability for non-Latin scripts

## 8. Binary Output Generation

### Feature
Generates ESC/POS binary output for direct printer communication.

### Implementation Details
- Converts ARGB32 Cairo surface to 1-bit bitmap
- Uses `convert_to_1bit_bmp()` function for monochrome conversion
- Implements proper byte packing (8 pixels per byte)
- MSB-first format for thermal printer compatibility

### Benefits
- Direct printer communication
- Proper thermal printer format
- Efficient binary representation

## 9. Direct USB Printer Communication

### Feature
Supports direct communication with USB thermal printers.

### Implementation Details
- `send_to_usb_printer()` function for direct device communication
- Uses standard file operations (`open`, `write`, `fsync`)
- Accepts device path as command line parameter
- Falls back to file output if printer device not specified

### Benefits
- Direct print capability
- No intermediate software needed
- Proper error handling for device communication

## 10. Command Line Interface

### Feature
Flexible command line interface supporting various usage patterns.

### Implementation Details
- Usage: `./poc_printer "text to print" [printer_device_path]`
- Example: `./poc_printer "ഹലോ" /dev/usb/lp0`
- Optional printer device parameter
- Automatic file output if no printer specified

### Benefits
- Easy integration into other systems
- Flexible deployment options
- Clear usage instructions