# Implementation Guide

This document provides a deep dive into the source code. It is intended for developers who want to modify the engine or understand exactly how it works under the hood.

## 1. Project Structure

The project is organized into a standard C project structure:

*   **`src/`**: Contains all `.c` source files.
    *   **`blocks/`**: Subdirectory containing the implementation for each specific block type.
*   **`include/`**: Contains all `.h` header files.
*   **`tests/`**: Contains JSON test files.
*   **`docs/`**: Documentation.
*   **`Makefile`**: Build configuration.

## 2. The Block System (`src/block_manager.c`)

The core of the extensibility is the **Block System**. It uses a manual implementation of polymorphism (similar to C++ vtables).

### 2.1. The `BlockVTable` Interface
Defined in `include/block_interface.h`, this struct defines the "methods" that every block must implement.

```c
typedef struct {
    const char *type_name;                           // The JSON "type" string (e.g., "text")
    void *(*parse)(json_object *json);               // Constructor: JSON -> Internal Data
    int (*calculate_height)(void *data, int width);  // Method: Calculate height in pixels
    void (*render)(void *data, BlockContext *ctx);   // Method: Draw to Cairo context
    void (*destroy)(void *data);                     // Destructor: Free internal data
} BlockVTable;
```

### 2.2. The `Block` Wrapper
This generic struct holds the specific data for an instance of a block.

```c
typedef struct {
    const BlockVTable *vtable; // Pointer to the implementation functions
    void *data;                // Pointer to the specific block data (e.g., TextBlockData)
} Block;
```

### 2.3. Registration
In `src/block_manager.c`, the `block_manager_init()` function registers all available blocks. To add a new block, you must register its VTable here.

```c
void block_manager_init() {
    block_manager_register(&text_block_vtable);
    block_manager_register(&image_block_vtable);
    // ...
}
```

## 3. Specific Block Implementations

### 3.1. Text Block (`src/blocks/text_block.c`)
*   **Data Structure**: Stores the text string and formatting options (align, weight, size).
*   **Height Calculation**: Creates a temporary Pango Layout, sets the width, sets the text, and asks Pango for the pixel height.
*   **Rendering**: Sets up the Pango Layout on the actual Cairo context and calls `pango_cairo_show_layout()`.

### 3.2. Table Block (`src/blocks/table_block.c`)
This is the most complex block.
*   **Data Structure**:
    *   `columns`: Array of `TableColumn` (header text, width, alignment).
    *   `rows`: 2D array of strings.
    *   `format`: Borders, padding, font size.
*   **Width Parsing**: Supports percentages ("50%") or raw pixels ("100"). It converts these to absolute pixel values based on the total available width.
*   **Height Calculation**:
    1.  Calculates the width of every column in pixels.
    2.  Iterates through every row.
    3.  For each cell in the row, calculates the text height given the column width.
    4.  The row height is the **maximum** of all cell heights in that row + padding.
    5.  Sums up all row heights.
*   **Rendering**:
    1.  Iterates rows and columns.
    2.  Renders text in the calculated cell box.
    3.  Draws lines using `cairo_move_to` and `cairo_line_to` based on the granular border configuration.

### 3.3. Image Block (`src/blocks/image_block.c`)
*   **Loading**: Uses `cairo_image_surface_create_from_png` to load images.
*   **Scaling**: If the image is wider than the paper, it calculates a scaling factor to fit it. It uses a high-quality scaling filter (`CAIRO_FILTER_BEST`) to resize the image during rendering.

## 4. Memory Management

The application uses dynamic memory allocation (`malloc`, `calloc`, `strdup`).
*   **Ownership**: The `BlockManager` owns the list of `Block` structs.
*   **Block Data**: Each `Block` owns its private `void *data`.
*   **Destruction**:
    1.  `block_list_destroy()` is called at the end of `main`.
    2.  It iterates the list.
    3.  For each block, it calls `vtable->destroy(block->data)` (letting the block free its own strings/arrays).
    4.  Then it frees the `Block` wrapper itself.

**Critical**: Every time you `strdup` a string from JSON, you must `free` it in the `destroy` function.

## 5. ESC/POS Utilities (`src/escpos_utils.c`)

This module handles the binary protocol.
*   **`create_escpos_raster_command`**:
    *   Input: A buffer of bytes where 1 bit = 1 pixel.
    *   Output: A buffer containing the ESC/POS command sequence.
    *   Format: `GS v 0 m xL xH yL yH d1...dk`
        *   `GS v 0`: Command header.
        *   `m`: Mode (0 for normal).
        *   `xL xH`: Width in bytes (little-endian).
        *   `yL yH`: Height in dots (little-endian).
        *   `d`: Data.

## 6. Bitmap Conversion (`src/bitmap_utils.c`)

This module converts the 32-bit ARGB Cairo surface to a 1-bit buffer.
*   **Logic**:
    *   Iterate Y from 0 to Height.
    *   Iterate X from 0 to Width.
    *   Get pixel value (ARGB).
    *   Calculate Luminance: `L = 0.299*R + 0.587*G + 0.114*B`.
    *   **Threshold**: If `L < 128` (dark), set the bit to 1. Else set to 0.
    *   **Bit Packing**: Bits are packed into bytes. The 7th bit corresponds to the first pixel, 6th to the second, etc.

## 7. Build System (`Makefile`)

*   **`CC`**: gcc
*   **`CFLAGS`**: `-Wall -Wextra -O2` (Warnings enabled, Optimization level 2).
*   **`INCLUDES`**: `-I./include` and `pkg-config` output.
*   **`LIBS`**: `pkg-config` output for cairo, pangocairo, libqrencode, json-c.
*   **Targets**:
    *   `all`: Builds `poc_printer`.
    *   `clean`: Removes object files and binary.
