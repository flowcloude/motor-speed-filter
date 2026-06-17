/**
  ******************************************************************************
  * @file    speed_filter.h
  * @brief   速度滤波器接口
  ******************************************************************************
  */
#ifndef SPEED_FILTER_H
#define SPEED_FILTER_H

#include "mc_type.h"

typedef enum {
    FILTER_NONE = 0, FILTER_MOVING_AVG, FILTER_LPF_1ST,
    FILTER_KALMAN, FILTER_MEDIAN, FILTER_COUNT
} FilterType_t;

#define FILTER_LOG_SIZE  50

extern int16_t log_raw[FILTER_LOG_SIZE];
extern int16_t log_mavg[FILTER_LOG_SIZE];
extern int16_t log_lpf1[FILTER_LOG_SIZE];
extern int16_t log_kalm[FILTER_LOG_SIZE];
extern int16_t log_med[FILTER_LOG_SIZE];
extern uint16_t log_cnt;

/* 滤波器实时输出 - Motor Pilot可读取 */
extern int16_t curRaw, curMavg, curLpf1, curKalm, curMed;

void   SpeedFilter_Init(void);
int16_t SpeedFilter_UpdateAll(int16_t rawSpeed);
FilterType_t SpeedFilter_GetActive(void);
void   SpeedFilter_SetActive(FilterType_t type);
void   SpeedFilter_SetAlpha(float alpha);
const char* SpeedFilter_GetName(FilterType_t type);
#endif
