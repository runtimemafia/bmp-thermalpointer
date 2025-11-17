Here is a clean, complete **README.md** that documents the **entire plan** for your receipt engine project.
This can go directly in your GitHub repo as the main project README.

---

# 🧾 Receipt Engine (C, Raspberry Pi Zero 2 W)

### High-performance ESC/POS thermal receipt renderer with full Unicode (Malayalam/Hindi/Tamil), tables, images, QR, and layout engine — inspired by *react-thermal-printer*.

---

## 📌 Overview

This project is a **fast, lightweight, full-featured receipt rendering engine written in C**, optimized for **Raspberry Pi Zero 2 W** kiosks.

It supports:

* ✅ Unicode text (Malayalam, Hindi, Tamil, etc.)
* ✅ Automatic complex-script shaping
* ✅ Multi-line wrapping
* ✅ Tables with configurable columns
* ✅ Images & logos (PNG/JPG)
* ✅ QR codes
* ✅ Horizontal rules
* ✅ Line feeds & paper cuts
* ✅ ESC/POS raster output (GS v 0)
* ✅ Preview image generation
* 🔌 Output to serial/USB printers

This is the same capability provided by **react-thermal-printer**, but implemented at **native speed**, with **Cairo + Pango** for text shaping, **QRencode** for QR codes, and ESC/POS compliant output.

---

## 🔧 Why C + Pi Zero 2W?

* Fast rasterization
* Minimal overhead (no Python/Node runtime)
* Can run inside a tight kiosk environment
* Perfect for embedded printers (KP-628E, Epson, etc.)
* Reliable and deterministic receipt generation

---

# 🏗 Architecture

The engine is composed of several modules:

## 1. **JSON → Layout Blocks**

A receipt is defined using JSON:

```json
{
  "width_dots": 384,
  "font": "Noto Sans Malayalam 20",
  "blocks": [
    {"type":"text","align":"center","font_size":28,"text":"കമ്പനി നാമം"},
    {"type":"hr"},
    {"type":"text","align":"left","font_size":20,"text":"Invoice #12345\nDate: 2025-11-17"},
    {"type":"table","cols":[{"w":0.6},{"w":0.4}],
      "rows":[["Item","Price"],["Dance class","₹500"]]},
    {"type":"qr","data":"https://example.com/pay/12345","size":150},
    {"type":"feed","lines":3},
    {"type":"cut"}
  ]
}
```

Blocks currently supported:

* **text**
* **hr** (horizontal line)
* **table**
* **qr**
* **image** (soon)
* **feed**
* **cut**

---

## 2. **Layout Engine (Core)**

Uses:

* **Pango** for text shaping + wrapping
* **Cairo** for drawing
* **libqrencode** for QR codes

The engine renders all blocks into a **large ARGB32 Cairo surface**, positioned vertically using a cursor (`y` position).
This behaves like a small HTML layout engine.

---

## 3. **Bitmap Conversion**

Converts the ARGB32 surface into:

* **1-bit monochrome** (thermal printer compatible)
* MSB-left byte packing
* Optional future dithering

---

## 4. **ESC/POS Encoder**

Produces final ESC/POS raster bytes:

```
ESC @           ; initialize
ESC a 1         ; center align
GS v 0 m xL xH yL yH   ; raster image
{bitmap bytes}
ESC d n         ; feed
GS V 1          ; cut
```

Output is stored as `out.bin`.

---

## 5. **Printer Output**

You can send `out.bin` to any printer via:

### USB (raw)

```
sudo cat out.bin > /dev/usb/lp0
```

### Serial (RS232/TTL)

```
sudo dd if=out.bin of=/dev/ttyUSB0 bs=1
```

Planned:

* `--device` CLI flag to send automatically
* Automatic baud, parity, flow config

---

## 6. **Preview Rendering**

The engine writes `preview.png` for debugging.
This is visually identical to how it prints on thermal paper.

---

# 📂 Project Structure (planned)

```
/src
  core_print.c               # Core renderer + ESC/POS builder
  layout_text.c              # Pango text layout
  layout_table.c             # Table rendering
  layout_qr.c                # QR generation
  layout_image.c             # Image scaling + dither (future)
  escpos_encode.c            # GS v0, ESC *, other formats
  device_output.c            # serial/usb output
  job_queue.c                # (future) queued printing daemon

/include
  core.h
  layout.h
  escpos.h
  device.h

/tests
  sample_receipt.json
  test_table.json
  test_malayalam.json

/docs
  architecture.md
  printer_compat.md
  examples.md

CMakeLists.txt               # future
README.md                    # this file
```

---

# 🚀 Build Instructions

### Install dependencies:

```bash
sudo apt update
sudo apt install -y build-essential pkg-config \
  libcairo2-dev libpango1.0-dev libpangocairo-1.0-0 \
  libqrencode-dev libcjson-dev fonts-noto
```

### Build:

```bash
gcc `pkg-config --cflags pangocairo` -O2 -o core_print src/core_print.c \
  -lcjson -lqrencode `pkg-config --libs pangocairo`
```

---

# ▶️ Usage

### Convert JSON → preview.png + out.bin

```
./core_print receipt.json out.bin
```

### Print (USB)

```
sudo cat out.bin > /dev/usb/lp0
```

### Print (Serial)

```
sudo dd if=out.bin of=/dev/ttyUSB0 bs=1
```

---

# 🧱 Features: Current vs Planned

| Feature                                      | Status                   |
| -------------------------------------------- | ------------------------ |
| Unicode Text (Malayalam, Hindi etc)          | ✅ Done                   |
| Text Wrapping & Alignment                    | ✅ Done                   |
| Tables                                       | ✅ Basic                  |
| QR Codes                                     | ✅ Done                   |
| Horizontal Rule                              | ✅ Done                   |
| Feed & Cut                                   | ✅ Done                   |
| Preview Image PNG                            | ✅ Done                   |
| ESC/POS Raster Output                        | ✅ Done                   |
| printer device output                        | 🔜 Next                  |
| Images / Logos (PNG/JPG)                     | 🔜 Soon                  |
| Bitmap Dithering                             | 🔜 Optional              |
| NV Bitmap cache (logo stored inside printer) | 🔜 Later                 |
| Job Queue + Daemon                           | 🔜 Later                 |
| Node/Python bindings                         | 🔜 Optional              |
| Freetype+HarfBuzz engine (no Pango)          | 🔜 Optional Optimization |

---

# 🔮 Future: “Receipt Engine API” (C)

We will expose a clean C API:

```c
rt_engine_t *rt_engine_create(int width_dots);
rt_job_t *rt_job_from_json(const char *json);
rt_block_t *rt_job_add_text(rt_job_t*, ...);
rt_render_to_escpos(rt_engine_t*, rt_job_t*, uint8_t **buf, size_t *sz);
rt_send_to_device(rt_engine_t*, buf, sz, "/dev/usb/lp0");
```

Planned backends:

* `/dev/usb/lp*`
* `/dev/ttyUSB*`
* TCP network printers

---

# 🧪 Testing Strategy

* Compare `preview.png` against expected PNGs.
* Visual test on thermal printer.
* Validate QR codes with scanner.
* Stress-test with long tables.
* UTF-8 test suite (Malayalam/Tamil/Hindi paragraphs).

---

# 📜 License

MIT (to be confirmed)

---

# 🙋 Support

Ask any questions in Issues or Discussions.
This project is being actively developed for kiosk/embedded applications.

---

# ✔ Ready for Next Step

The **core renderer is done** (JSON → preview.png + out.bin).
Next suggested step:

➡️ **Add device output (`--device /dev/...`) so the engine prints directly without cat/dd.**

Let me know and I will implement that next.
