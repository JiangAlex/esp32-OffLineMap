#include "LiveMapView.h"
#include "App/Config/Config.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <Arduino.h>

using namespace Page;

#ifdef MAP_FORMAT_BIN
// For BIN format maps, use custom BIN decoder
#include "App/Utils/lv_img_bin/lv_img_bin.h"
#  define TILE_IMG_CREATE  lv_img_bin_create
#  define TILE_IMG_SET_SRC lv_img_bin_set_src
#else
// For PNG format maps, use PNG decoder
#if CONFIG_MAP_IMG_PNG_ENABLE
#include "Utils/lv_img_png/lv_img_png.h"
#  define TILE_IMG_CREATE  lv_img_png_create
#  define TILE_IMG_SET_SRC lv_img_png_set_src
#else
#  define TILE_IMG_CREATE  lv_img_create
#  define TILE_IMG_SET_SRC lv_img_set_src
#endif
#endif

void LiveMapView::Create(lv_obj_t* root, uint32_t tileNum)
{
    lv_obj_set_style_bg_color(root, lv_color_white(), 0);

    lv_obj_t* label = lv_label_create(root);
    lv_obj_center(label);
    lv_obj_set_style_text_font(label, ResourcePool::GetFont("bahnschrift_17"), 0);
    lv_label_set_text(label, "LOADING...");
    ui.labelInfo = label;

    Style_Create();
    Map_Create(root, tileNum);
    ZoomCtrl_Create(root);
    SportInfo_Create(root);
}

void LiveMapView::Delete()
{
    if (ui.track.lineTrack)
    {
        delete ui.track.lineTrack;
        ui.track.lineTrack = nullptr;
    }

    if (ui.map.imgTiles)
    {
        lv_mem_free(ui.map.imgTiles);
        ui.map.imgTiles = nullptr;
    }

    lv_style_reset(&ui.styleCont);
    lv_style_reset(&ui.styleLabel);
    lv_style_reset(&ui.styleLine);
}

void LiveMapView::Style_Create()
{
    lv_style_init(&ui.styleCont);
    lv_style_set_bg_color(&ui.styleCont, lv_color_black());
    lv_style_set_bg_opa(&ui.styleCont, LV_OPA_60);
    lv_style_set_radius(&ui.styleCont, 6);
    lv_style_set_shadow_width(&ui.styleCont, 10);
    lv_style_set_shadow_color(&ui.styleCont, lv_color_black());

    lv_style_init(&ui.styleLabel);
    lv_style_set_text_font(&ui.styleLabel, ResourcePool::GetFont("bahnschrift_17"));
    lv_style_set_text_color(&ui.styleLabel, lv_color_white());

    lv_style_init(&ui.styleLine);
    lv_style_set_line_color(&ui.styleLine, lv_color_hex(0xff931e));
    lv_style_set_line_width(&ui.styleLine, 5);
    lv_style_set_line_opa(&ui.styleLine, LV_OPA_COVER);
    lv_style_set_line_rounded(&ui.styleLine, true);
}

void LiveMapView::Map_Create(lv_obj_t* par, uint32_t tileNum)
{
    lv_obj_t* cont = lv_obj_create(par);
    lv_obj_remove_style_all(cont);
#if CONFIG_LIVE_MAP_DEBUG_ENABLE
    lv_obj_set_style_outline_color(cont, lv_palette_main(LV_PALETTE_BLUE), 0);
    lv_obj_set_style_outline_width(cont, 2, 0);
#endif
    ui.map.cont = cont;

    ui.map.imgTiles = (lv_obj_t**)lv_mem_alloc(tileNum * sizeof(lv_obj_t*));
    ui.map.tileNum = tileNum;

    for (uint32_t i = 0; i < tileNum; i++)
    {
        lv_obj_t* img = TILE_IMG_CREATE(cont);
        lv_obj_remove_style_all(img);
        ui.map.imgTiles[i] = img;
    }

    Track_Create(cont);

    lv_obj_t* img = lv_img_create(cont);
    lv_img_set_src(img, ResourcePool::GetImage("gps_arrow_dark"));

    lv_img_t* imgOri = (lv_img_t*)img;
    lv_obj_set_pos(img, -imgOri->w, -imgOri->h);
    ui.map.imgArrow = img;
}

void LiveMapView::SetMapTile(uint32_t tileSize, uint32_t widthCnt)
{
    uint32_t tileNum = ui.map.tileNum;

    lv_coord_t width = (lv_coord_t)(tileSize * widthCnt);
    lv_coord_t height = (lv_coord_t)(tileSize * (ui.map.tileNum / widthCnt));

    lv_obj_set_size(ui.map.cont, width, height);

    for (uint32_t i = 0; i < tileNum; i++)
    {
        lv_obj_t* img = ui.map.imgTiles[i];

        lv_obj_set_size(img, tileSize, tileSize);

        lv_coord_t x = (i % widthCnt) * tileSize;
        lv_coord_t y = (i / widthCnt) * tileSize;
        
        lv_obj_set_pos(img, x, y);
    }
}

void LiveMapView::SetMapTileSrc(uint32_t index, const char* src)
{
    if (index >= ui.map.tileNum)
    {
        printf("[SetMapTileSrc] Error: index %u >= tileNum %u\n", index, ui.map.tileNum);
        return;
    }
    
    // Check if file exists
    lv_fs_file_t file;
    lv_fs_res_t res = lv_fs_open(&file, src, LV_FS_MODE_RD);
    if (res == LV_FS_RES_OK)
    {
        lv_fs_close(&file);
        printf("[SetMapTileSrc] Loading image tile %u from: %s\n", index, src);
        
        // Check file details for debugging using LVGL FS
        lv_fs_file_t debug_file;
        lv_fs_res_t debug_res = lv_fs_open(&debug_file, src, LV_FS_MODE_RD);
        if (debug_res == LV_FS_RES_OK) {
            // Get file size
            uint32_t file_size;
            lv_fs_seek(&debug_file, 0, LV_FS_SEEK_END);
            lv_fs_tell(&debug_file, &file_size);
            lv_fs_seek(&debug_file, 0, LV_FS_SEEK_SET);
            
            // Read file signature to verify format
            unsigned char file_sig[8];
            uint32_t read_bytes;
            lv_fs_read(&debug_file, file_sig, 8, &read_bytes);
            lv_fs_close(&debug_file);
            
#ifdef MAP_FORMAT_BIN
            printf("[SetMapTileSrc] File size=%u bytes, BIN sig=[%02X %02X %02X %02X]\n", 
                   file_size, file_sig[0], file_sig[1], file_sig[2], file_sig[3]);
            
            // Analyze BIN file structure
            if (read_bytes >= 8) {
                uint16_t *header16 = (uint16_t*)file_sig;
                uint32_t *header32 = (uint32_t*)file_sig;
                printf("[SetMapTileSrc] BIN header analysis:\n");
                printf("[SetMapTileSrc]   First 4 bytes as uint16: %u, %u\n", header16[0], header16[1]);
                printf("[SetMapTileSrc]   First 4 bytes as uint32: %u\n", header32[0]);
                printf("[SetMapTileSrc]   Expected tile size: 256x256 = 65536 pixels\n");
                printf("[SetMapTileSrc]   Expected RGB565 size: 65536 * 2 = 131072 bytes\n");
                printf("[SetMapTileSrc]   Actual file size: %u bytes\n", file_size);
                
                if (file_size == 131072) {
                    printf("[SetMapTileSrc]   ✓ File size matches raw RGB565 data (no header)\n");
                } else if (file_size == 131076) {
                    printf("[SetMapTileSrc]   ✓ File size matches RGB565 data + 4-byte header\n");
                } else {
                    printf("[SetMapTileSrc]   ⚠ Unexpected file size\n");
                }
            }
            printf("[SetMapTileSrc] ✓ BIN format file detected\n");
#else
            printf("[SetMapTileSrc] File size=%u bytes, PNG sig=[%02X %02X %02X %02X]\n", 
                   file_size, file_sig[0], file_sig[1], file_sig[2], file_sig[3]);
            
            // Check PNG signature (should be: 89 50 4E 47)
            if (read_bytes >= 4 && file_sig[0] == 0x89 && file_sig[1] == 0x50 && 
                file_sig[2] == 0x4E && file_sig[3] == 0x47) {
                printf("[SetMapTileSrc] ✓ Valid PNG signature\n");
            } else {
                printf("[SetMapTileSrc] ⚠ Invalid PNG signature\n");
            }
#endif
        } else {
            printf("[SetMapTileSrc] ⚠ Cannot open file for debugging: %d\n", debug_res);
        }
        
        // Set the image source
        printf("[SetMapTileSrc] Setting image source for tile %u: %s\n", index, src);
        
        // Try different path formats for LVGL
        char lvgl_path[256];
        
        // Try with LVGL filesystem prefix
        if (src[0] == '/') {
            // Try A: drive prefix (common in LVGL)
            snprintf(lvgl_path, sizeof(lvgl_path), "A:%s", src);
            printf("[SetMapTileSrc] Trying LVGL path: %s\n", lvgl_path);
            TILE_IMG_SET_SRC(ui.map.imgTiles[index], lvgl_path);
        } else {
            // Use original path
            printf("[SetMapTileSrc] Using original path: %s\n", src);
            TILE_IMG_SET_SRC(ui.map.imgTiles[index], src);
        }
        
        // Process LVGL tasks to allow image loading
        lv_task_handler();
        
        // Wait a moment for PNG decoding
        vTaskDelay(pdMS_TO_TICKS(10));
        
        // Check if image loaded successfully
        const void* img_src = lv_img_get_src(ui.map.imgTiles[index]);
        printf("[SetMapTileSrc] Image source pointer after loading: %p\n", img_src);
        
        if (img_src != NULL)
        {
#ifdef MAP_FORMAT_BIN
            // For BIN format, check if we got an image descriptor pointer
            if ((uintptr_t)img_src > 0x1000) { // Valid memory address
                printf("[SetMapTileSrc] ✓ BIN image descriptor loaded: %p\n", img_src);
                
                // Image loaded successfully - force display update
                lv_obj_clear_flag(ui.map.imgTiles[index], LV_OBJ_FLAG_HIDDEN);
                
                // Force refresh the image object
                lv_obj_invalidate(ui.map.imgTiles[index]);
                
                // Force LVGL to process the image immediately
                lv_task_handler();
                
                printf("[SetMapTileSrc] BIN tile %u forced refresh\n", index);
            } else {
                printf("[SetMapTileSrc] ⚠ BIN decode failed for tile %u, invalid descriptor\n", index);
            }
#else
            // Check if the source matches what we set for PNG
            if (img_src == src || strcmp((const char*)img_src, src) == 0) {
                printf("[SetMapTileSrc] ✓ Image source matches: %s\n", (const char*)img_src);
            } else {
                printf("[SetMapTileSrc] ⚠ Image source mismatch: expected=%s, got=%s\n", src, (const char*)img_src);
            }
            // Try to get image header info to verify decoding
            lv_img_header_t header;
            lv_res_t header_res = lv_img_decoder_get_info(src, &header);
            
            printf("[SetMapTileSrc] PNG decode attempt for tile %u, result=%d\n", index, header_res);
            
            // In LVGL v8.x, 0 typically means success (LV_RES_OK)
            if (header_res == 0)
            {
                printf("[SetMapTileSrc] ✓ PNG loaded successfully for tile %u - %dx%d, cf=%d\n", 
                       index, header.w, header.h, header.cf);
                
                // Image loaded successfully - force display update
                lv_obj_clear_flag(ui.map.imgTiles[index], LV_OBJ_FLAG_HIDDEN);
                
                // Force refresh the image object
                lv_obj_invalidate(ui.map.imgTiles[index]);
                
                // Force LVGL to process the image immediately
                lv_task_handler();
                
                printf("[SetMapTileSrc] PNG tile %u forced refresh with blue border\n", index);
            }
            else
            {
                printf("[SetMapTileSrc] ⚠ PNG decode failed for tile %u (error=%d), using fallback\n", index, header_res);
                
                // Check file size for debugging
                FILE* f = fopen(src, "rb");
                if (f) {
                    fseek(f, 0, SEEK_END);
                    long size = ftell(f);
                    fclose(f);
                    printf("[SetMapTileSrc] File size: %ld bytes\n", size);
                }
                // Decode failed, fall back to colored placeholder
                CreateFallbackTile(index, src);
            }
#endif
        }
        else
        {
            printf("[SetMapTileSrc] ✗ Failed to set image source for tile %u\n", index);
            CreateFallbackTile(index, src);
        }
        
        printf("[SetMapTileSrc] Tile %u: pos(%d,%d) size(%d,%d)\n", index,
               lv_obj_get_x(ui.map.imgTiles[index]), lv_obj_get_y(ui.map.imgTiles[index]),
               lv_obj_get_width(ui.map.imgTiles[index]), lv_obj_get_height(ui.map.imgTiles[index]));
    }
    else
    {
        printf("[SetMapTileSrc] File not found: %s\n", src);
        // Use a default placeholder when file not found
        TILE_IMG_SET_SRC(ui.map.imgTiles[index], NULL);
        
        // Set a background color to indicate missing tile
        lv_obj_set_style_bg_color(ui.map.imgTiles[index], lv_color_hex(0xE0E0E0), 0);
        lv_obj_set_style_bg_opa(ui.map.imgTiles[index], LV_OPA_COVER, 0);
        
        // Add a label to show tile coordinates for debugging
        lv_obj_t* label = lv_label_create(ui.map.imgTiles[index]);
        lv_obj_center(label);
        lv_obj_set_style_text_color(label, lv_color_hex(0x666666), 0);
        lv_label_set_text_fmt(label, "T%u\nMissing", index);
    }
}

void LiveMapView::SetArrowTheme(const char* theme)
{
    char buf[32];
    snprintf(buf, sizeof(buf), "gps_arrow_%s", theme);

    const void* src = ResourcePool::GetImage(buf);

    if (src == nullptr)
    {
        ResourcePool::GetImage("gps_arrow_default");
    }

    lv_img_set_src(ui.map.imgArrow, src);
}

void LiveMapView::SetLineActivePoint(lv_coord_t x, lv_coord_t y)
{
    lv_point_t end_point;
    if (!ui.track.lineTrack->get_end_point(&end_point))
    {
        return;
    }

    ui.track.pointActive[0] = end_point;
    ui.track.pointActive[1].x = x;
    ui.track.pointActive[1].y = y;
    lv_line_set_points(ui.track.lineActive, ui.track.pointActive, 2);
}

void LiveMapView::ZoomCtrl_Create(lv_obj_t* par)
{
    lv_obj_t* cont = lv_obj_create(par);
    lv_obj_remove_style_all(cont);
    lv_obj_add_style(cont, &ui.styleCont, 0);
    lv_obj_set_style_opa(cont, LV_OPA_COVER, 0);
    lv_obj_set_size(cont, 50, 30);
    lv_obj_set_pos(cont, lv_obj_get_style_width(par, 0) - lv_obj_get_style_width(cont, 0) + 5, 40);
    ui.zoom.cont = cont;

    static const lv_style_prop_t prop[] =
    {
        LV_STYLE_X,
        LV_STYLE_OPA,
        LV_STYLE_PROP_INV
    };
    static lv_style_transition_dsc_t tran;
    lv_style_transition_dsc_init(&tran, prop, lv_anim_path_ease_out, 200, 0, nullptr);
    lv_obj_set_style_x(cont, lv_obj_get_style_width(par, 0), LV_STATE_USER_1);
    lv_obj_set_style_opa(cont, LV_OPA_TRANSP, LV_STATE_USER_1);
    lv_obj_set_style_transition(cont, &tran, LV_STATE_USER_1);
    lv_obj_set_style_transition(cont, &tran, LV_STATE_DEFAULT);
    lv_obj_add_state(cont, LV_STATE_USER_1);

    lv_obj_t* label = lv_label_create(cont);
    lv_obj_add_style(label, &ui.styleLabel, 0);
    lv_obj_align(label, LV_ALIGN_CENTER, -2, 0);
    lv_label_set_text(label, "--");
    ui.zoom.labelInfo = label;

    lv_obj_t* slider = lv_slider_create(cont);
    lv_obj_remove_style_all(slider);
    // lv_obj_set_style_opa(slider, LV_OPA_COVER, 0); // #new
    lv_slider_set_value(slider, CONFIG_LIVE_MAP_LEVEL_DEFAULT, LV_ANIM_OFF);  // Use config default
    ui.zoom.slider = slider;
}

void LiveMapView::SportInfo_Create(lv_obj_t* par)
{
    /* cont */
    lv_obj_t* obj = lv_obj_create(par);
    lv_obj_remove_style_all(obj);
    lv_obj_add_style(obj, &ui.styleCont, 0);
    lv_obj_set_size(obj, 159, 66);
    lv_obj_align(obj, LV_ALIGN_BOTTOM_LEFT, -10, 10);
    lv_obj_set_style_radius(obj, 10, 0);
    ui.sportInfo.cont = obj;

    /* speed */
    lv_obj_t* label = lv_label_create(obj);
    lv_label_set_text(label, "00");
    lv_obj_set_style_text_font(label, ResourcePool::GetFont("bahnschrift_32"), 0);
    lv_obj_set_style_text_color(label, lv_color_white(), 0);
    lv_obj_align(label, LV_ALIGN_LEFT_MID, 20, -10);
    ui.sportInfo.labelSpeed = label;

    label = lv_label_create(obj);
    lv_label_set_text(label, "km/h");
    lv_obj_set_style_text_font(label, ResourcePool::GetFont("bahnschrift_13"), 0);
    lv_obj_set_style_text_color(label, lv_color_white(), 0);
    lv_obj_align_to(label, ui.sportInfo.labelSpeed, LV_ALIGN_OUT_BOTTOM_MID, 0, 3);

    ui.sportInfo.labelTrip = ImgLabel_Create(obj, ResourcePool::GetImage("trip"), 5, 10);
    ui.sportInfo.labelTime = ImgLabel_Create(obj, ResourcePool::GetImage("alarm"), 5, 30);
}

lv_obj_t* LiveMapView::ImgLabel_Create(lv_obj_t* par, const void* img_src, lv_coord_t x_ofs, lv_coord_t y_ofs)
{
    lv_obj_t* img = lv_img_create(par);
    lv_img_set_src(img, img_src);

    lv_obj_align(img, LV_ALIGN_TOP_MID, 0, y_ofs);

    lv_obj_t* label = lv_label_create(par);
    lv_label_set_text(label, "--");
    lv_obj_add_style(label, &ui.styleLabel, 0);
    lv_obj_align_to(label, img, LV_ALIGN_OUT_RIGHT_MID, x_ofs, 0);
    return label;
}

void LiveMapView::Track_Create(lv_obj_t* par)
{
    lv_obj_t* cont = lv_obj_create(par);
    lv_obj_remove_style_all(cont);
    lv_obj_set_size(cont, LV_PCT(100), LV_PCT(100));
    ui.track.cont = cont;

    ui.track.lineTrack = new lv_poly_line(cont);

    ui.track.lineTrack->set_style(&ui.styleLine);

    lv_obj_t* line = lv_line_create(cont);
    lv_obj_remove_style_all(line);
    lv_obj_add_style(line, &ui.styleLine, 0);
#if CONFIG_LIVE_MAP_DEBUG_ENABLE
    lv_obj_set_style_line_color(line, lv_palette_main(LV_PALETTE_BLUE), 0);
#endif
    ui.track.lineActive = line;
}

void LiveMapView::CreateFallbackTile(uint32_t index, const char* src)
{
    // Set to NULL image source  
    TILE_IMG_SET_SRC(ui.map.imgTiles[index], NULL);
    
    // Create different colored tiles to show layout
    uint32_t colors[] = {0x4CAF50, 0x2196F3, 0xFF9800, 0x9C27B0, 0xFF5722, 0x607D8B};
    uint32_t color = colors[index % 6];
    
    // Set background color
    lv_obj_set_style_bg_color(ui.map.imgTiles[index], lv_color_hex(color), 0);
    lv_obj_set_style_bg_opa(ui.map.imgTiles[index], LV_OPA_COVER, 0);
    
    // Make sure it's visible
    lv_obj_clear_flag(ui.map.imgTiles[index], LV_OBJ_FLAG_HIDDEN);
    
    // Add a label with tile info
    lv_obj_t* label = lv_label_create(ui.map.imgTiles[index]);
    lv_obj_center(label);
    lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_12, 0);
    
    // Extract just the filename for display
    const char* filename = strrchr(src, '/');
    if (filename) filename++; else filename = src;
    lv_label_set_text_fmt(label, "T%u\n%s", index, filename);
}
