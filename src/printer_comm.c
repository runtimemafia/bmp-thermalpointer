#include "printer_utils.h"

int send_to_usb_printer(unsigned char *data, int size, const char *device_path) {
    int fd = open(device_path, O_WRONLY);
    if (fd < 0) {
        perror("Failed to open printer device");
        return -1;
    }

    ssize_t bytes_written = write(fd, data, size);
    if (bytes_written != size) {
        perror("Failed to write all data to printer");
        close(fd);
        return -1;
    }

    // Force flush the data
    fsync(fd);

    close(fd);
    return 0; // Success
}

void save_to_file(unsigned char *data, int size) {
    FILE *fp = fopen("output.bin", "wb");
    if (fp) {
        fwrite(data, 1, size, fp);
        fclose(fp);
        printf("ESC/POS output written to output.bin\n");
    } else {
        printf("Could not write output.bin\n");
    }
}

void save_to_png(cairo_surface_t *surface, const char *filename) {
    cairo_status_t status = cairo_surface_write_to_png(surface, filename);
    if (status == CAIRO_STATUS_SUCCESS) {
        printf("PNG preview saved to %s\n", filename);
    } else {
        printf("Error saving PNG: %s\n", cairo_status_to_string(status));
    }
}