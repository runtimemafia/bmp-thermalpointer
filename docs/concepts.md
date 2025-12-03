# Concepts & Architecture

## 1. Introduction
The **Thermal Printer Engine** is a specialized C application designed to bridge the gap between high-level, structured document formats (JSON) and the low-level, byte-oriented protocols used by thermal receipt printers (ESC/POS).

In modern web and application development, developers are accustomed to defining layouts using HTML/CSS or structured objects. However, thermal printers typically speak a legacy protocol called **ESC/POS**, which requires sending specific byte sequences to control the print head, align text, and print bitmaps. This application abstracts away that complexity, allowing developers to "print JSON" directly.

## 2. Thermal Printing Fundamentals
To understand how this engine works, it is helpful to understand the target hardware.

### 2.1. The Print Head
A thermal printer does not use ink. It has a **thermal print head** consisting of a line of tiny heating elements (dots). As the thermal paper passes over the head, specific elements heat up, turning the chemical coating on the paper black.

### 2.2. Dots and Resolution
*   **Width**: The most common receipt width is **58mm**.
*   **Resolution**: A standard 58mm printer usually has a print width of **384 dots**.
*   **DPI**: This equates to approximately **203 DPI** (Dots Per Inch).
*   **Implication**: All rendering in this engine is calculated in "dots". If you want a line to span half the page, it must be 192 dots wide.

### 2.3. Raster vs. Native Commands
There are two ways to print on a thermal printer:
1.  **Native Commands**: Sending ASCII text bytes directly to the printer (e.g., sending "Hello"). The printer uses its internal built-in font.
    *   *Pros*: Fast, low data transfer.
    *   *Cons*: Limited fonts, no support for complex scripts (like Malayalam/Hindi) unless the printer has a specific font card, limited layout control.
2.  **Raster Graphics (Our Approach)**: Rendering the entire receipt as a single image (bitmap) in memory, and then sending that image to the printer dot-by-dot.
    *   *Pros*: **Pixel-perfect control**. We can use any font, any language, any layout, images, and QR codes. What you see on screen is exactly what prints.
    *   *Cons*: Slower (sending image data takes more bandwidth), but negligible for modern USB/Network printers.

## 3. The Rendering Pipeline
The application follows a strict linear pipeline to process a print job.

### Step 1: JSON Parsing
The input is a JSON object containing a list of "blocks".
```json
{
  "blocks": [ ... ]
}
```
The engine uses `libjson-c` to parse this text into a DOM (Document Object Model) structure in memory.

### Step 2: Block Instantiation
The **Block Manager** iterates through the JSON array. For each item, it looks at the `"type"` field (e.g., "text", "table"). It then delegates the creation of an internal `Block` structure to the specific handler for that type.

### Step 3: Layout & Height Calculation
Before we can draw anything, we need to know how long the receipt will be.
*   The width is fixed (384 dots).
*   The engine asks each block: *"Given a width of 384 dots, how many vertical dots do you need?"*
*   **Text Blocks**: Use Pango to calculate how many lines the text will wrap into.
*   **Image Blocks**: Calculate height based on aspect ratio.
*   **Table Blocks**: Calculate the height of the tallest cell in each row.
*   The sum of all block heights + padding = **Total Surface Height**.

### Step 4: Cairo Rendering (The "Canvas")
We allocate a **Cairo Surface** (`cairo_image_surface_create`) with the calculated dimensions. This acts like an in-memory canvas.
*   We paint the background white.
*   We iterate through the blocks again, passing them the Cairo Context (`cairo_t`).
*   Each block draws itself onto the canvas at the current Y-offset.
*   **Text**: Drawn using PangoCairo, which handles font rendering and anti-aliasing.
*   **Shapes**: Lines and borders are drawn using Cairo's vector drawing paths.

### Step 5: Bitmap Conversion (Dithering)
The Cairo surface is in **ARGB32** format (Alpha, Red, Green, Blue - 32 bits per pixel). The printer only understands **1-bit** (Black or White).
*   We scan the image pixel by pixel.
*   **Thresholding**: If a pixel's brightness is below a certain threshold (e.g., 128), it becomes a 1 (Black dot). Otherwise, it becomes a 0 (White dot).
*   This produces a raw buffer of bits.

### Step 6: ESC/POS Encoding
The raw bits are packaged into the ESC/POS command format `GS v 0` (Raster Bit Image).
*   **Header**: `0x1D 0x76 0x30 0x00` (Command ID).
*   **Width Bytes**: `48` (384 dots / 8 bits per byte).
*   **Height**: The number of lines.
*   **Data**: The raw bit buffer.

### Step 7: Transmission
The final byte sequence is written to the output file descriptor. This could be a file (`output.bin`) or a device character file (`/dev/usb/lp0`).

## 4. Technology Stack

### 4.1. C Language
Chosen for performance, low-level memory control, and ease of interaction with Linux device files.

### 4.2. Cairo Graphics
A 2D vector graphics library. It is the industry standard for high-quality 2D rendering on Linux. We use it to draw the "receipt image".

### 4.3. Pango
A library for laying out and rendering of text, with an emphasis on internationalization.
*   **Why Pango?** Simple text rendering libraries often fail at **Complex Text Layout (CTL)** required for languages like Arabic, Hindi, or Malayalam (where characters change shape based on neighbors). Pango handles this perfectly.

### 4.4. JSON-C
A lightweight C library for parsing JSON.

### 4.5. LibQREncode
A library for encoding data into a QR code matrix.

## 5. Coordinate System
*   **X-Axis**: 0 is the left edge. 384 is the right edge.
*   **Y-Axis**: 0 is the top edge. Increases downwards.
*   **Margins**: The engine enforces a `SIDE_MARGIN` (typically 10 dots) to ensure content doesn't get cut off by the printer mechanism.
*   **Content Width**: `384 - (2 * 10) = 364 dots`. Blocks must fit their content within this width.
