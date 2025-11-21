#!/usr/bin/env python3

# Read the file content
with open('/home/yash/code/temp/thermalprinter/src/poc_printer.c', 'r') as f:
    lines = f.readlines()

# Find the function and fix the problematic section
result_lines = []
i = 0
while i < len(lines):
    line = lines[i]
    
    # Look for the problematic section to replace
    if '// Calculate text position with alignment and padding' in line:
        # Skip the old problematic section
        while i < len(lines) and 'pango_cairo_show_layout(cr, layout);' not in lines[i]:
            i += 1
            if i >= len(lines):
                break
        # Add the corrected version
        result_lines.extend([
            "    // Set color to black\n",
            "    cairo_set_source_rgb(cr, 0, 0, 0);\n",
            "\n",
            "    // Position the layout at the left edge of the content area\n",
            "    // Pango will handle the alignment internally within the layout's width\n",
            "    int pos_x = base_x + format->left_padding;\n",
            "\n",
            "    // Move to position with top padding\n",
            "    cairo_move_to(cr, pos_x, base_y + format->top_padding);\n",
            "\n",
            "    // Render the layout\n",
            "    pango_cairo_show_layout(cr, layout);\n"
        ])
    else:
        result_lines.append(line)
        i += 1

# Write the updated content back to the file
with open('/home/yash/code/temp/thermalprinter/src/poc_printer.c', 'w') as f:
    f.writelines(result_lines)

print("File has been updated successfully!")