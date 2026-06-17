# 电机速度估计与滤波方法

基于 STM32G474RETx + ST Motor Control SDK 6.4.2 实现的永磁同步电机 FOC 速度滤波课程设计。

## 项目简介

在电机 FOC 控制中，通过 STO-PLL 无传感器算法估计电机转速，但估计值存在噪声。本项目设计并实现四种数字滤波器，对速度估计值进行实时滤波处理，并通过 ST Motor Pilot 上位机进行实时波形对比分析。

## 四种滤波算法

| 滤波方式 | 核心方法 | 参数 |
|---------|---------|------|
| 滑动平均滤波 (Moving Average) | 8 点滑动窗口取均值 | 窗口大小 = 8 |
| 一阶低通滤波 (First-Order LPF) | y[n] = alpha * x[n] + (1-alpha) * y[n-1] | alpha = 0.15 |
| 卡尔曼滤波 (Kalman) | 预测-更新递归估计 | Q=0.1, R=0.5 |
| 中值滤波 (Median) | 5 点窗口取中值 | 窗口大小 = 5 |

## 项目结构

```
motor/
├── Inc/                          # 头文件
│   ├── speed_filter.h            # 滤波器接口定义
│   └── register_interface.h      # 自定义寄存器 ID 定义 (200~204)
├── Src/                          # 源文件
│   ├── speed_filter.c            # 4种滤波算法实现 + 自动数据记录
│   ├── mc_tasks_foc.c            # FOC 控制任务（滤波器集成点）
│   ├── hf_registers.c            # 高频寄存器映射（Motor Pilot 轮询）
│   └── sync_registers.c          # 寄存器同步处理（ASPEP 协议）
└── MDK-ARM/                      # Keil MDK 工程
    ├── motor.uvprojx             # Keil 工程文件
    └── startup_stm32g474xx.s     # 启动文件
```

## 核心设计

### 数据流

```
STO_PLL 速度估计
    │
    ▼
SpeedFilter_UpdateAll(rawSpeed)
    ├── 滑动平均滤波 ──► curMavg
    ├── 一阶低通滤波 ──► curLpf1
    ├── 卡尔曼滤波  ──► curKalm
    └── 中值滤波    ──► curMed
    │
    ▼
回写 hAvrMecSpeedUnit → 速度PID → 电流环 → SVPWM → 电机
    │
    ▼
Motor Pilot 高频波形窗口
  寄存器 ID 200~204，5 条曲线实时对比
```

### 种子初始化

为防止滤波器从 0 状态启动导致电机速度环振荡，首次调用 `SpeedFilter_UpdateAll()` 时，将所有滤波器内部状态统一初始化为当前速度值（`filterSeeded` 机制）。

### 自动数据采集

每 20 次滤波器调用记录一条数据，5 个 `int16_t[50]` 数组分别存储原始速度及四种滤波输出，支持后续离线分析。

## 开发环境

| 工具 | 版本 |
|------|------|
| Keil MDK-ARM | 5.x |
| STM32CubeMX | 6.x |
| ST Motor Control SDK | 6.4.2 |
| ST Motor Pilot | 6.4.2 |
| 目标芯片 | STM32G474RETx |

## Motor Pilot 使用

1. 编译下载固件，电机上电运行
2. 打开 ST Motor Pilot V6.4.2
3. 选择 UART，波特率 1843200，连接
4. Register 页签搜索 "FILT"，勾选 Poll + Plot
5. High-frequency Plot 窗口查看 5 条滤波曲线实时对比

## License

本项目中 STM32 相关代码版权归 STMicroelectronics 所有。自定义滤波器代码用于课程设计学习目的。
