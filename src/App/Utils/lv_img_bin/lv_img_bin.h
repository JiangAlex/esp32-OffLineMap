#ifndef LV_IMG_BIN_H
#define LV_IMG_BIN_H

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Create a BIN image object
 * @param parent parent object
 * @return pointer to the new image object
 */
lv_obj_t * lv_img_bin_create(lv_obj_t * parent);

/**
 * Set the source of a BIN image object
 * @param obj image object
 * @param src path to the BIN file
 */
void lv_img_bin_set_src(lv_obj_t * obj, const void * src);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_IMG_BIN_H*/