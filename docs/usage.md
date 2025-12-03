# Usage Manual

This manual covers everything you need to know to install, run, and use the Thermal Printer Engine.

## 1. Installation

### 1.1. System Requirements
The engine runs on **Linux**. It requires the following libraries:
*   **Cairo**: For 2D graphics.
*   **Pango**: For text layout.
*   **JSON-C**: For JSON parsing.
*   **LibQREncode**: For QR codes.
*   **GCC & Make**: For compilation.

### 1.2. Installing Dependencies (Debian/Ubuntu/Raspberry Pi OS)
Run the following command to install all necessary development headers and libraries:

```bash
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    pkg-config \
    libcairo2-dev \
    libpango1.0-dev \
    libjson-c-dev \
    libqrencode-dev
```

### 1.3. Compilation
Navigate to the project directory and run `make`.

```bash
cd thermalprinter
make clean
make
```

If successful, you will see a binary file named `poc_printer` in the directory.

## 2. Running the Engine

The engine is a command-line tool. It accepts a JSON input and an optional printer device path.

### Syntax
```bash
./poc_printer <INPUT> [PRINTER_DEVICE]
```

### Arguments
1.  **`INPUT`**: This can be either:
    *   A **file path** to a JSON file (e.g., `receipts/order_123.json`).
    *   A **raw JSON string** (e.g., `'{"blocks":[]}'`).
2.  **`PRINTER_DEVICE`** (Optional):
    *   The path to the character device file for your printer.
    *   Common paths: `/dev/usb/lp0`, `/dev/usb/lp1`, `/dev/ttyUSB0`.
    *   **If omitted**: The engine will generate the commands but save them to a file named `output.bin` in the current directory. It will also generate a `preview.png` image.

### Examples

**Scenario A: Testing without a printer**
```bash
./poc_printer tests/test_table.json
# Result: Check 'preview.png' to see what it would look like.
```

**Scenario B: Printing a file to a USB printer**
```bash
./poc_printer tests/test_table.json /dev/usb/lp0
```

**Scenario C: Printing from a script (passing JSON string)**
```bash
./poc_printer '{"blocks":[{"type":"text","content":"TEST PRINT"}]}' /dev/usb/lp0
```

## 3. JSON Format Reference

The input must be a valid JSON object with a root key `"blocks"`, which is an array of Block Objects.

```json
{
  "blocks": [
    { ... block 1 ... },
    { ... block 2 ... }
  ]
}
```

### 3.1. Common Properties
All blocks support the `type` property. Other properties depend on the type.

### 3.2. Block Type: `text`
Renders a paragraph of text.

| Property | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `type` | string | **Required** | Must be `"text"`. |
| `content` | string | **Required** | The text string to print. |
| `format` | object | `{}` | Formatting options. |

**Format Object:**
| Property | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `alignment` | string | `"left"` | `"left"`, `"center"`, or `"right"`. |
| `font_weight` | string | `"normal"` | `"normal"` or `"bold"`. |
| `font_size` | integer | `24` | Font size in pixels. |
| `bottom_padding`| integer | `0` | Space after this block (pixels). |

**Example:**
```json
{
    "type": "text",
    "content": "Store Title",
    "format": {
        "alignment": "center",
        "font_weight": "bold",
        "font_size": 32,
        "bottom_padding": 20
    }
}
```

### 3.3. Block Type: `table`
Renders a grid of data.

| Property | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `type` | string | **Required** | Must be `"table"`. |
| `columns` | array | **Required** | Array of Column Definitions. |
| `rows` | array | **Required** | 2D Array of strings (cell data). |
| `format` | object | `{}` | Table-wide formatting. |

**Column Definition:**
| Property | Type | Description |
| :--- | :--- | :--- |
| `header` | string | Text to display in the header row. |
| `width` | string | Width of column. Can be percentage (`"50%"`) or pixels (`"100"`). |
| `align` | string | Alignment for this column (`"left"`, `"right"`, `"center"`). |

**Format Object:**
| Property | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `cell_padding` | integer | `5` | Padding inside each cell. |
| `font_size` | integer | `18` | Font size for table content. |
| `border` | boolean | `false` | Quick switch to enable ALL borders. |
| `borders` | object | `null` | Granular border control (overrides `border`). |

**Borders Object:**
| Property | Type | Description |
| :--- | :--- | :--- |
| `top` | boolean | Top border of the table header. |
| `bottom` | boolean | Bottom border of the table. |
| `left` | boolean | Leftmost vertical border. |
| `right` | boolean | Rightmost vertical border. |
| `inner_horizontal`| boolean | Lines between rows. |
| `inner_vertical` | boolean | Lines between columns. |

**Example:**
```json
{
    "type": "table",
    "columns": [
        {"header": "Item", "width": "70%", "align": "left"},
        {"header": "Price", "width": "30%", "align": "right"}
    ],
    "rows": [
        ["Burger", "$5.00"],
        ["Fries", "$2.50"]
    ],
    "format": {
        "font_size": 18,
        "borders": {
            "bottom": true,
            "inner_horizontal": true
        }
    }
}
```

### 3.4. Block Type: `qr`
Generates a QR code.

| Property | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `type` | string | **Required** | Must be `"qr"`. |
| `data` | string | **Required** | The content to encode. |
| `size` | integer | `4` | Size of each QR module (pixel multiplier). |
| `align` | string | `"center"` | `"left"`, `"center"`, `"right"`. |

**Example:**
```json
{
    "type": "qr",
    "data": "https://myshop.com/order/123",
    "size": 5,
    "align": "center"
}
```

### 3.5. Block Type: `image`
Prints an image from the filesystem.

| Property | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `type` | string | **Required** | Must be `"image"`. |
| `path` | string | **Required** | Absolute path to the PNG/JPG file. |
| `width` | integer | `0` | Target width. If 0, uses original width (scaled down if too big). |
| `align` | string | `"center"` | `"left"`, `"center"`, `"right"`. |

**Example:**
```json
{
    "type": "image",
    "path": "/home/pi/logo.png",
    "width": 200
}
```

### 3.6. Block Type: `hr`
Draws a horizontal rule (line).

| Property | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `type` | string | **Required** | Must be `"hr"`. |

### 3.7. Block Type: `feed`
Feeds the paper without printing. Useful for adding space at the bottom of a receipt so it can be torn off.

| Property | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `type` | string | **Required** | Must be `"feed"`. |
| `lines` | integer | `1` | Number of lines to feed. |

## 4. Troubleshooting

### "Error: Failed to parse JSON input"
*   **Cause**: Your JSON syntax is invalid.
*   **Fix**: Use a JSON validator (like `jq` or an online tool) to check your input. Ensure you are escaping quotes properly if passing JSON as a command-line argument.

### "Failed to send to printer"
*   **Cause**: The device path (e.g., `/dev/usb/lp0`) is incorrect, or the user does not have permission to write to it.
*   **Fix**:
    *   Check if the printer is connected: `ls -l /dev/usb/`
    *   Check permissions: `sudo chmod 666 /dev/usb/lp0` or run the binary with `sudo`.

### "Text is too small/large"
*   **Fix**: Adjust the `font_size` in the `format` object of your text or table blocks.

### "Malayalam/Special characters not showing"
*   **Cause**: The system might be missing the required fonts.
*   **Fix**: Install fonts: `sudo apt-get install fonts-noto-core fonts-noto-ui-core`. The engine defaults to "Noto Sans Malayalam".
