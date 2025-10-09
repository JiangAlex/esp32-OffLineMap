#include "lv_img_bin.h"
#include <Arduino.h>
#include <string.h>

// BIN file header structure
typedef struct {
    uint16_t width;
    uint16_t height;
} bin_header_t;

lv_obj_t * lv_img_bin_create(lv_obj_t * parent)
{
    return lv_img_create(parent);
}

void lv_img_bin_set_src(lv_obj_t * obj, const void * src)
{
    if (!obj || !src) return;
    
    const char* file_path = (const char*)src;
    Serial.printf("[lv_img_bin_set_src] Loading BIN file: %s\n", file_path);
    
    // Handle LVGL path prefix (A:)
    const char* actual_path = file_path;
    if (strncmp(file_path, "A:", 2) == 0) {
        actual_path = file_path + 2; // Skip "A:" prefix
        Serial.printf("[lv_img_bin_set_src] Adjusted path: %s\n", actual_path);
    }
    
    // Open file
    lv_fs_file_t file;
    lv_fs_res_t res = lv_fs_open(&file, actual_path, LV_FS_MODE_RD);
    if (res != LV_FS_RES_OK) {
        Serial.printf("[lv_img_bin_set_src] ❌ Failed to open file: %d\n", res);
        return;
    }
    
    // Get file size
    uint32_t file_size;
    lv_fs_seek(&file, 0, LV_FS_SEEK_END);
    lv_fs_tell(&file, &file_size);
    lv_fs_seek(&file, 0, LV_FS_SEEK_SET);
    
    Serial.printf("[lv_img_bin_set_src] File size: %ld bytes\n", file_size);
    
    // Skip 4-byte header and assume fixed 256x256 tile size
    uint32_t header_size = 4;
    uint32_t tile_width = 256;
    uint32_t tile_height = 256;
    uint32_t actual_data_size = file_size - header_size;
    uint32_t expected_data_size = tile_width * tile_height * 2; // RGB565 = 2 bytes per pixel
    
    Serial.printf("[lv_img_bin_set_src] Assuming tile size: %ux%u\n", tile_width, tile_height);
    Serial.printf("[lv_img_bin_set_src] Data size: %ld bytes (expected: %u)\n", 
                 actual_data_size, expected_data_size);
    
    if (actual_data_size != expected_data_size) {
        Serial.printf("[lv_img_bin_set_src] ⚠ Size mismatch: expected %u, got %ld\n", 
                     expected_data_size, actual_data_size);
    }
    
    // Allocate memory for image data
    uint8_t* img_data = (uint8_t*)malloc(file_size);
    if (!img_data) {
        Serial.printf("[lv_img_bin_set_src] ❌ Failed to allocate memory\n");
        lv_fs_close(&file);
        return;
    }
    
    // Read entire file
    uint32_t read_bytes;
    lv_fs_read(&file, img_data, file_size, &read_bytes);
    lv_fs_close(&file);
    
    if (read_bytes != file_size) {
        Serial.printf("[lv_img_bin_set_src] ❌ Failed to read file data\n");
        free(img_data);
        return;
    }
    
    // Create static image descriptor (must persist)
    static lv_img_dsc_t* img_descriptors[10] = {0}; // Support up to 10 images
    static int desc_index = 0;
    
    // Find next available descriptor slot
    int current_desc = desc_index;
    desc_index = (desc_index + 1) % 10;
    
    // Free previous descriptor if exists
    if (img_descriptors[current_desc]) {
        free((void*)img_descriptors[current_desc]->data);
        free(img_descriptors[current_desc]);
    }
    
    // Allocate new descriptor
    img_descriptors[current_desc] = (lv_img_dsc_t*)malloc(sizeof(lv_img_dsc_t));
    if (!img_descriptors[current_desc]) {
        Serial.printf("[lv_img_bin_set_src] ❌ Failed to allocate image descriptor\n");
        free(img_data);
        return;
    }
    
    // Allocate separate memory for image data (without header)
    uint8_t* pure_img_data = (uint8_t*)malloc(actual_data_size);
    if (!pure_img_data) {
        Serial.printf("[lv_img_bin_set_src] ❌ Failed to allocate image data memory\n");
        free(img_data);
        free(img_descriptors[current_desc]);
        img_descriptors[current_desc] = NULL;
        return;
    }
    
    // Copy image data (skip header)
    memcpy(pure_img_data, img_data + header_size, actual_data_size);
    free(img_data); // No longer needed
    
    // Set up image descriptor
    img_descriptors[current_desc]->header.always_zero = 0;
    img_descriptors[current_desc]->header.w = tile_width;
    img_descriptors[current_desc]->header.h = tile_height;
    img_descriptors[current_desc]->header.cf = LV_IMG_CF_TRUE_COLOR; // RGB565
    img_descriptors[current_desc]->data_size = actual_data_size;
    img_descriptors[current_desc]->data = pure_img_data;
    
    Serial.printf("[lv_img_bin_set_src] ✓ Image descriptor created: %ux%u, data_size=%u, ptr=%p\n", 
                 img_descriptors[current_desc]->header.w, 
                 img_descriptors[current_desc]->header.h, 
                 img_descriptors[current_desc]->data_size,
                 img_descriptors[current_desc]);
    
    // Set image source directly to descriptor
    lv_img_set_src(obj, img_descriptors[current_desc]);
    
    Serial.printf("[lv_img_bin_set_src] ✓ Image source set successfully\n");
}