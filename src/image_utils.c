#include "printer_utils.h"

// Function to create a Cairo surface from an image file with aspect ratio preservation
cairo_surface_t* create_image_surface(const char *filename, int target_width, int target_height) {
    // Use cairo to load the image (this requires that Cairo was built with image support)
    cairo_surface_t *original_surface = cairo_image_surface_create_from_png(filename);
    
    // If it's not a PNG, try to load as other formats using a library like GdkPixbuf or STBI
    // For now, we'll check if the PNG loading worked
    if (original_surface == NULL || cairo_surface_status(original_surface) != CAIRO_STATUS_SUCCESS) {
        printf("Error: Could not load image file: %s\n", filename);
        return NULL;
    }
    
    // Get original image dimensions
    int original_width = cairo_image_surface_get_width(original_surface);
    int original_height = cairo_image_surface_get_height(original_surface);
    
    // Calculate dimensions while preserving aspect ratio
    double aspect_ratio = (double)original_width / original_height;
    
    if (target_width <= 0 && target_height <= 0) {
        // Use original dimensions
        target_width = original_width;
        target_height = original_height;
    } else if (target_width <= 0) {
        // Calculate width based on specified height while preserving aspect ratio
        target_width = (int)(target_height * aspect_ratio);
    } else if (target_height <= 0) {
        // Calculate height based on specified width while preserving aspect ratio
        target_height = (int)(target_width / aspect_ratio);
    }
    // If both are specified, use them as is (user's intention to stretch image)
    
    // Create a new surface with target dimensions
    cairo_surface_t *scaled_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, target_width, target_height);
    cairo_t *cr = cairo_create(scaled_surface);
    
    // Scale the original image to fit the target dimensions
    cairo_scale(cr, 
                (double)target_width / original_width, 
                (double)target_height / original_height);
    
    // Paint the original surface onto the new scaled surface
    cairo_set_source_surface(cr, original_surface, 0, 0);
    cairo_paint(cr);
    
    // Cleanup
    cairo_destroy(cr);
    cairo_surface_destroy(original_surface);
    
    return scaled_surface;
}

// Function to initialize default image format from JSON
void init_default_image_format_from_json(json_object *block, ImageFormat *format) {
    // Initialize with default values
    format->width = -1;  // Use original width or calculate from height
    format->height = -1; // Use original height or calculate from width
    format->x_pos = 0;   // Center
    format->y_pos = 0;   // Default position
    format->filename[0] = '\0'; // Empty filename
    format->scale = 1;   // Default scale
    
    // Override defaults with JSON values if present
    json_object *width, *height, *x_pos, *y_pos, *filename, *scale;
    
    if (json_object_object_get_ex(block, "width", &width) && json_object_is_type(width, json_type_int)) {
        format->width = json_object_get_int(width);
    }
    
    if (json_object_object_get_ex(block, "height", &height) && json_object_is_type(height, json_type_int)) {
        format->height = json_object_get_int(height);
    }
    
    if (json_object_object_get_ex(block, "x_pos", &x_pos) && json_object_is_type(x_pos, json_type_int)) {
        format->x_pos = json_object_get_int(x_pos);
    }
    
    if (json_object_object_get_ex(block, "y_pos", &y_pos) && json_object_is_type(y_pos, json_type_int)) {
        format->y_pos = json_object_get_int(y_pos);
    }
    
    if (json_object_object_get_ex(block, "filename", &filename) && json_object_is_type(filename, json_type_string)) {
        const char *filename_str = json_object_get_string(filename);
        strncpy(format->filename, filename_str, sizeof(format->filename) - 1);
        format->filename[sizeof(format->filename) - 1] = '\0';
    }
    // Also support "src" as an alias for filename
    else if (json_object_object_get_ex(block, "src", &filename) && json_object_is_type(filename, json_type_string)) {
        const char *filename_str = json_object_get_string(filename);
        strncpy(format->filename, filename_str, sizeof(format->filename) - 1);
        format->filename[sizeof(format->filename) - 1] = '\0';
    }
    
    if (json_object_object_get_ex(block, "scale", &scale) && json_object_is_type(scale, json_type_int)) {
        format->scale = json_object_get_int(scale);
    }
}

// Enhanced function to render image with formatting options
void render_image_to_surface_with_format(cairo_surface_t *surface, int base_y, ImageFormat *format) {
    if (format->filename[0] == '\0') {
        printf("Error: No image filename specified\n");
        return;
    }
    
    // Calculate target dimensions based on scale
    int target_width = format->width;
    int target_height = format->height;
    
    cairo_surface_t *image_surface = create_image_surface(format->filename, target_width, target_height);
    
    if (image_surface) {
        // Calculate image width and position
        int img_width = cairo_image_surface_get_width(image_surface);
        int img_height = cairo_image_surface_get_height(image_surface);
        
        // Apply scale if needed
        if (format->scale > 1) {
            img_width *= format->scale;
            img_height *= format->scale;
        }
        
        int x_pos = 0;
        
        if (format->x_pos == 0) {
            // Center the image horizontally
            x_pos = (PRINTER_WIDTH_DOTS - img_width) / 2;
        } else if (format->x_pos < 0) {
            // Left aligned with margin
            x_pos = -format->x_pos;  // Use the absolute value as left margin
        } else {
            // Right aligned with margin
            x_pos = PRINTER_WIDTH_DOTS - img_width - format->x_pos;
        }
        
        int y_pos = base_y + format->y_pos;
        
        cairo_t *cr = cairo_create(surface);
        
        // If scale > 1, we need to scale the image when drawing
        if (format->scale > 1) {
            cairo_scale(cr, format->scale, format->scale);
            // Adjust position to account for scaling
            cairo_set_source_surface(cr, image_surface, x_pos / format->scale, y_pos / format->scale);
        } else {
            cairo_set_source_surface(cr, image_surface, x_pos, y_pos);
        }
        
        cairo_paint(cr);
        cairo_destroy(cr);
        
        // Cleanup image surface
        cairo_surface_destroy(image_surface);
    } else {
        printf("Error: Could not render image from file: %s\n", format->filename);
    }
}