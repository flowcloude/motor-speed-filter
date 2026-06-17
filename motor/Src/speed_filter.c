/**
  ******************************************************************************
  * @file    speed_filter.c
  * @brief   4种滤波同时运行 + 5个 int16 数组自动记录
  ******************************************************************************
  */
#include "speed_filter.h"
#include <string.h>

#define MOVING_AVG_WINDOW   8
#define MEDIAN_WINDOW_SIZE  5
#define KALMAN_Q            0.1f
#define KALMAN_R            0.5f
#define DEFAULT_ALPHA       0.15f
#define LOG_INTERVAL        20

int16_t  log_raw[FILTER_LOG_SIZE];
int16_t  log_mavg[FILTER_LOG_SIZE];
int16_t  log_lpf1[FILTER_LOG_SIZE];
int16_t  log_kalm[FILTER_LOG_SIZE];
int16_t  log_med[FILTER_LOG_SIZE];
uint16_t log_cnt;

static bool filterSeeded = false;
static uint16_t logThrottle = 0;
static FilterType_t activeFilter = FILTER_LPF_1ST;

static int16_t  maBuf[MOVING_AVG_WINDOW];
static uint8_t  maIdx;
static float    lpfA = DEFAULT_ALPHA;
static int16_t  lpfV;
static float    kEst, kPest;
static int16_t  medBuf[MEDIAN_WINDOW_SIZE];
static uint8_t  medIdx;
int16_t  curRaw, curMavg, curLpf1, curKalm, curMed;

static void ma_update(int16_t in) {
    maBuf[maIdx] = in; maIdx = (maIdx + 1) % MOVING_AVG_WINDOW;
    int32_t s = 0; for (int i = 0; i < MOVING_AVG_WINDOW; i++) s += maBuf[i];
    curMavg = (int16_t)(s / MOVING_AVG_WINDOW);
}
static void lpf_update(int16_t in) {
    lpfV = (int16_t)(lpfA * in + (1.0f - lpfA) * lpfV); curLpf1 = lpfV;
}
static void kal_update(int16_t in) {
    float p = kPest + KALMAN_Q; float g = p / (p + KALMAN_R);
    kEst = g * in + (1.0f - g) * kEst; kPest = (1.0f - g) * p;
    curKalm = (int16_t)kEst;
}
static void med_update(int16_t in) {
    medBuf[medIdx] = in; medIdx = (medIdx + 1) % MEDIAN_WINDOW_SIZE;
    int16_t t[MEDIAN_WINDOW_SIZE]; memcpy(t, medBuf, sizeof(t));
    for (int i = 0; i < MEDIAN_WINDOW_SIZE - 1; i++)
        for (int j = i+1; j < MEDIAN_WINDOW_SIZE; j++)
            if (t[i] > t[j]) { int16_t x = t[i]; t[i]=t[j]; t[j]=x; }
    curMed = t[MEDIAN_WINDOW_SIZE / 2];
}

void SpeedFilter_Init(void)
{
    memset(log_raw,  0, sizeof(log_raw));
    memset(log_mavg, 0, sizeof(log_mavg));
    memset(log_lpf1, 0, sizeof(log_lpf1));
    memset(log_kalm, 0, sizeof(log_kalm));
    memset(log_med,  0, sizeof(log_med));
    log_cnt = 0; filterSeeded = false; logThrottle = 0;
    activeFilter = FILTER_LPF_1ST;
    maIdx = 0; lpfV = 0; kEst = 0; kPest = 1; medIdx = 0;
    curRaw = 0; curMavg = 0; curLpf1 = 0; curKalm = 0; curMed = 0;
}

int16_t SpeedFilter_UpdateAll(int16_t rawSpeed)
{
    curRaw = rawSpeed;

    if (!filterSeeded) {
        filterSeeded = true;
        for (int i = 0; i < MOVING_AVG_WINDOW; i++) maBuf[i] = rawSpeed;
        lpfV = rawSpeed; kEst = (float)rawSpeed;
        for (int i = 0; i < MEDIAN_WINDOW_SIZE; i++) medBuf[i] = rawSpeed;
        curMavg = curLpf1 = curKalm = curMed = rawSpeed;
    } else {
        ma_update(rawSpeed); lpf_update(rawSpeed);
        kal_update(rawSpeed); med_update(rawSpeed);
    }

    logThrottle++;
    if (logThrottle >= LOG_INTERVAL && log_cnt < FILTER_LOG_SIZE) {
        logThrottle = 0;
        log_raw[log_cnt]  = curRaw;
        log_mavg[log_cnt] = curMavg;
        log_lpf1[log_cnt] = curLpf1;
        log_kalm[log_cnt] = curKalm;
        log_med[log_cnt]  = curMed;
        log_cnt++;
    }

    switch (activeFilter) {
        case FILTER_NONE: return curRaw; case FILTER_MOVING_AVG: return curMavg;
        case FILTER_LPF_1ST: return curLpf1; case FILTER_KALMAN: return curKalm;
        case FILTER_MEDIAN: return curMed; default: return curRaw;
    }
}

FilterType_t SpeedFilter_GetActive(void)    { return activeFilter; }
void SpeedFilter_SetActive(FilterType_t t) { if (t < FILTER_COUNT) activeFilter = t; }
void SpeedFilter_SetAlpha(float a)         { if (a > 0.0f && a <= 1.0f) lpfA = a; }
const char* SpeedFilter_GetName(FilterType_t t) {
    switch (t) {
        case FILTER_NONE: return "NONE"; case FILTER_MOVING_AVG: return "MOVING_AVG";
        case FILTER_LPF_1ST: return "LPF_1ST"; case FILTER_KALMAN: return "KALMAN";
        case FILTER_MEDIAN: return "MEDIAN"; default: return "?";
    }
}
