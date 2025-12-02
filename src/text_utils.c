#include "printer_utils.h"

// Function to initialize default text format
void init_default_text_format(TextFormat *format) {
    format->font_size = DEFAULT_FONT_SIZE;
    strcpy(format->font_weight, DEFAULT_FONT_WEIGHT);
    strcpy(format->alignment, "left");
    format->top_padding = 0;
    format->bottom_padding = 0;
    format->left_padding = 0;
    format->right_padding = 0;
}

// Function to initialize default QR format
void init_default_qr_format(QRFormat *format) {
    format->size = 3;  // default scale
    format->x_pos = 0; // center
    format->y_pos = 0;
}

// Function to initialize default text format from JSON
void init_default_text_format_from_json(json_object *block, TextFormat *format) {
    init_default_text_format(format);

    // Override defaults with JSON values if present
    json_object *font_size, *font_weight, *alignment, *top_padding, *bottom_padding, *left_padding, *right_padding;

    if (json_object_object_get_ex(block, "font_size", &font_size) && json_object_is_type(font_size, json_type_int)) {
        format->font_size = json_object_get_int(font_size);
    }

    if (json_object_object_get_ex(block, "font_weight", &font_weight) && json_object_is_type(font_weight, json_type_string)) {
        const char *weight_str = json_object_get_string(font_weight);
        strncpy(format->font_weight, weight_str, sizeof(format->font_weight) - 1);
        format->font_weight[sizeof(format->font_weight) - 1] = '\0';
    }

    if (json_object_object_get_ex(block, "alignment", &alignment) && json_object_is_type(alignment, json_type_string)) {
        const char *align_str = json_object_get_string(alignment);
        strncpy(format->alignment, align_str, sizeof(format->alignment) - 1);
        format->alignment[sizeof(format->alignment) - 1] = '\0';
    }

    if (json_object_object_get_ex(block, "top_padding", &top_padding) && json_object_is_type(top_padding, json_type_int)) {
        format->top_padding = json_object_get_int(top_padding);
    }

    if (json_object_object_get_ex(block, "bottom_padding", &bottom_padding) && json_object_is_type(bottom_padding, json_type_int)) {
        format->bottom_padding = json_object_get_int(bottom_padding);
    }

    if (json_object_object_get_ex(block, "left_padding", &left_padding) && json_object_is_type(left_padding, json_type_int)) {
        format->left_padding = json_object_get_int(left_padding);
    }

    if (json_object_object_get_ex(block, "right_padding", &right_padding) && json_object_is_type(right_padding, json_type_int)) {
        format->right_padding = json_object_get_int(right_padding);
    }
}

// Function to initialize default QR format from JSON
void init_default_qr_format_from_json(json_object *block, QRFormat *format) {
    init_default_qr_format(format);

    // Override defaults with JSON values if present
    json_object *size, *x_pos, *y_pos;

    if (json_object_object_get_ex(block, "size", &size) && json_object_is_type(size, json_type_int)) {
        format->size = json_object_get_int(size);
    }

    if (json_object_object_get_ex(block, "x_pos", &x_pos) && json_object_is_type(x_pos, json_type_int)) {
        format->x_pos = json_object_get_int(x_pos);
    }

    if (json_object_object_get_ex(block, "y_pos", &y_pos) && json_object_is_type(y_pos, json_type_int)) {
        format->y_pos = json_object_get_int(y_pos);
    }
}

// Function to calculate the height required for the text with custom format
int calculate_text_height_with_format(const char *text, int width, TextFormat *format) {
    cairo_surface_t *dummy_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, 100);
    cairo_t *cr = cairo_create(dummy_surface);

    PangoLayout *layout = pango_cairo_create_layout(cr);

    // Create font string with configurable weight and size from format
    char font_string[256];
    snprintf(font_string, sizeof(font_string), "%s %s %d", DEFAULT_FONT_NAME, format->font_weight, format->font_size);
    PangoFontDescription *font_desc = pango_font_description_from_string(font_string);

    pango_layout_set_font_description(layout, font_desc);
    pango_layout_set_text(layout, text, -1);

    // Calculate available width for text (full width minus margins and padding)
    int text_width = width - format->left_padding - format->right_padding;
    pango_layout_set_width(layout, text_width * PANGO_SCALE);
    pango_layout_set_wrap(layout, PANGO_WRAP_WORD_CHAR);

    int layout_width, layout_height;
    pango_layout_get_pixel_size(layout, &layout_width, &layout_height);

    // Calculate total height including padding
    int total_height = layout_height + format->top_padding + format->bottom_padding;

    // Cleanup
    pango_font_description_free(font_desc);
    g_object_unref(layout);
    cairo_destroy(cr);
    cairo_surface_destroy(dummy_surface);

    return total_height;
}

// Enhanced function to render text with formatting options
void render_text_to_surface_with_format(cairo_surface_t *surface, const char *text, int base_x, int base_y, TextFormat *format) {
    cairo_t *cr = cairo_create(surface);

    // Set up Pango for text rendering
    PangoLayout *layout = pango_cairo_create_layout(cr);

    // Create font string with configurable weight and size from format
    char font_string[256];
    snprintf(font_string, sizeof(font_string), "%s %s %d", DEFAULT_FONT_NAME, format->font_weight, format->font_size);
    PangoFontDescription *font_desc = pango_font_description_from_string(font_string);

    pango_layout_set_font_description(layout, font_desc);
    pango_layout_set_text(layout, text, -1);

    // Calculate available width for text (full width minus margins and padding)
    int text_width = PRINTER_WIDTH_DOTS - (2 * base_x) - format->left_padding - format->right_padding;
    pango_layout_set_width(layout, text_width * PANGO_SCALE);
    pango_layout_set_wrap(layout, PANGO_WRAP_WORD_CHAR);

    // Set alignment for Pango to handle text alignment within the layout box
    PangoAlignment alignment = PANGO_ALIGN_LEFT;
    if (strcmp(format->alignment, "center") == 0) {
        alignment = PANGO_ALIGN_CENTER;
    } else if (strcmp(format->alignment, "right") == 0) {
        alignment = PANGO_ALIGN_RIGHT;
    }
    pango_layout_set_alignment(layout, alignment);

    // Set color to black
    cairo_set_source_rgb(cr, 0, 0, 0);

    // Position the layout at the left edge of the content area
    // Pango will handle the alignment internally within the layout's width
    int pos_x = base_x + format->left_padding;

    // Move to position with top padding
    cairo_move_to(cr, pos_x, base_y + format->top_padding);

    // Render the layout
    pango_cairo_show_layout(cr, layout);

    // Cleanup
    pango_font_description_free(font_desc);
    g_object_unref(layout);
    cairo_destroy(cr);
}

// Basic function to render text without formatting
int calculate_text_height(const char *text, int width) {
    cairo_surface_t *dummy_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, 100);
    cairo_t *cr = cairo_create(dummy_surface);

    PangoLayout *layout = pango_cairo_create_layout(cr);

    // Create font string with configurable weight and size
    char font_string[256];
    snprintf(font_string, sizeof(font_string), "%s %s %d", DEFAULT_FONT_NAME, DEFAULT_FONT_WEIGHT, DEFAULT_FONT_SIZE);
    PangoFontDescription *font_desc = pango_font_description_from_string(font_string);

    pango_layout_set_font_description(layout, font_desc);
    pango_layout_set_text(layout, text, -1);
    pango_layout_set_width(layout, width * PANGO_SCALE);
    pango_layout_set_wrap(layout, PANGO_WRAP_WORD_CHAR);

    int text_width, text_height;
    pango_layout_get_pixel_size(layout, &text_width, &text_height);

    // Cleanup
    pango_font_description_free(font_desc);
    g_object_unref(layout);
    cairo_destroy(cr);
    cairo_surface_destroy(dummy_surface);

    return text_height;
}

void render_text_to_surface(cairo_surface_t *surface, const char *text, int x, int y) {
    cairo_t *cr = cairo_create(surface);

    // Set up Pango for text rendering
    PangoLayout *layout = pango_cairo_create_layout(cr);

    // Create font string with configurable weight and size
    char font_string[256];
    snprintf(font_string, sizeof(font_string), "%s %s %d", DEFAULT_FONT_NAME, DEFAULT_FONT_WEIGHT, DEFAULT_FONT_SIZE);
    PangoFontDescription *font_desc = pango_font_description_from_string(font_string);

    pango_layout_set_font_description(layout, font_desc);
    pango_layout_set_text(layout, text, -1);

    // Calculate available width for text (full width minus margins)
    int text_width = PRINTER_WIDTH_DOTS - (2 * x); // Subtract left and right margins
    pango_layout_set_width(layout, text_width * PANGO_SCALE);
    pango_layout_set_wrap(layout, PANGO_WRAP_WORD_CHAR);

    // Set color to black
    cairo_set_source_rgb(cr, 0, 0, 0);

    // Move to position
    cairo_move_to(cr, x, y);

    // Render the layout
    pango_cairo_show_layout(cr, layout);

    // Cleanup
    pango_font_description_free(font_desc);
    g_object_unref(layout);
    cairo_destroy(cr);
}