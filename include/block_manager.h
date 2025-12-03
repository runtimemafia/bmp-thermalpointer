#ifndef BLOCK_MANAGER_H
#define BLOCK_MANAGER_H

#include "block_interface.h"
#include <cairo.h>
#include <json-c/json.h>

// Initialize the block manager (register all block types)
void block_manager_init();

// Parse a JSON array of blocks into a list of Block objects
// Returns a pointer to an array of Block objects, terminated by a Block with
// NULL vtable. The caller is responsible for freeing the array and its contents
// using block_list_destroy.
Block *block_manager_parse(json_object *json_array);

// Calculate the total height of a list of blocks
int block_manager_calculate_height(Block *blocks, int width);

// Render a list of blocks to a surface
void block_manager_render(Block *blocks, cairo_surface_t *surface, int width);

// Destroy a list of blocks
void block_list_destroy(Block *blocks);

#endif // BLOCK_MANAGER_H
