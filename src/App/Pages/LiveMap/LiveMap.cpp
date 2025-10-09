#include "LiveMap.h"
#include "App/Config/Config.h"
#include "App/Common/HAL/HAL.h"
#include <string.h>

using namespace Page;

uint16_t LiveMap::mapLevelCurrent = CONFIG_LIVE_MAP_LEVEL_DEFAULT;

LiveMap::LiveMap()
{
    memset(&priv, 0, sizeof(priv));
}

LiveMap::~LiveMap()
{
}

void LiveMap::onCustomAttrConfig()
{
    SetCustomCacheEnable(false);
}

void LiveMap::onViewLoad()
{
    const uint32_t tileSize = 256;

    Model.tileConv.SetTileSize(tileSize);
    Model.tileConv.SetViewSize(
        CONFIG_LIVE_MAP_VIEW_WIDTH,
        CONFIG_LIVE_MAP_VIEW_HEIGHT);
    Model.tileConv.SetFocusPos(0, 0);

    TileConv::Rect_t rect;
    uint32_t tileNum = Model.tileConv.GetTileContainer(&rect);

    View.Create(_root, tileNum);
    lv_slider_set_range(
        View.ui.zoom.slider,
        Model.mapConv.GetLevelMin(),
        Model.mapConv.GetLevelMax());
    View.SetMapTile(tileSize, rect.width / tileSize);

#if CONFIG_LIVE_MAP_DEBUG_ENABLE
    lv_obj_t *contView = lv_obj_create(root);
    lv_obj_center(contView);
    lv_obj_set_size(contView, CONFIG_LIVE_MAP_VIEW_WIDTH, CONFIG_LIVE_MAP_VIEW_HEIGHT);
    lv_obj_set_style_border_color(contView, lv_palette_main(LV_PALETTE_RED), 0);
    lv_obj_set_style_border_width(contView, 1, 0);
#endif

    AttachEvent(lv_scr_act());
    AttachEvent(_root);
    AttachEvent(View.ui.zoom.slider);
    AttachEvent(View.ui.sportInfo.cont);

    lv_slider_set_value(View.ui.zoom.slider, mapLevelCurrent, LV_ANIM_OFF);
    Model.mapConv.SetLevel(mapLevelCurrent);
    printf("[LiveMap::onViewLoad] Initial settings: mapLevelCurrent=%d, CONFIG_DEFAULT=%d\n", 
           mapLevelCurrent, CONFIG_LIVE_MAP_LEVEL_DEFAULT);
    printf("[LiveMap::onViewLoad] MapConv level after SetLevel: %d\n", Model.mapConv.GetLevel());
    lv_obj_add_flag(View.ui.map.cont, LV_OBJ_FLAG_HIDDEN);

    /* Point filter */
    Model.pointFilter.SetOffsetThreshold(CONFIG_TRACK_FILTER_OFFSET_THRESHOLD);
    Model.pointFilter.SetOutputPointCallback([](TrackPointFilter *filter, const TrackPointFilter::Point_t *point)
                                             {
        LiveMap* instance = (LiveMap*)filter->userData;
        instance->TrackLineAppendToEnd((int32_t)point->x, (int32_t)point->y); });
    Model.pointFilter.userData = this;

    /* Line filter */
    Model.lineFilter.SetOutputPointCallback(onTrackLineEvent);
    Model.lineFilter.userData = this;

}

void LiveMap::onViewDidLoad()
{
}

void LiveMap::onViewWillAppear()
{
    lv_obj_set_style_opa(_root, LV_OPA_COVER, LV_PART_MAIN);
    Model.Init();

    char theme[16];
    Model.GetArrowTheme(theme, sizeof(theme));
    View.SetArrowTheme(theme);

    priv.isTrackAvtive = Model.GetTrackFilterActive();

    Model.SetStatusBarStyle(DataProc::STATUS_BAR_STYLE_BLACK);
    SportInfoUpdate();
    lv_obj_clear_flag(View.ui.labelInfo, LV_OBJ_FLAG_HIDDEN);
}

void LiveMap::onViewDidAppear()
{
    priv.timer = lv_timer_create([](lv_timer_t *timer)
                                 {
        LiveMap* instance = (LiveMap*)timer->user_data;
        instance->Update(); },
                                 100,
                                 this);
    priv.lastMapUpdateTime = 0;
    
    printf("[LiveMap] Making map container visible...\n");
    lv_obj_clear_flag(View.ui.map.cont, LV_OBJ_FLAG_HIDDEN);
    
    printf("[LiveMap] Hiding loading label...\n");
    lv_obj_add_flag(View.ui.labelInfo, LV_OBJ_FLAG_HIDDEN);
    
    // Debug map container position and size
    lv_coord_t map_x = lv_obj_get_x(View.ui.map.cont);
    lv_coord_t map_y = lv_obj_get_y(View.ui.map.cont);
    lv_coord_t map_w = lv_obj_get_width(View.ui.map.cont);
    lv_coord_t map_h = lv_obj_get_height(View.ui.map.cont);
    printf("[LiveMap] Map container: pos(%d,%d) size(%d,%d)\n", map_x, map_y, map_w, map_h);
    
    priv.lastTileContOriPoint.x = 0;
    priv.lastTileContOriPoint.y = 0;

    priv.isTrackAvtive = Model.GetTrackFilterActive();
    if (!priv.isTrackAvtive)
    {
        Model.pointFilter.SetOutputPointCallback(nullptr);
    }

    lv_group_t *group = lv_group_get_default();
    lv_group_add_obj(group, View.ui.zoom.slider);
    lv_group_set_editing(group, View.ui.zoom.slider);
}

void LiveMap::onViewWillDisappear()
{
    lv_timer_del(priv.timer);
    lv_obj_add_flag(View.ui.map.cont, LV_OBJ_FLAG_HIDDEN);
    lv_obj_fade_out(_root, 250, 250);
}

void LiveMap::onViewDidDisappear()
{
    Model.Deinit();
}

void LiveMap::onViewUnload()
{
    View.Delete();

    lv_obj_remove_event_cb(lv_scr_act(), onEvent); // 移除屏幕手势回调函数
}

void LiveMap::onViewDidUnload()
{
}

void LiveMap::AttachEvent(lv_obj_t *obj)
{
    lv_obj_add_event_cb(obj, onEvent, LV_EVENT_ALL, this);
}

void LiveMap::Update()
{
    if (lv_tick_elaps(priv.lastMapUpdateTime) >= CONFIG_GPS_REFR_PERIOD)
    {
        CheckPosition();
        SportInfoUpdate();
        priv.lastMapUpdateTime = lv_tick_get();
    }
    else if (lv_tick_elaps(priv.lastContShowTime) >= 3000)
    {
        lv_obj_add_state(View.ui.zoom.cont, LV_STATE_USER_1);
    }
}

void LiveMap::UpdateDelay(uint32_t ms)
{
    priv.lastMapUpdateTime = lv_tick_get() - 1000 + ms;
}

void LiveMap::SportInfoUpdate()
{
    lv_label_set_text_fmt(
        View.ui.sportInfo.labelSpeed,
        "%02d",
        (int)Model.sportStatusInfo.speedKph);

    lv_label_set_text_fmt(
        View.ui.sportInfo.labelTrip,
        "%0.1f km",
        Model.sportStatusInfo.singleDistance / 1000);

    char buf[16];
    lv_label_set_text(
        View.ui.sportInfo.labelTime,
        DataProc::MakeTimeString(Model.sportStatusInfo.singleTime, buf, sizeof(buf)));
}

void LiveMap::CheckPosition()
{
    bool refreshMap = false;

    HAL::GPS_Info_t gpsInfo;
    Model.GetGPS_Info(&gpsInfo);

    mapLevelCurrent = lv_slider_get_value(View.ui.zoom.slider);
    if (mapLevelCurrent != Model.mapConv.GetLevel())
    {
        refreshMap = true;
        Model.mapConv.SetLevel(mapLevelCurrent);
        printf("[Update] SetLevel: %d -> %d\n", Model.mapConv.GetLevel(), mapLevelCurrent);
    }

    int32_t mapX, mapY;
    Model.mapConv.ConvertMapCoordinate(
        gpsInfo.longitude, gpsInfo.latitude,
        &mapX, &mapY);
    Model.tileConv.SetFocusPos(mapX, mapY);

    if (GetIsMapTileContChanged())
    {
        refreshMap = true;
        printf("[Update] refreshMap\n");
    }

    if (refreshMap)
    {
        TileConv::Rect_t rect;
        Model.tileConv.GetTileContainer(&rect);

        Area_t area =
            {
                .x0 = rect.x,
                .y0 = rect.y,
                .x1 = rect.x + rect.width - 1,
                .y1 = rect.y + rect.height - 1};

        onMapTileContRefresh(&area, mapX, mapY);
    }

    MapTileContUpdate(mapX, mapY, gpsInfo.course);

    if (priv.isTrackAvtive)
    {
        Model.pointFilter.PushPoint(mapX, mapY);
    }
}

void LiveMap::onMapTileContRefresh(const Area_t *area, int32_t x, int32_t y)
{
    LV_LOG_INFO(
        "area: (%d, %d) [%dx%d]",
        area->x0, area->y0,
        area->x1 - area->x0 + 1,
        area->y1 - area->y0 + 1);

    MapTileContReload();

    if (priv.isTrackAvtive)
    {
        TrackLineReload(area, x, y);
    }
}

void LiveMap::MapTileContUpdate(int32_t mapX, int32_t mapY, float course)
{
    TileConv::Point_t offset;
    TileConv::Point_t curPoint = {mapX, mapY};
    Model.tileConv.GetOffset(&offset, &curPoint);

    /* arrow */
    lv_obj_t *img = View.ui.map.imgArrow;
    Model.tileConv.GetFocusOffset(&offset);
    lv_coord_t x = offset.x - lv_obj_get_width(img) / 2;
    lv_coord_t y = offset.y - lv_obj_get_height(img) / 2;
    View.SetImgArrowStatus(x, y, course);

    /* active line */
    if (priv.isTrackAvtive)
    {
        View.SetLineActivePoint((lv_coord_t)offset.x, (lv_coord_t)offset.y);
    }

    /* map cont */
    Model.tileConv.GetTileContainerOffset(&offset);

    lv_coord_t baseX = (LV_HOR_RES - CONFIG_LIVE_MAP_VIEW_WIDTH) / 2;
    lv_coord_t baseY = (LV_VER_RES - CONFIG_LIVE_MAP_VIEW_HEIGHT) / 2;
    lv_obj_set_pos(View.ui.map.cont, baseX - offset.x, baseY - offset.y);
}

void LiveMap::MapTileContReload()
{
    /* SD card and MAP directory check */
    printf("[MapTileContReload] Starting map tile reload...\n");
    
    // Check SD card status first
    bool sdReady = HAL::SD_GetReady();
    printf("[MapTileContReload] SD card status: %s\n", sdReady ? "Ready" : "Not Ready");
    
    if (!sdReady)
    {
        printf("[MapTileContReload] Attempting to initialize SD card...\n");
        printf("[MapTileContReload] SD card pins: MISO=13, MOSI=11, CLK=12, CS=21\n");
        
        bool initResult = HAL::SD_Init();
        printf("[MapTileContReload] SD init result: %s\n", initResult ? "Success" : "Failed");
        
        if (!initResult)
        {
            printf("[MapTileContReload] SD card initialization failed!\n");
            printf("[MapTileContReload] Possible causes:\n");
            printf("[MapTileContReload] 1. SD card not inserted\n");
            printf("[MapTileContReload] 2. SD card damaged or incompatible\n");
            printf("[MapTileContReload] 3. Hardware connection issues\n");
            printf("[MapTileContReload] 4. SD card not formatted properly (should be FAT32)\n");
            printf("[MapTileContReload] Cannot load map tiles without SD card.\n");
            return;
        }
    }
    
    // First check if SD card root is accessible
    lv_fs_dir_t rootDir;
    if (lv_fs_dir_open(&rootDir, "/") == LV_FS_RES_OK)
    {
        printf("[MapTileContReload] Root directory (/) opened successfully\n");
        
        // List root directory contents
        char name[128];
        printf("[MapTileContReload] Root directory contents:\n");
        while (1)
        {
            lv_fs_res_t res = lv_fs_dir_read(&rootDir, name);
            if (name[0] == '\0' || res != LV_FS_RES_OK)
            {
                break;
            }
            printf("[MapTileContReload]   - %s\n", name);
        }
        lv_fs_dir_close(&rootDir);
    }
    else
    {
        printf("[MapTileContReload] ERROR: Cannot open root directory! SD card may not be mounted.\n");
        printf("[MapTileContReload] Trying to check SD card size...\n");
        float cardSize = HAL::SD_GetCardSizeMB();
        printf("[MapTileContReload] SD card size: %.1f MB\n", cardSize);
        return;
    }
    
    // Check if MAP directory exists
    lv_fs_dir_t dir;
    if (lv_fs_dir_open(&dir, "/MAP") == LV_FS_RES_OK)
    {
        printf("[MapTileContReload] /MAP directory opened successfully\n");
        lv_fs_dir_close(&dir);
        
        // Check if current level directory exists
        char levelPath[32];
        int currentLevel = Model.mapConv.GetLevel();
        snprintf(levelPath, sizeof(levelPath), "/MAP/%d", currentLevel);
        if (lv_fs_dir_open(&dir, levelPath) == LV_FS_RES_OK)
        {
            printf("[MapTileContReload] %s directory exists\n", levelPath);
            lv_fs_dir_close(&dir);
        }
        else
        {
            printf("[MapTileContReload] ERROR: %s directory not found!\n", levelPath);
            printf("[MapTileContReload] Current map level: %d\n", currentLevel);
            
            // Try to fallback to default level if current level doesn't exist
            if (currentLevel != CONFIG_LIVE_MAP_LEVEL_DEFAULT)
            {
                printf("[MapTileContReload] Attempting fallback to default level %d\n", CONFIG_LIVE_MAP_LEVEL_DEFAULT);
                Model.mapConv.SetLevel(CONFIG_LIVE_MAP_LEVEL_DEFAULT);
                mapLevelCurrent = CONFIG_LIVE_MAP_LEVEL_DEFAULT;
                
                // Update slider to reflect the corrected level
                lv_slider_set_value(View.ui.zoom.slider, CONFIG_LIVE_MAP_LEVEL_DEFAULT, LV_ANIM_OFF);
                
                // Try again with default level
                snprintf(levelPath, sizeof(levelPath), "/MAP/%d", CONFIG_LIVE_MAP_LEVEL_DEFAULT);
                if (lv_fs_dir_open(&dir, levelPath) == LV_FS_RES_OK)
                {
                    printf("[MapTileContReload] Fallback successful: %s directory exists\n", levelPath);
                    lv_fs_dir_close(&dir);
                }
                else
                {
                    printf("[MapTileContReload] CRITICAL: Even default level %d directory not found!\n", CONFIG_LIVE_MAP_LEVEL_DEFAULT);
                    
                    // Last resort: try to find any available map level
                    printf("[MapTileContReload] Searching for any available map level...\n");
                    bool foundLevel = false;
                    for (int tryLevel = 6; tryLevel <= 18 && !foundLevel; tryLevel++)
                    {
                        snprintf(levelPath, sizeof(levelPath), "/MAP/%d", tryLevel);
                        if (lv_fs_dir_open(&dir, levelPath) == LV_FS_RES_OK)
                        {
                            lv_fs_dir_close(&dir);
                            printf("[MapTileContReload] Found available level %d, using it\n", tryLevel);
                            Model.mapConv.SetLevel(tryLevel);
                            mapLevelCurrent = tryLevel;
                            lv_slider_set_value(View.ui.zoom.slider, tryLevel, LV_ANIM_OFF);
                            foundLevel = true;
                        }
                    }
                    
                    if (!foundLevel)
                    {
                        printf("[MapTileContReload] FATAL: No map data found on SD card!\n");
                        return;
                    }
                }
            }
            else
            {
                printf("[MapTileContReload] CRITICAL: Default level directory not found!\n");
                return;
            }
        }
    }
    else
    {
        printf("[MapTileContReload] ERROR: Cannot open /MAP directory!\n");
        printf("[MapTileContReload] Checking if MAP directory exists...\n");
        
        // Try alternative paths
        const char* altPaths[] = {"MAP", "/SD/MAP", "/sd/MAP", "/SDCARD/MAP"};
        for (int i = 0; i < 4; i++)
        {
            if (lv_fs_dir_open(&dir, altPaths[i]) == LV_FS_RES_OK)
            {
                printf("[MapTileContReload] Found MAP at alternative path: %s\n", altPaths[i]);
                lv_fs_dir_close(&dir);
                break;
            }
        }
        return;
    }

    /* tile src */
    printf("[MapTileContReload] Loading %u map tiles at level %d\n", View.ui.map.tileNum, Model.mapConv.GetLevel());
    for (uint32_t i = 0; i < View.ui.map.tileNum; i++)
    {
        TileConv::Point_t pos;
        Model.tileConv.GetTilePos(i, &pos);

        char path[64];
        Model.mapConv.ConvertMapPath(pos.x, pos.y, path, sizeof(path));
        
        // Debug: Show the actual path generated by ConvertMapPath
        printf("[MapTileContReload] Generated path for tile %u: %s\n", i, path);
        #ifdef MAP_FORMAT_BIN
        printf("[MapTileContReload] Using BIN format (MAP_FORMAT_BIN defined)\n");
        #else
        printf("[MapTileContReload] Using PNG format (MAP_FORMAT_BIN not defined)\n");
        #endif
        
        // Calculate tile grid position for better debugging
        // Assuming a common grid layout (like 3x2 for 6 tiles)
        int tilesPerRow = (View.ui.map.tileNum <= 4) ? 2 : 3;  // Estimate based on tile count
        int gridRow = i / tilesPerRow;
        int gridCol = i % tilesPerRow;

        // Quick file existence check
        lv_fs_file_t testFile;
        bool tileExists = (lv_fs_open(&testFile, path, LV_FS_MODE_RD) == LV_FS_RES_OK);
        if (tileExists)
        {
            lv_fs_close(&testFile);
            printf("[MapTileContReload] ✓ Tile %u [%d,%d]\n", i, gridRow, gridCol);
            View.SetMapTileSrc(i, path);
        }
        else
        {
            printf("[MapTileContReload] ✗ Tile %u [%d,%d]: %s (not found)\n", i, gridRow, gridCol, path);
            // Still try to set the tile source - let the view handle missing files
            View.SetMapTileSrc(i, path);
        }
    }
}

bool LiveMap::GetIsMapTileContChanged()
{
    TileConv::Point_t pos;
    Model.tileConv.GetTilePos(0, &pos);

    bool ret = (pos.x != priv.lastTileContOriPoint.x || pos.y != priv.lastTileContOriPoint.y);

    priv.lastTileContOriPoint = pos;

    return ret;
}

void LiveMap::TrackLineReload(const Area_t *area, int32_t x, int32_t y)
{
    Model.lineFilter.SetClipArea(area);
    Model.lineFilter.Reset();
    Model.TrackReload([](TrackPointFilter *filter, const TrackPointFilter::Point_t *point)
                      {
        LiveMap* instance = (LiveMap*)filter->userData;
        instance->Model.lineFilter.PushPoint((int32_t)point->x, (int32_t)point->y); }, this);
    Model.lineFilter.PushPoint(x, y);
    Model.lineFilter.PushEnd();
}

void LiveMap::TrackLineAppend(int32_t x, int32_t y)
{
    TileConv::Point_t offset;
    TileConv::Point_t curPoint = {x, y};
    Model.tileConv.GetOffset(&offset, &curPoint);
    View.ui.track.lineTrack->append((lv_coord_t)offset.x, (lv_coord_t)offset.y);
}

void LiveMap::TrackLineAppendToEnd(int32_t x, int32_t y)
{
    TileConv::Point_t offset;
    TileConv::Point_t curPoint = {x, y};
    Model.tileConv.GetOffset(&offset, &curPoint);
    View.ui.track.lineTrack->append_to_end((lv_coord_t)offset.x, (lv_coord_t)offset.y);
}

void LiveMap::onTrackLineEvent(TrackLineFilter *filter, TrackLineFilter::Event_t *event)
{
    LiveMap *instance = (LiveMap *)filter->userData;
    lv_poly_line *lineTrack = instance->View.ui.track.lineTrack;

    switch (event->code)
    {
    case TrackLineFilter::EVENT_START_LINE:
        lineTrack->start();
        instance->TrackLineAppend(event->point->x, event->point->y);
        break;
    case TrackLineFilter::EVENT_APPEND_POINT:
        instance->TrackLineAppend(event->point->x, event->point->y);
        break;
    case TrackLineFilter::EVENT_END_LINE:
        if (event->point != nullptr)
        {
            instance->TrackLineAppend(event->point->x, event->point->y);
        }
        lineTrack->stop();
        break;
    case TrackLineFilter::EVENT_RESET:
        lineTrack->reset();
        break;
    default:
        break;
    }
}

void LiveMap::onEvent(lv_event_t *event)
{
    LiveMap *instance = (LiveMap *)lv_event_get_user_data(event);
    LV_ASSERT_NULL(instance);

    lv_obj_t *obj = lv_event_get_current_target(event);
    lv_event_code_t code = lv_event_get_code(event);

    if (code == LV_EVENT_LEAVE)
    {
        instance->_Manager->Pop();
        return;
    }

    if (obj == instance->View.ui.zoom.slider)
    {
        if (code == LV_EVENT_VALUE_CHANGED)
        {
            // int32_t level = lv_slider_get_value(obj);
            // int32_t levelMax = instance->Model.mapConv.GetLevelMax();
            // lv_label_set_text_fmt(instance->View.ui.zoom.labelInfo, "%d/%d", level, levelMax);

            // lv_obj_clear_state(instance->View.ui.zoom.cont, LV_STATE_USER_1);
            // instance->priv.lastContShowTime = lv_tick_get();
            // instance->UpdateDelay(200);

            printf("[Update] Slider Val Change\n");
        }
        else if (code == LV_EVENT_PRESSED)
        {
            instance->_Manager->Pop();
        }
    }

    if (code == LV_EVENT_GESTURE)
    {
        lv_indev_wait_release(lv_indev_get_act());

        // int32_t level = lv_slider_get_value(instance->View.ui.zoom.slider);
        // int32_t levelMax = instance->Model.mapConv.GetLevelMax();

        switch (lv_indev_get_gesture_dir(lv_indev_get_act()))
        {
        case LV_DIR_LEFT:
            lv_slider_set_value(instance->View.ui.zoom.slider, lv_slider_get_value(instance->View.ui.zoom.slider) - 1, LV_ANIM_OFF);

            lv_label_set_text_fmt(instance->View.ui.zoom.labelInfo, "%d/%d", lv_slider_get_value(instance->View.ui.zoom.slider), instance->Model.mapConv.GetLevelMax());

            lv_obj_clear_state(instance->View.ui.zoom.cont, LV_STATE_USER_1);
            instance->priv.lastContShowTime = lv_tick_get();
            instance->UpdateDelay(200);

            printf("[Update] Slider Val Change\n");

            printf("[TOUCH] LEFT:%d\n", lv_slider_get_value(instance->View.ui.zoom.slider));
            break;
        case LV_DIR_RIGHT:
            lv_slider_set_value(instance->View.ui.zoom.slider, lv_slider_get_value(instance->View.ui.zoom.slider) + 1, LV_ANIM_OFF);

            lv_label_set_text_fmt(instance->View.ui.zoom.labelInfo, "%d/%d", lv_slider_get_value(instance->View.ui.zoom.slider), instance->Model.mapConv.GetLevelMax());

            lv_obj_clear_state(instance->View.ui.zoom.cont, LV_STATE_USER_1);
            instance->priv.lastContShowTime = lv_tick_get();
            instance->UpdateDelay(200);

            printf("[Update] Slider Val Change\n");

            printf("[TOUCH] RIGHT:%d\n", lv_slider_get_value(instance->View.ui.zoom.slider));

            break;
        case LV_DIR_TOP:

            break;
        case LV_DIR_BOTTOM:

            break;
        default:
            break;
        }
    }

    if (obj == instance->View.ui.sportInfo.cont)
    {
        if (code == LV_EVENT_PRESSED)
        {
            instance->_Manager->Pop();
        }
    }
}
