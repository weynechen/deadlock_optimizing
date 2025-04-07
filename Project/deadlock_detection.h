/*
 * FreeRTOS死锁检测模块
 * 提供死锁检测和自动复位功能
 */

#ifndef DEADLOCK_DETECTION_H
#define DEADLOCK_DETECTION_H

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

/* 配置死锁检测超时时间（以tick为单位） */
#ifndef configDEADLOCK_DETECTION_TIMEOUT
    #define configDEADLOCK_DETECTION_TIMEOUT    ( ( TickType_t ) 5000 ) /* 默认5秒 */
#endif

/* 配置是否启用死锁检测 */
#ifndef configENABLE_DEADLOCK_DETECTION
    #define configENABLE_DEADLOCK_DETECTION     1
#endif

/* 互斥量信息结构体 */
typedef struct MutexInfo
{
    SemaphoreHandle_t mutex;       /* 互斥量句柄 */
    TaskHandle_t holder;           /* 当前持有者 */
    TickType_t acquireTime;        /* 获取时间 */
    const char *mutexName;         /* 互斥量名称（可选） */
} MutexInfo_t;

/* 互斥量跟踪数组大小 */
#ifndef configMAX_MUTEX_TRACKING
    #define configMAX_MUTEX_TRACKING      10
#endif

/* 函数原型 */

/**
 * 初始化死锁检测模块
 */
void vDeadlockDetectionInit(void);

/**
 * 创建互斥量并注册到死锁检测模块
 *
 * @param name 互斥量名称，用于调试
 * @return 互斥量句柄
 */
SemaphoreHandle_t xCreateMutexWithDeadlockDetection(const char *name);

/**
 * 使用超时参数获取互斥量
 *
 * @param mutex 互斥量句柄
 * @param timeout 超时时间
 * @return pdTRUE 成功获取，pdFALSE 获取失败
 */
BaseType_t xTakeMutexWithDeadlockDetection(SemaphoreHandle_t mutex, TickType_t timeout);

/**
 * 释放互斥量
 *
 * @param mutex 互斥量句柄
 * @return pdTRUE 成功释放，pdFALSE 释放失败
 */
BaseType_t xGiveMutexWithDeadlockDetection(SemaphoreHandle_t mutex);

/**
 * 开始死锁检测任务
 */
void vStartDeadlockDetectionTask(void);

/**
 * 复位系统（在检测到死锁时调用）
 */
void vDeadlockSystemReset(void);

#endif /* DEADLOCK_DETECTION_H */ 