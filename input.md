# Thermal Printer Input Format Documentation

This document outlines the JSON format accepted by the thermal printer POC application. The format allows for flexible layout of text, QR codes, and other elements on thermal paper.

## Overall Structure

```json
{
  "blocks": [
    // Array of block objects
  ]
}
```

## Dynamic Height Calculation

The application automatically calculates the required height for your content, eliminating the possibility of content being cut off at the bottom. This means all content will be fully rendered regardless of length.

## Supported Block Types

### Text Block

Displays text content with customizable formatting options.

#### Basic Format
```json
{
  "type": "text",
  "content": "Your text content here"
}
```

#### Enhanced Format with Options
```json
{
  "type": "text",
  "content": "Your text content here",
  "font_size": 24,
  "font_weight": "Light",
  "alignment": "left",
  "top_padding": 0,
  "bottom_padding": 0,
  "left_padding": 0,
  "right_padding": 0
}
```

#### Text Block Options

- **`content`** (required, string): The text content to display
- **`font_size`** (optional, integer): Font size in points. Default: 24
- **`font_weight`** (optional, string): Font weight (e.g., "Light", "Bold", "Thin"). Default: "Light"
- **`alignment`** (optional, string): Text alignment ("left", "center", "right"). Default: "left"
- **`top_padding`** (optional, integer): Padding in pixels above the text. Default: 0
- **`bottom_padding`** (optional, integer): Padding in pixels below the text. Default: 0
- **`left_padding`** (optional, integer): Padding in pixels to the left of the text. Default: 0
- **`right_padding`** (optional, integer): Padding in pixels to the right of the text. Default: 0

### QR Code Block

Displays a QR code with customizable sizing and positioning.

#### Basic Format
```json
{
  "type": "qr",
  "data": "https://example.com"
}
```

#### Enhanced Format with Options
```json
{
  "type": "qr",
  "data": "https://example.com",
  "size": 3,
  "x_pos": 0,
  "y_pos": 0
}
```

#### QR Block Options

- **`data`** (required, string): The data to encode in the QR code (URL, text, etc.)
- **`size`** (optional, integer): Scaling factor for the QR code size. Default: 3
- **`x_pos`** (optional, integer): Horizontal position (-1 for left aligned, 0 for center, positive for right aligned margin)
- **`y_pos`** (optional, integer): Vertical offset in pixels from the normal position. Default: 0

### Horizontal Rule Block

Creates a horizontal line across the paper.

#### Format
```json
{
  "type": "hr"
}
```

### Feed Block

Adds vertical space to feed the paper.

#### Format
```json
{
  "type": "feed",
  "lines": 3
}
```

#### Feed Block Options

- **`lines`** (optional, integer): Number of lines to feed. Default: 1

## Complete Example

```json
{
  "blocks": [
    {
      "type": "text",
      "content": "Receipt Title",
      "font_size": 32,
      "font_weight": "Bold",
      "alignment": "center",
      "top_padding": 10
    },
    {
      "type": "hr"
    },
    {
      "type": "text",
      "content": "Item 1: $10.00",
      "alignment": "left"
    },
    {
      "type": "text",
      "content": "Item 2: $15.50",
      "alignment": "left"
    },
    {
      "type": "text",
      "content": "Total: $25.50",
      "font_size": 28,
      "font_weight": "Bold",
      "alignment": "right",
      "bottom_padding": 10
    },
    {
      "type": "hr"
    },
    {
      "type": "qr",
      "data": "https://example.com/order/12345",
      "size": 4,
      "x_pos": 0
    },
    {
      "type": "text",
      "content": "Scan QR for details",
      "alignment": "center"
    },
    {
      "type": "feed",
      "lines": 2
    }
  ]
}
```

## Paper Specifications

- **Width**: 384 dots (48mm at 8 dots/mm)
- **Default side margins**: 10 pixels
- **Top padding**: 0 pixels
- **Bottom padding**: 140 pixels

## Supported Fonts

The default font is "Noto Sans Malayalam", but you can use any font installed on the system by modifying the source code.