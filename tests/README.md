# Thermal Printer Test Suite

This directory contains comprehensive test files for the thermal printer project. Each test file is designed to validate specific features and capabilities of the printer.

## Test Files

### `comprehensive_features_test.json`
A general test file that covers all major features in both English and Malayalam.

### `feature_verification_suite.json`
A detailed, enterprise-grade test suite that validates each feature systematically:

#### English Language Tests:
- **Text Alignment Verification**: Tests left, center, and right alignment
- **Font Size Verification**: Tests multiple font sizes (12px to 48px)
- **Font Weight Verification**: Tests Light, Thin, and Bold weights
- **Padding Verification**: Tests all padding combinations (top, bottom, left, right)
- **QR Code Verification**: Tests different QR code sizes (2 to 4)
- **QR Code Positioning**: Tests left-aligned, centered, and right-aligned QR codes
- **Mixed Script Verification**: Tests English with Malayalam text

#### Malayalam Language Tests:
- All the above features but with Malayalam text
- Proper rendering of complex scripts
- Multilingual support validation

#### Additional Features:
- Horizontal rules (hr)
- Line feeds
- Combined formatting options
- Number system testing (Arabic and Malayalam numerals)

### `image_support_test.json`
A test file specifically designed to validate image support functionality:

#### Image Support Tests:
- **Basic Image Insertion**: Tests inserting PNG images into print content
- **Image Sizing**: Tests custom width and height specifications
- **Image Scaling**: Tests scale factor functionality (2x, etc.)
- **Image Positioning**: Tests left, center, and right alignment for images
- **Mixed Content**: Tests images combined with text and other elements
- **Cross-Language Compatibility**: Tests images with both English and Malayalam text

### `aspect_ratio_test.json`
A test file specifically designed to validate aspect ratio preservation:

#### Aspect Ratio Tests:
- **Width-Based Sizing**: When only width is specified, height is calculated to preserve aspect ratio
- **Height-Based Sizing**: When only height is specified, width is calculated to preserve aspect ratio
- **Explicit Dimensions**: When both width and height are specified, image is stretched to fit
- **Scaling Integration**: Tests that aspect ratio calculations work with scaling factors

## Running Tests

To run any test file, use the following command:

```bash
./poc_printer tests/[test-file-name].json
```

This will generate:
- `output.bin` - The ESC/POS command file for the printer
- `preview.png` - A visual preview of the print output

## Test Structure

Each test file follows a JSON structure with the following block types:
- `"text"`: For text content with formatting options
- `"qr"`: For QR code generation with sizing and positioning
- `"image"`: For image insertion with sizing and positioning
- `"hr"`: For horizontal rules
- `"feed"`: For line feeds

## Validation Criteria

Each test includes:
- Clear labeling of test categories
- Verification of formatting options
- Visual confirmation through PNG preview
- Multiple language support validation
- Feature-specific test cases
- Edge case scenarios
- Proper handling of various numeric systems
- Image rendering accuracy