#pragma once

#ifndef COMPONENTS_VISION_UI_VISION_UI_LIB_H
#define COMPONENTS_VISION_UI_VISION_UI_LIB_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void lumenLoadLayout();

typedef struct VisionUIMotionVec3 {
    float x;
    float y;
    float z;
    bool xAvailable;
    bool yAvailable;
    bool zAvailable;
    const char* unit;
} VisionUIMotionVec3;

typedef struct VisionUIMotionVelocity3 {
    float p;
    float r;
    float y;
    bool yawAvailable;
    const char* unit;
} VisionUIMotionVelocity3;

typedef struct VisionUIMotionAnglePry {
    float p;
    float r;
    float y;
    bool yawAvailable;
    const char* unit;
} VisionUIMotionAnglePry;

typedef struct VisionUIMotionStatus {
    const char* stateText;
    const char* rateHzText;
} VisionUIMotionStatus;

VisionUIMotionVec3 lumenMotionGetAccXyz();
VisionUIMotionVelocity3 lumenMotionGetVelPry();
VisionUIMotionAnglePry lumenMotionGetAnglePry();
VisionUIMotionStatus lumenMotionGetStatus();

typedef struct StatsPower {
    float systemPowerNorm;
    float systemPowerW;
    float usbEnergyWh;
    float voltage;
    float current;
} StatsPower;

typedef struct StatsStatus {
    bool usbEnabled;
    bool ocpActive;
    bool ovpActive;
    bool systemFault;
    const char* systemStateText;
} StatsStatus;

StatsPower lumenStatsGetPower();
StatsStatus lumenStatsGetStatus();

typedef struct LumenConfigCallbacks {
    void (*overcurrentOnChange)(int16_t value);
    void (*overvoltageOnChange)(int16_t value);
    void (*enableAutoFaultRecoveryOnChange)(bool value);
    void (*turnOffUsbOnChange)(bool value);
    void (*overvoltageAlertOnChange)(bool value);
    void (*overcurrentAlertOnChange)(bool value);
} LumenConfigCallbacks;

LumenConfigCallbacks lumenSetConfigCallbacks();

typedef struct LumenSystemInfo {
    const char* commit;
    const char* build;
    const char* version;
} LumenSystemInfo;

#ifndef VISION_UI_LIB_IS_LIB
typedef enum { VisionAllocMalloc, VisionAllocCalloc, VisionAllocFree } vision_alloc_op_t; // NOLINT

typedef struct vision_ui_list_icon { // NOLINT
    uint8_t* list_header; // NOLINT
    uint8_t* switch_header; // NOLINT
    uint8_t* slider_header; // NOLINT
    uint8_t* default_header; // NOLINT

    uint8_t* switch_on_footer; // NOLINT
    uint8_t* switch_off_footer; // NOLINT
    uint8_t* slider_footer; // NOLINT

    size_t header_width; // NOLINT
    size_t header_height; // NOLINT

    size_t footer_width; // NOLINT
    size_t footer_height; // NOLINT
} vision_ui_icon_t; // NOLINT

typedef struct vision_ui_font { // NOLINT
    const void* font; // NOLINT
    int8_t top_compensation; // NOLINT
    int8_t bottom_compensation; // NOLINT
} vision_ui_font_t; // NOLINT

typedef enum vision_ui_action_t { // NOLINT
    UiActionNone,
    UiActionGoPrev,
    UiActionGoNext,
    UiActionEnter,
    UiActionExit,
} vision_ui_action_t; // NOLINT
#else
#include "vision_ui_item.h"
#include "vision_ui_renderer.h"
#endif

typedef struct LumenSystemConfig {
    vision_ui_font_t title;
    vision_ui_font_t subtitle;
    vision_ui_font_t normal;
    vision_ui_font_t mini;

    vision_ui_icon_t icon;

    const uint8_t* logo;
    uint32_t logoSpan;

    const uint8_t* systemIcon;
    const uint8_t* motionIcon;
    const uint8_t* usbIcon;
    const uint8_t* statIcon;
    const uint8_t* creeperIcon;
} LumenSystemConfig;


LumenSystemInfo lumenGetSystemInfo();
LumenSystemConfig lumenGetSystemConfig();

typedef struct LumenUSBInfo {
    int16_t overCurrentMin;
    int16_t hardwareLimitedCurrent;
    int16_t overCurrentDefault;
    int16_t overVoltageMin;
    int16_t overVoltageMax;
    int16_t overVoltageDefault;
} LumenUSBInfo;


LumenUSBInfo lumenGetUSBInfo();

typedef struct LumenEasterEgg {
    const int16_t creeperWidth;
    const int16_t creeperHeight;
    const uint8_t* creeperLeft;
    const uint8_t* creeperRight;
    const uint8_t* creeperLeftBlowing;
    const uint8_t* creeperRightBlowing;

    const int16_t explosionWidth;
    const int16_t explosionHeight;
    const uint8_t* explosionEffects[15];

    const int16_t particleWidth;
    const int16_t particleHeight;
    const uint8_t* particleEffects[5];
} LumenEasterEgg;

typedef struct LumenEasterEggState {
    const int16_t dx;
    const int16_t dy;
    const bool ignite;
} LumenEasterEggState;

LumenEasterEgg lumenGetEasterEgg();
LumenEasterEggState lumenGetEasterEggState();

extern void vision_ui_step_render(); // NOLINT
extern void vision_ui_allocator_set(void* (*allocator)(vision_alloc_op_t op, size_t size, size_t count, void* ptr)
); // NOLINT

vision_ui_action_t vision_ui_driver_action_get(); // NOLINT
uint32_t vision_ui_driver_ticks_ms_get(); // NOLINT
void vision_ui_driver_delay(uint32_t ms); // NOLINT
void vision_ui_driver_bind(void* driver); // NOLINT
void vision_ui_driver_font_set(vision_ui_font_t font); // NOLINT
vision_ui_font_t vision_ui_driver_font_get(); // NOLINT
void vision_ui_driver_str_draw(uint16_t x, uint16_t y, const char* str); // NOLINT
void vision_ui_driver_str_utf8_draw(uint16_t x, uint16_t y, const char* str); // NOLINT
uint16_t vision_ui_driver_str_width_get(const char* str); // NOLINT
uint16_t vision_ui_driver_str_utf8_width_get(const char* str); // NOLINT
uint16_t vision_ui_driver_str_height_get(); // NOLINT
void vision_ui_driver_pixel_draw(uint16_t x, uint16_t y); // NOLINT
void vision_ui_driver_circle_draw(uint16_t x, uint16_t y, uint16_t r); // NOLINT
void vision_ui_driver_disc_draw(uint16_t x, uint16_t y, uint16_t r); // NOLINT
void vision_ui_driver_box_r_draw(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t r); // NOLINT
void vision_ui_driver_box_draw(uint16_t x, uint16_t y, uint16_t w, uint16_t h); // NOLINT
void vision_ui_driver_frame_draw(uint16_t x, uint16_t y, uint16_t w, uint16_t h); // NOLINT
void vision_ui_driver_frame_r_draw(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t r); // NOLINT
void vision_ui_driver_line_h_draw(uint16_t x, uint16_t y, uint16_t l); // NOLINT
void vision_ui_driver_line_v_draw(uint16_t x, uint16_t y, uint16_t h); // NOLINT
void vision_ui_driver_line_draw(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2); // NOLINT
void vision_ui_driver_line_h_dotted_draw(uint16_t x, uint16_t y, uint16_t l); // NOLINT
void vision_ui_driver_line_v_dotted_draw(uint16_t x, uint16_t y, uint16_t h); // NOLINT
void vision_ui_driver_bmp_draw(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint8_t* bit_map); // NOLINT
void vision_ui_driver_color_draw(uint8_t color); // NOLINT
void vision_ui_driver_font_mode_set(uint8_t mode); // NOLINT
void vision_ui_driver_font_direction_set(uint8_t dir); // NOLINT
void vision_ui_driver_clip_window_set(int16_t x0, int16_t y0, int16_t x1, int16_t y1); // NOLINT
void vision_ui_driver_clip_window_reset(); // NOLINT
void vision_ui_driver_buffer_clear(); // NOLINT
void vision_ui_driver_buffer_send(); // NOLINT
void vision_ui_driver_buffer_area_send(uint16_t x, uint16_t y, uint16_t w, uint16_t h); // NOLINT
/// @attention this should be a full size buffer
void* vision_ui_driver_buffer_pointer_get(); // NOLINT

#ifdef __cplusplus
}
#endif

#endif // COMPONENTS_VISION_UI_VISION_UI_LIB_H
