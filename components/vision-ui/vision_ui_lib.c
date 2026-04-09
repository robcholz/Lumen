#include "vision_ui_lib.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include <vision_ui_config.h>
#include <vision_ui_draw_driver.h>
#include <vision_ui_item.h>
#include <vision_ui_renderer.h>

static int maxInt(const int a, const int b) {
    return a > b ? a : b;
}

static float maxF(const float a, const float b) {
    return a > b ? a : b;
}

static float minF(const float a, const float b) {
    return a < b ? a : b;
}

static float clampF(const float v, const float lo, const float hi) {
    if (v < lo) {
        return lo;
    }
    if (v > hi) {
        return hi;
    }
    return v;
}

typedef struct MotionDialDot {
    float dotNx;
    float dotNy;
    bool dotVisible;
} MotionDialDot;

static MotionDialDot motionGetDialDot() {
    const VisionUIMotionVec3 acc = lumenMotionGetAccXyz(); // m/s^2 (body x,y,z)
    const VisionUIMotionVelocity3 omg = lumenMotionGetVelPry(); // rad/s (p,r,y)
    const VisionUIMotionAnglePry ang = lumenMotionGetAnglePry(); // rad   (p,r,y)

    // 45deg = pi/4 rad
    const float mPI = 3.14159265358979323846264338327950288;
    const float attRangeRad = mPI / 4.0f;

    // attitude -> normalized
    const float nxAtt = ang.r / attRangeRad; // roll drives x
    const float nyAtt = ang.p / attRangeRad; // pitch drives y

    // angular rate -> tiny dynamic feel
    const float gyroGain = 0.015f;
    const float nxGyro = omg.r * gyroGain;
    const float nyGyro = omg.p * gyroGain;

    // linear accel -> lpf "shake"
    static float accLpfX = 0.0f;
    static float accLpfY = 0.0f;
    const float accAlpha = 0.06f;
    const float accGain = 0.04f;

    accLpfX = accLpfX * (1.0f - accAlpha) + acc.x * accAlpha;
    accLpfY = accLpfY * (1.0f - accAlpha) + acc.y * accAlpha;

    const float nxAcc = accLpfX * accGain;
    const float nyAcc = accLpfY * accGain;

    const float nx = nxAtt + nxGyro + nxAcc;
    const float ny = nyAtt + nyGyro + nyAcc;

    return (MotionDialDot){.dotNx = clampF(-nx, -1.0f, 1.0f), .dotNy = clampF(-ny, -1.0f, 1.0f), .dotVisible = true};
}

static void motionDrawFilledCircle(const uint16_t x, const uint16_t y, const uint16_t r) {
    vision_ui_driver_disc_draw(x, y, r);
}

static void motionFormatComponent(
        char* buf,
        const size_t bufSize,
        const float value,
        const bool available,
        const char* fmt
) {
    if (available) {
        snprintf(buf, bufSize, fmt, value);
    } else {
        snprintf(buf, bufSize, "%6s", "--");
    }
}

void motionInitCallback() {
    vision_ui_driver_font_set(vision_ui_minifont_get());
}

void motionLoop() {
    const uint16_t topPadding = 3;
    const uint16_t rowGap = 3;
    const uint16_t leftPadding = 6;

    const VisionUIMotionVec3 acc = lumenMotionGetAccXyz();
    const VisionUIMotionVelocity3 vel = lumenMotionGetVelPry();
    const VisionUIMotionAnglePry ang = lumenMotionGetAnglePry();
    const VisionUIMotionStatus status = lumenMotionGetStatus();
    const MotionDialDot dot = motionGetDialDot();

    char accX[8];
    char accY[8];
    char accZ[8];
    motionFormatComponent(accX, sizeof(accX), acc.x, acc.xAvailable, "%+6.2f");
    motionFormatComponent(accY, sizeof(accY), acc.y, acc.yAvailable, "%+6.2f");
    motionFormatComponent(accZ, sizeof(accZ), acc.z, acc.zAvailable, "%+6.2f");

    char velP[8];
    char velR[8];
    char velY[8];
    motionFormatComponent(velP, sizeof(velP), vel.p, true, "%+6.1f");
    motionFormatComponent(velR, sizeof(velR), vel.r, true, "%+6.2f");
    if (vel.yawAvailable) {
        motionFormatComponent(velY, sizeof(velY), ang.y, true, "%+6.1f");
    } else {
        snprintf(velY, sizeof(velY), "%6s", "--");
    }

    char angP[8];
    char angR[8];
    char angY[8];
    motionFormatComponent(angP, sizeof(angP), ang.p, true, "%+6.1f");
    motionFormatComponent(angR, sizeof(angR), ang.r, true, "%+6.1f");
    if (ang.yawAvailable) {
        motionFormatComponent(angY, sizeof(angY), ang.y, true, "%+6.1f");
    } else {
        snprintf(angY, sizeof(angY), "%6s", "--");
    }

    static char accLine[96];
    snprintf(accLine, sizeof(accLine), "Acc  x:%s y:%s z:%s  %s", accX, accY, accZ, acc.unit);
    static char velLine[96];
    snprintf(velLine, sizeof(velLine), "Vel  x:%s y:%s z:%s  %s", velP, velR, velY, vel.unit);
    static char angLine[96];
    snprintf(angLine, sizeof(angLine), "Angle  p:%s r:%s y:%s  %s", angP, angR, angY, ang.unit);
    static char motionLine[64];
    snprintf(motionLine, sizeof(motionLine), "Motion  %-7s %s", status.stateText, status.rateHzText);

    const uint16_t lineHeight = vision_ui_driver_str_height_get();
    uint16_t y = topPadding + lineHeight;

    vision_ui_driver_color_draw(1);
    vision_ui_driver_str_draw(leftPadding, y, accLine);
    y += lineHeight + rowGap;
    vision_ui_driver_str_draw(leftPadding, y, velLine);
    y += lineHeight + rowGap;
    vision_ui_driver_str_draw(leftPadding, y, angLine);
    y += lineHeight + rowGap;
    vision_ui_driver_str_draw(leftPadding, y, motionLine);

    const uint16_t textBlockHeight = topPadding + 4 * lineHeight + 3 * rowGap;
    const float remainingH = (float) (VISION_UI_SCREEN_HEIGHT - textBlockHeight);

    const float cxF = (float) (VISION_UI_SCREEN_WIDTH) * 0.5f;
    const float cyF = (float) (textBlockHeight) + remainingH * 0.5f;

    float radius = minF((float) (VISION_UI_SCREEN_WIDTH) * 0.35f, remainingH * 0.40f);
    radius = maxF(40.0f, minF(radius, 120.0f));

    const uint16_t cx = (uint16_t) (lrintf(cxF));
    const uint16_t cy = (uint16_t) (lrintf(cyF));
    const uint16_t r = (uint16_t) (lrintf(radius));

    // crosshair only; outer ring removed
    vision_ui_driver_line_h_draw(cx - r, cy, 2 * r);
    vision_ui_driver_line_v_draw(cx, cy - r, 2 * r);
    // center anchor disk
    const float centerR = clampF(radius * 0.06f, 2.0f, 5.0f);
    motionDrawFilledCircle(cx, cy, (uint16_t) (lrintf(centerR)));

    const float offset = radius * 0.85f;
    const float dotXF = cxF + dot.dotNx * offset;
    const float dotYF = cyF - dot.dotNy * offset;
    const uint16_t dotX = (uint16_t) (lrintf(dotXF));
    const uint16_t dotY = (uint16_t) (lrintf(dotYF));

    float dotRadius = maxF(3.0f, radius * 0.08f);
    dotRadius = minF(dotRadius, 8.0f);
    const uint16_t dotR = (uint16_t) (lrintf(dotRadius));
    vision_ui_driver_color_draw(1); // invert to stay visible over center disk
    motionDrawFilledCircle(dotX, dotY, dotR);
}

void motionExit() {
}

static void statsDrawPowerRing(const float norm, const uint16_t cx, const uint16_t cy, const uint16_t r) {
    const float clamped = clampF(norm, 0.0f, 1.0f);
    if (clamped <= 0.0f) {
        return;
    }

    const float mPI = 3.14159265358979323846264338327950288;

    const float sweep = mPI * 2 * clamped;
    const int segments = maxInt(8, (int) (ceilf(clamped * 60.0f)));

    static const float statsStartRad = -1.57079632679f;

    const float prevAngle = statsStartRad;
    uint16_t prevX = (uint16_t) (lrintf(cx + r * cosf(prevAngle)));
    uint16_t prevY = (uint16_t) (lrintf(cy + r * sinf(prevAngle)));

    for (int i = 1; i <= segments; ++i) {
        const float a = statsStartRad + sweep * ((float) (i) / (float) (segments));
        const uint16_t x = (uint16_t) (lrintf(cx + r * cosf(a)));
        const uint16_t y = (uint16_t) (lrintf(cy + r * sinf(a)));
        vision_ui_driver_line_draw(prevX, prevY, x, y + 4);
        prevX = x;
        prevY = y;
    }
}

void statsInit() {
    vision_ui_driver_font_set(vision_ui_minifont_get());
}

static uint16_t drawStatusCellC(
        const uint16_t cx,
        const uint16_t cy,
        const bool active,
        const char* label,
        const uint16_t labelH
) {
    const uint16_t rectSize = 10;
    const uint16_t gap = 6;

    const uint16_t labelW = vision_ui_driver_str_width_get(label);
    const uint16_t totalW = (uint16_t) (labelW + gap + rectSize);
    const uint16_t startXCell = (uint16_t) (cx - totalW / 2);

    const uint16_t textX = startXCell;
    const uint16_t textY = (uint16_t) (cy + labelH / 2);
    vision_ui_driver_str_draw(textX, textY, label);

    const uint16_t rectX = (uint16_t) (startXCell + labelW + gap);
    const uint16_t rectY = (uint16_t) (cy - rectSize / 2);

    if (active) {
        vision_ui_driver_box_draw(rectX, rectY, rectSize, rectSize);
    } else {
        vision_ui_driver_frame_draw(rectX, rectY, rectSize, rectSize);
    }

    return (uint16_t) (rectY + rectSize);
}

void statsLoop(void) {
    vision_ui_driver_color_draw(1);

    const StatsPower power = lumenStatsGetPower();
    const StatsStatus status = lumenStatsGetStatus();

    const uint16_t centerX = (VISION_UI_SCREEN_WIDTH / 2);

    const uint16_t ringRadius = 40;
    const uint16_t ringCenterY = (24 + ringRadius);
    statsDrawPowerRing(power.systemPowerNorm, centerX, ringCenterY, ringRadius);

    const uint16_t energyBetweenGap = 10;

    vision_ui_driver_font_set(vision_ui_font_get());
    char powerStr[24];
    snprintf(powerStr, sizeof(powerStr), "%.2f W", (double) power.systemPowerW);
    const uint16_t powerW = vision_ui_driver_str_width_get(powerStr);
    const uint16_t powerH = vision_ui_driver_str_height_get();
    const uint16_t powerY = (uint16_t) (ringCenterY + ringRadius + 8 + powerH);
    vision_ui_driver_str_draw((uint16_t) (centerX - powerW / 2), powerY, powerStr);

    vision_ui_driver_font_set(vision_ui_minifont_get());
    char energyStr[24];
    snprintf(energyStr, sizeof(energyStr), "%.2f Wh", (double) power.usbEnergyWh);
    const uint16_t energyW = vision_ui_driver_str_width_get(energyStr);
    const uint16_t energyH = vision_ui_driver_str_height_get();
    const uint16_t energyY = (uint16_t) (powerY + energyBetweenGap + energyH);
    vision_ui_driver_str_draw((uint16_t) (centerX - energyW / 2), energyY, energyStr);

    const uint16_t powerDetailH = (uint16_t) (energyY + 18);
    const uint16_t powerDetailWPadding = 5;

    char voltageStr[24];
    snprintf(voltageStr, sizeof(voltageStr), "%.2f V", (double) power.voltage);
    const uint16_t voltageW = vision_ui_driver_str_width_get(voltageStr);
    vision_ui_driver_str_draw((uint16_t) (centerX - voltageW - powerDetailWPadding), powerDetailH, voltageStr);

    char currentStr[24];
    snprintf(currentStr, sizeof(currentStr), "%.2f A", (double) power.current);
    vision_ui_driver_str_draw((centerX + powerDetailWPadding), powerDetailH, currentStr);

    const uint16_t gridTop = (uint16_t) (energyY + 15);
    const uint16_t cellWidth = 34;
    const uint16_t cellHeight = 12;
    const uint16_t colGap = 16;
    const uint16_t rowGap = 10;

    const uint16_t gridWidth = (uint16_t) (cellWidth * 2 + colGap);
    const uint16_t startX = (uint16_t) (centerX - gridWidth / 2);

    // Preserve the original layout math here even though the width/height halves look swapped.
    // If needed, this can be corrected later to use cellHeight/2 for rows and cellWidth/2 for columns.
    const uint16_t row1Y = (uint16_t) (gridTop + cellWidth / 2);
    const uint16_t row2Y = (uint16_t) (row1Y + cellHeight + rowGap);
    const uint16_t col1X = (startX + cellHeight / 2);
    const uint16_t col2X = (startX + cellWidth + colGap + cellWidth / 2);

    vision_ui_driver_font_set(vision_ui_minifont_get());
    const uint16_t labelH = vision_ui_driver_str_height_get();

    drawStatusCellC(col1X, row1Y, status.usbEnabled, "USB", labelH);
    drawStatusCellC(col2X, row1Y, status.ocpActive, "OCP", labelH);
    drawStatusCellC(col1X, row2Y, status.ovpActive, "OVP", labelH);
    drawStatusCellC(col2X, row2Y, status.systemFault, "FALT", labelH);
}

void statsExit() {
}

typedef enum {
    CreeperStateEnter,
    CreeperStateIdle,
    CreeperStateWalk,
} CreeperState;

static CreeperState CREEPER_STATE = CreeperStateEnter;
static float CREEPER_X = 0.0f;
static float CREEPER_Y = 0.0f;
static int8_t CREEPER_DIR_X = -1;
static int8_t CREEPER_FACE_X = -1;
static uint32_t CREEPER_LAST_TICK_MS = 0;
static uint32_t CREEPER_STATE_UNTIL_MS = 0;
static uint32_t CREEPER_DIR_CHANGE_MS = 0;
static uint32_t CREEPER_RAND_STATE = 1;
static float CREEPER_ENTRY_TARGET_X = 0.0f;
static const uint32_t CREEPER_IDLE_MIN_MS = 3000;
static const uint32_t CREEPER_TURN_MIN_MS = 900u;
static const uint32_t CREEPER_TURN_MAX_MS = 1800u;
static const uint32_t CREEPER_TURN_KEEP_PCT = 60u;
static const uint32_t CREEPER_MANUAL_HOLD_MS = 5000u;
static const uint32_t CREEPER_IGNITE_MS = 2500u;
static const uint32_t CREEPER_FLASH_PERIOD_MS = 200u;
static const uint32_t CREEPER_EXPLOSION_FRAME_MS = 10u;
static const uint32_t CREEPER_RESPAWN_PERIOD_MS = 1000u;
#define CREEPER_PARTICLE_COUNT 20
static const int16_t CREEPER_PARTICLE_RADIUS_MIN = 8;
static const int16_t CREEPER_PARTICLE_RADIUS_MAX = 22;
static uint32_t CREEPER_MANUAL_UNTIL_MS = 0u;
static bool CREEPER_WAS_MANUAL = false;
static bool CREEPER_IGNITE_ACTIVE = false;
static uint32_t CREEPER_IGNITE_START_MS = 0u;
static bool CREEPER_EXPLOSION_ACTIVE = false;
static uint32_t CREEPER_EXPLOSION_START_MS = 0u;
static bool CREEPER_EXPLODED = false;
static uint32_t CREEPER_RESPAWN_AT_MS = 0u;
static int16_t CREEPER_PARTICLE_OFFSETS_X[CREEPER_PARTICLE_COUNT];
static int16_t CREEPER_PARTICLE_OFFSETS_Y[CREEPER_PARTICLE_COUNT];

static uint32_t creeperRandU32() {
    return rand();
}

static uint32_t creeperRandRangeU32(const uint32_t minV, const uint32_t maxV) {
    if (maxV <= minV) {
        return minV;
    }
    return minV + (creeperRandU32() % (maxV - minV + 1u));
}

static void creeperPickDirectionWithBounds(const float maxX, const float margin) {
    int8_t dx = 0;
    if (CREEPER_X <= margin) {
        dx = 1;
    } else if (CREEPER_X >= maxX - margin) {
        dx = -1;
    } else {
        const uint32_t keepRoll = creeperRandU32() % 100u;
        if (keepRoll < CREEPER_TURN_KEEP_PCT && CREEPER_DIR_X != 0) {
            dx = CREEPER_DIR_X;
        } else {
            dx = (creeperRandU32() % 2u) ? 1 : -1;
        }
    }
    CREEPER_DIR_X = dx;
    CREEPER_FACE_X = dx;
}

static int16_t creeperClampParticleAxis(const int16_t value, const int16_t maxValue) {
    if (value < 0) {
        return 0;
    }
    if (value > maxValue) {
        return maxValue;
    }
    return value;
}

static void creeperInitParticles() {
    for (uint32_t i = 0; i < CREEPER_PARTICLE_COUNT; ++i) {
        const int16_t radius = (int16_t
        ) creeperRandRangeU32((uint32_t) CREEPER_PARTICLE_RADIUS_MIN, (uint32_t) CREEPER_PARTICLE_RADIUS_MAX);
        int16_t dx = 0;
        int16_t dy = 0;
        while (dx == 0 && dy == 0) {
            dx = (int16_t) ((int32_t) (creeperRandU32() % 3u) - 1);
            dy = (int16_t) ((int32_t) (creeperRandU32() % 3u) - 1);
        }
        CREEPER_PARTICLE_OFFSETS_X[i] = (int16_t) (dx * radius);
        CREEPER_PARTICLE_OFFSETS_Y[i] = (int16_t) (dy * radius);
    }
}

void creeperInit() {
    srand(vision_ui_driver_ticks_ms_get());
    const uint32_t nowMs = vision_ui_driver_ticks_ms_get();
    const LumenEasterEgg easter = lumenGetEasterEgg();

    CREEPER_RAND_STATE = nowMs ? nowMs : 1u;
    CREEPER_STATE = CreeperStateEnter;
    CREEPER_X = (float) VISION_UI_SCREEN_WIDTH;
    CREEPER_Y = (float) VISION_UI_SCREEN_HEIGHT - easter.creeperHeight;
    CREEPER_ENTRY_TARGET_X =
            (float) creeperRandRangeU32(0u, (uint32_t) maxInt(0, VISION_UI_SCREEN_WIDTH - easter.creeperWidth));
    CREEPER_DIR_X = -1;
    CREEPER_FACE_X = -1;
    CREEPER_LAST_TICK_MS = nowMs;
    CREEPER_STATE_UNTIL_MS = nowMs;
    CREEPER_DIR_CHANGE_MS = nowMs;
    CREEPER_MANUAL_UNTIL_MS = 0u;
    CREEPER_WAS_MANUAL = false;
    CREEPER_IGNITE_ACTIVE = false;
    CREEPER_IGNITE_START_MS = 0u;
    CREEPER_EXPLOSION_ACTIVE = false;
    CREEPER_EXPLOSION_START_MS = 0u;
    CREEPER_EXPLODED = false;
    vision_ui_driver_color_draw(0);
}

void creeperLoop() {
    const uint32_t nowMs = vision_ui_driver_ticks_ms_get();
    const uint32_t dtMs = nowMs - CREEPER_LAST_TICK_MS;
    CREEPER_LAST_TICK_MS = nowMs;

    const LumenEasterEgg easter = lumenGetEasterEgg();
    const LumenEasterEggState state = lumenGetEasterEggState();
    const float dtS = (float) dtMs / 1000.0f;

    const float maxX = (float) maxInt(0, VISION_UI_SCREEN_WIDTH - easter.creeperWidth);
    const float maxY = (float) maxInt(0, VISION_UI_SCREEN_HEIGHT - easter.creeperHeight);
    const bool manualInput = (state.dx != 0) || (state.dy != 0);

    const bool igniteWasActive = CREEPER_IGNITE_ACTIVE;
    if (state.ignite && !CREEPER_EXPLOSION_ACTIVE && !CREEPER_EXPLODED) {
        if (!CREEPER_IGNITE_ACTIVE) {
            CREEPER_IGNITE_ACTIVE = true;
            CREEPER_IGNITE_START_MS = nowMs;
        }
        if ((nowMs - CREEPER_IGNITE_START_MS) >= CREEPER_IGNITE_MS) {
            CREEPER_IGNITE_ACTIVE = false;
            CREEPER_EXPLOSION_ACTIVE = true;
            CREEPER_EXPLOSION_START_MS = nowMs;
            creeperInitParticles();
        }
    } else if (!state.ignite) {
        CREEPER_IGNITE_ACTIVE = false;
        CREEPER_IGNITE_START_MS = 0u;
    }
    if (igniteWasActive && !state.ignite && !CREEPER_EXPLOSION_ACTIVE && !CREEPER_EXPLODED) {
        CREEPER_STATE = CreeperStateIdle;
        CREEPER_STATE_UNTIL_MS = nowMs + creeperRandRangeU32(CREEPER_IDLE_MIN_MS, 1400u);
        CREEPER_DIR_CHANGE_MS = nowMs + creeperRandRangeU32(CREEPER_TURN_MIN_MS, CREEPER_TURN_MAX_MS);
        creeperPickDirectionWithBounds(maxX, 4.0f);
    }
    if (CREEPER_EXPLODED) {
        if (!manualInput && !state.ignite) {
            if (CREEPER_RESPAWN_AT_MS == 0u) {
                CREEPER_RESPAWN_AT_MS = nowMs + CREEPER_RESPAWN_PERIOD_MS;
            }
            if (nowMs >= CREEPER_RESPAWN_AT_MS) {
                creeperInit();
            }
        } else {
            CREEPER_RESPAWN_AT_MS = 0u;
        }
        return;
    }

    if (!CREEPER_IGNITE_ACTIVE && !CREEPER_EXPLOSION_ACTIVE && manualInput) {
        CREEPER_MANUAL_UNTIL_MS = nowMs + CREEPER_MANUAL_HOLD_MS;
        CREEPER_WAS_MANUAL = true;
        CREEPER_X += (float) state.dx;
        CREEPER_Y += (float) state.dy;
        if (state.dx != 0) {
            CREEPER_DIR_X = (state.dx > 0) ? 1 : -1;
            CREEPER_FACE_X = CREEPER_DIR_X;
        }
        if (CREEPER_X < 0.0f) {
            CREEPER_X = 0.0f;
        } else if (CREEPER_X > maxX) {
            CREEPER_X = maxX;
        }
        if (CREEPER_Y < 0.0f) {
            CREEPER_Y = 0.0f;
        } else if (CREEPER_Y > maxY) {
            CREEPER_Y = maxY;
        }
    }

    const bool manualActive =
            (!CREEPER_IGNITE_ACTIVE) && (!CREEPER_EXPLOSION_ACTIVE) && (nowMs < CREEPER_MANUAL_UNTIL_MS);
    if (!manualActive && CREEPER_WAS_MANUAL && !CREEPER_IGNITE_ACTIVE && !CREEPER_EXPLOSION_ACTIVE) {
        CREEPER_WAS_MANUAL = false;
        CREEPER_STATE = CreeperStateIdle;
        CREEPER_STATE_UNTIL_MS = nowMs + creeperRandRangeU32(CREEPER_IDLE_MIN_MS, 1400u);
        CREEPER_DIR_CHANGE_MS = nowMs + creeperRandRangeU32(CREEPER_TURN_MIN_MS, CREEPER_TURN_MAX_MS);
        creeperPickDirectionWithBounds(maxX, 4.0f);
    }

    if (!CREEPER_IGNITE_ACTIVE && !CREEPER_EXPLOSION_ACTIVE && !manualActive && CREEPER_STATE == CreeperStateEnter) {
        const float enterSpeed = 60.0f;
        CREEPER_X -= enterSpeed * dtS;
        if (CREEPER_X <= CREEPER_ENTRY_TARGET_X) {
            CREEPER_X = CREEPER_ENTRY_TARGET_X;
            CREEPER_STATE = CreeperStateIdle;
            CREEPER_STATE_UNTIL_MS = nowMs + creeperRandRangeU32(CREEPER_IDLE_MIN_MS, 1400u);
        }
    } else if (!CREEPER_IGNITE_ACTIVE && !CREEPER_EXPLOSION_ACTIVE && !manualActive &&
               CREEPER_STATE == CreeperStateIdle) {
        if (nowMs >= CREEPER_STATE_UNTIL_MS) {
            CREEPER_STATE = CreeperStateWalk;
            CREEPER_STATE_UNTIL_MS = nowMs + creeperRandRangeU32(1200u, 2600u);
            CREEPER_DIR_CHANGE_MS = nowMs + creeperRandRangeU32(CREEPER_TURN_MIN_MS, CREEPER_TURN_MAX_MS);
            creeperPickDirectionWithBounds(maxX, 4.0f);
        }
    } else if (!CREEPER_IGNITE_ACTIVE && !CREEPER_EXPLOSION_ACTIVE && !manualActive &&
               CREEPER_STATE == CreeperStateWalk) {
        if (nowMs >= CREEPER_STATE_UNTIL_MS) {
            CREEPER_STATE = CreeperStateIdle;
            CREEPER_STATE_UNTIL_MS = nowMs + creeperRandRangeU32(CREEPER_IDLE_MIN_MS, 1400u);
        } else {
            const float walkSpeed = 40.0f;
            if (nowMs >= CREEPER_DIR_CHANGE_MS) {
                CREEPER_DIR_CHANGE_MS = nowMs + creeperRandRangeU32(CREEPER_TURN_MIN_MS, CREEPER_TURN_MAX_MS);
                creeperPickDirectionWithBounds(maxX, 4.0f);
            }
            CREEPER_X += (float) CREEPER_DIR_X * walkSpeed * dtS;
            if (CREEPER_X < 0.0f) {
                CREEPER_X = 0.0f;
                CREEPER_DIR_X = 1;
                CREEPER_FACE_X = 1;
                CREEPER_DIR_CHANGE_MS = nowMs + creeperRandRangeU32(CREEPER_TURN_MIN_MS, CREEPER_TURN_MAX_MS);
            } else if (CREEPER_X > maxX) {
                CREEPER_X = maxX;
                CREEPER_DIR_X = -1;
                CREEPER_FACE_X = -1;
                CREEPER_DIR_CHANGE_MS = nowMs + creeperRandRangeU32(CREEPER_TURN_MIN_MS, CREEPER_TURN_MAX_MS);
            }
        }
    }

    if (CREEPER_EXPLOSION_ACTIVE) {
        const uint32_t elapsed = nowMs - CREEPER_EXPLOSION_START_MS;
        const uint32_t frame = elapsed / CREEPER_EXPLOSION_FRAME_MS;
        if (frame >= 15u) {
            CREEPER_EXPLOSION_ACTIVE = false;
            CREEPER_EXPLODED = true;
            return;
        }
        const int16_t offsetX = (int16_t) ((easter.creeperWidth - easter.explosionWidth) / 2);
        const int16_t offsetY = (int16_t) ((easter.creeperHeight - easter.explosionHeight) / 2);
        int16_t drawX = (int16_t) CREEPER_X + offsetX;
        int16_t drawY = (int16_t) CREEPER_Y + offsetY;
        if (drawX < 0) {
            drawX = 0;
        } else if (drawX > (int16_t) maxX) {
            drawX = (int16_t) maxX;
        }
        if (drawY < 0) {
            drawY = 0;
        } else if (drawY > (int16_t) maxY) {
            drawY = (int16_t) maxY;
        }
        vision_ui_driver_bmp_draw(
                (uint16_t) drawX,
                (uint16_t) drawY,
                (uint16_t) easter.explosionWidth,
                (uint16_t) easter.explosionHeight,
                easter.explosionEffects[frame]
        );
        const int16_t particleMaxX = (int16_t) maxInt(0, VISION_UI_SCREEN_WIDTH - easter.particleWidth);
        const int16_t particleMaxY = (int16_t) maxInt(0, VISION_UI_SCREEN_HEIGHT - easter.particleHeight);
        const int16_t centerX = (int16_t) (drawX + easter.explosionWidth / 2);
        const int16_t centerY = (int16_t) (drawY + easter.explosionHeight / 2);
        for (uint32_t i = 0; i < CREEPER_PARTICLE_COUNT; ++i) {
            const uint32_t particleFrame = (frame + i) % 5u;
            const int16_t px = creeperClampParticleAxis(
                    (int16_t) (centerX + CREEPER_PARTICLE_OFFSETS_X[i] - easter.particleWidth / 2), particleMaxX
            );
            const int16_t py = creeperClampParticleAxis(
                    (int16_t) (centerY + CREEPER_PARTICLE_OFFSETS_Y[i] - easter.particleHeight / 2), particleMaxY
            );
            vision_ui_driver_bmp_draw(
                    (uint16_t) px,
                    (uint16_t) py,
                    (uint16_t) easter.particleWidth,
                    (uint16_t) easter.particleHeight,
                    easter.particleEffects[particleFrame]
            );
        }
        return;
    }

    if (!CREEPER_EXPLODED) {
        const bool flashOn = CREEPER_IGNITE_ACTIVE && ((nowMs / CREEPER_FLASH_PERIOD_MS) % 2u == 0u);
        const uint8_t* sprite = NULL;
        if (flashOn) {
            sprite = CREEPER_FACE_X < 0 ? easter.creeperLeftBlowing : easter.creeperRightBlowing;
        } else {
            sprite = CREEPER_FACE_X < 0 ? easter.creeperLeft : easter.creeperRight;
        }
        vision_ui_driver_color_draw(0);
        vision_ui_driver_bmp_draw(
                (uint16_t) CREEPER_X,
                (uint16_t) CREEPER_Y,
                (uint16_t) easter.creeperWidth,
                (uint16_t) easter.creeperHeight,
                sprite
        );
    }
}

void creeperExit() {
    CREEPER_STATE = CreeperStateEnter;
    CREEPER_MANUAL_UNTIL_MS = 0u;
    CREEPER_WAS_MANUAL = false;
    CREEPER_IGNITE_ACTIVE = false;
    CREEPER_IGNITE_START_MS = 0u;
    CREEPER_EXPLOSION_ACTIVE = false;
    CREEPER_EXPLOSION_START_MS = 0u;
    CREEPER_EXPLODED = false;
    CREEPER_RESPAWN_AT_MS = 0u;
    creeperInitParticles();
}

void lumenLoadLayout() {
    vision_ui_font_set_title(lumenGetSystemConfig().title);
    vision_ui_font_set_subtitle(lumenGetSystemConfig().subtitle);
    vision_ui_font_set(lumenGetSystemConfig().normal);
    vision_ui_minifont_set(lumenGetSystemConfig().mini);
    vision_ui_list_icon_set(lumenGetSystemConfig().icon);
    vision_ui_start_logo_set(lumenGetSystemConfig().logo, lumenGetSystemConfig().logoSpan);

    vision_ui_list_item_t* iconView = vision_ui_list_item_new(6, true, "VisionUI");

    vision_ui_root_item_set(iconView);
    vision_ui_core_init();

    vision_ui_list_item_t* systemList =
            vision_ui_list_icon_item_new(4, lumenGetSystemConfig().systemIcon, "System", "Settings");

    vision_ui_list_item_t* motionFeature =
            vision_ui_list_icon_item_new(1, lumenGetSystemConfig().motionIcon, "Motion", "Movement info");
    vision_ui_list_push_item(
            motionFeature, vision_ui_list_user_item_new("", motionInitCallback, motionLoop, motionExit)
    );

    vision_ui_list_item_t* usbFeature =
            vision_ui_list_icon_item_new(5, lumenGetSystemConfig().usbIcon, "USB", "Port Configuration");
    vision_ui_list_push_item(usbFeature, vision_ui_list_title_item_new("USB"));
    vision_ui_list_push_item(
            usbFeature,
            vision_ui_list_slider_item_new(
                    "Overcurrent Limit",
                    lumenGetUSBInfo().overCurrentDefault,
                    10,
                    lumenGetUSBInfo().overCurrentMin,
                    lumenGetUSBInfo().hardwareLimitedCurrent,
                    lumenSetConfigCallbacks().overcurrentOnChange
            )
    );
    vision_ui_list_push_item(
            usbFeature,
            vision_ui_list_slider_item_new(
                    "Overvoltage Limit",
                    lumenGetUSBInfo().overVoltageDefault,
                    100,
                    lumenGetUSBInfo().overVoltageMin,
                    lumenGetUSBInfo().overVoltageMax,
                    lumenSetConfigCallbacks().overvoltageOnChange
            )
    );
    vision_ui_list_push_item(
            usbFeature,
            vision_ui_list_switch_item_new(
                    "Enable AUTO Fault Recovery", false, lumenSetConfigCallbacks().enableAutoFaultRecoveryOnChange
            )
    );
    vision_ui_list_push_item(
            usbFeature,
            vision_ui_list_switch_item_new("Turn off USB", false, lumenSetConfigCallbacks().turnOffUsbOnChange)
    );

    vision_ui_list_item_t* statsFeature =
            vision_ui_list_icon_item_new(1, lumenGetSystemConfig().statIcon, "Stats", "Live view");
    vision_ui_list_push_item(statsFeature, vision_ui_list_user_item_new("", statsInit, statsLoop, statsExit));

    vision_ui_list_item_t* creeperFeature =
            vision_ui_list_icon_item_new(1, lumenGetSystemConfig().creeperIcon, "Creeper", "Aww man");
    vision_ui_list_push_item(creeperFeature, vision_ui_list_user_item_new("", creeperInit, creeperLoop, creeperExit));

    vision_ui_list_item_t* minecraftSyncFeature =
            vision_ui_list_icon_item_new(1, lumenGetSystemConfig().minecraftSyncIcon, "MC Sync", "Guess what");
    vision_ui_list_push_item(
            minecraftSyncFeature,
            vision_ui_list_user_item_new(
                    "",
                    lumenGetMinecraftSync().initFunction,
                    lumenGetMinecraftSync().loopFunction,
                    lumenGetMinecraftSync().exitFunction
            )
    );

    vision_ui_list_push_item(iconView, creeperFeature);
    vision_ui_list_push_item(iconView, statsFeature);
    vision_ui_list_push_item(iconView, usbFeature);
    vision_ui_list_push_item(iconView, motionFeature);
    vision_ui_list_push_item(iconView, minecraftSyncFeature);
    vision_ui_list_push_item(iconView, systemList);

    vision_ui_list_item_t* aboutList = vision_ui_list_item_new(6, false, "About");
    vision_ui_list_push_item(aboutList, vision_ui_list_title_item_new("About"));
    vision_ui_list_push_item(aboutList, vision_ui_list_item_new(0, false, "Project Lumen"));
    vision_ui_list_push_item(aboutList, vision_ui_list_item_new(0, false, "Author: Finn Sheng"));
    vision_ui_list_push_item(aboutList, vision_ui_list_item_new(0, false, lumenGetSystemInfo().version));
    vision_ui_list_push_item(aboutList, vision_ui_list_item_new(0, false, lumenGetSystemInfo().build));
    vision_ui_list_push_item(aboutList, vision_ui_list_item_new(0, false, lumenGetSystemInfo().commit));

    vision_ui_list_push_item(systemList, vision_ui_list_title_item_new("System"));
    vision_ui_list_push_item(
            systemList,
            vision_ui_list_switch_item_new(
                    "Overvoltage Alert", true, lumenSetConfigCallbacks().overvoltageAlertOnChange
            )
    );
    vision_ui_list_push_item(
            systemList,
            vision_ui_list_switch_item_new(
                    "Overcurrent Alert", true, lumenSetConfigCallbacks().overcurrentAlertOnChange
            )
    );
    vision_ui_list_push_item(systemList, aboutList);

    vision_ui_render_init();
}
