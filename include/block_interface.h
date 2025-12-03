#ifndef BLOCK_INTERFACE_H
#define BLOCK_INTERFACE_H

#include <cairo.h>
#include <json-c/json.h>

// Context passed to render functions
typedef struct BlockContext {
    cairo_surface_t *surface; // The drawing surface
    cairo_t *cr;              // Cairo context
    int width;                // Printable width in dots
    int current_y;            // Current Y position to draw at
} BlockContext;

// Function pointers for block operations
typedef struct BlockVTable {
    const char *type_name;
    
    // Parse JSON into a block-specific data structure.
    // Returns a pointer to the private data, or NULL on failure.
    void* (*parse)(json_object *json);
    
    // Calculate height required for this block.
    // data: the private data returned by parse
    // width: the available width in dots
    int (*calculate_height)(void *data, int width);
    
    // Render the block to the context.
    // data: the private data returned by parse
    // ctx: the rendering context (surface, cr, width, current_y)
    void (*render)(void *data, BlockContext *ctx);
    
    // Free the block-specific data.
    void (*destroy)(void *data);
} BlockVTable;

// Generic Block wrapper
typedef struct Block {
    const BlockVTable *vtable;
    void *data;
} Block;

#endif // BLOCK_INTERFACE_H
