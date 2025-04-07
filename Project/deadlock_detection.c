/*
 * FreeRTOS死锁检测模块
 * 提供死锁检测和自动复位功能
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "deadlock_detection.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

/* 定义互斥量跟踪数组 */
static MutexInfo_t xMutexList[configMAX_MUTEX_TRACKING];
static SemaphoreHandle_t xMutexListLock = NULL;
static volatile UBaseType_t uxMutexCount = 0;

/* 死锁检测任务句柄 */
static TaskHandle_t xDeadlockDetectionTaskHandle = NULL;

/* 死锁检测任务的优先级和堆栈大小 */
#define DEADLOCK_DETECTION_TASK_PRIORITY  (tskIDLE_PRIORITY + 1)
#define DEADLOCK_DETECTION_TASK_STACK     (configMINIMAL_STACK_SIZE)

/* 声明所有静态函数 */
static void prvDeadlockDetectionTask(void *pvParameters);
static BaseType_t prvFindMutexInList(SemaphoreHandle_t mutex, UBaseType_t *puxIndex);
static void prvPrintTaskHeldMutexes(TaskHandle_t xTask);

/**
 * 初始化死锁检测模块
 */
void vDeadlockDetectionInit(void)
{
    /* 清空互斥量跟踪数组 */
    memset(xMutexList, 0, sizeof(xMutexList));
    
    /* 创建保护互斥量列表的锁 */
    xMutexListLock = xSemaphoreCreateMutex();
    configASSERT(xMutexListLock != NULL);
    
    /* 初始化状态 */
    uxMutexCount = 0;
    
    /* 启动死锁检测任务 */
    vStartDeadlockDetectionTask();
}

/**
 * 创建互斥量并注册到死锁检测模块
 */
SemaphoreHandle_t xCreateMutexWithDeadlockDetection(const char *name)
{
    SemaphoreHandle_t xNewMutex = NULL;
    
    /* 创建标准互斥量 */
    xNewMutex = xSemaphoreCreateMutex();
    
    /* 确保创建成功 */
    if (xNewMutex != NULL)
    {
        /* 获取访问互斥量列表的锁 */
        if (xMutexListLock != NULL && xSemaphoreTake(xMutexListLock, portMAX_DELAY) == pdTRUE)
        {
            /* 检查是否有空间添加新的互斥量 */
            if (uxMutexCount < configMAX_MUTEX_TRACKING)
            {
                /* 注册到跟踪数组 */
                xMutexList[uxMutexCount].mutex = xNewMutex;
                xMutexList[uxMutexCount].holder = NULL;
                xMutexList[uxMutexCount].acquireTime = 0;
                xMutexList[uxMutexCount].mutexName = name;
                
                uxMutexCount++;
            }
            else
            {
                /* 没有足够空间跟踪这个互斥量 */
                printf("警告: 互斥量跟踪数组已满，无法注册新互斥量\r\n");
            }
            
            /* 释放互斥量列表的锁 */
            xSemaphoreGive(xMutexListLock);
        }
    }
    
    return xNewMutex;
}

/**
 * 使用超时参数获取互斥量
 */
BaseType_t xTakeMutexWithDeadlockDetection(SemaphoreHandle_t mutex, TickType_t timeout)
{
    BaseType_t xResult;
    UBaseType_t uxIndex;
    
    /* 尝试获取互斥量 */
    xResult = xSemaphoreTake(mutex, timeout);
    
    if (xResult == pdTRUE)
    {
        /* 成功获取互斥量，更新跟踪信息 */
        if (xMutexListLock != NULL && xSemaphoreTake(xMutexListLock, portMAX_DELAY) == pdTRUE)
        {
            /* 查找互斥量在列表中的位置 */
            if (prvFindMutexInList(mutex, &uxIndex) == pdTRUE)
            {
                /* 更新持有者和获取时间 */
                xMutexList[uxIndex].holder = xTaskGetCurrentTaskHandle();
                xMutexList[uxIndex].acquireTime = xTaskGetTickCount();
            }
            
            /* 释放互斥量列表的锁 */
            xSemaphoreGive(xMutexListLock);
        }
    }
    
    return xResult;
}

/**
 * 释放互斥量
 */
BaseType_t xGiveMutexWithDeadlockDetection(SemaphoreHandle_t mutex)
{
    BaseType_t xResult;
    UBaseType_t uxIndex;
    
    /* 尝试释放互斥量 */
    xResult = xSemaphoreGive(mutex);
    
    if (xResult == pdTRUE)
    {
        /* 成功释放互斥量，更新跟踪信息 */
        if (xMutexListLock != NULL && xSemaphoreTake(xMutexListLock, portMAX_DELAY) == pdTRUE)
        {
            /* 查找互斥量在列表中的位置 */
            if (prvFindMutexInList(mutex, &uxIndex) == pdTRUE)
            {
                /* 清除持有者和获取时间 */
                xMutexList[uxIndex].holder = NULL;
                xMutexList[uxIndex].acquireTime = 0;
            }
            
            /* 释放互斥量列表的锁 */
            xSemaphoreGive(xMutexListLock);
        }
    }
    
    return xResult;
}

/**
 * 开始死锁检测任务
 */
void vStartDeadlockDetectionTask(void)
{
#if (configENABLE_DEADLOCK_DETECTION == 1)
    /* 创建死锁检测任务 */
    xTaskCreate(prvDeadlockDetectionTask,
                "DeadlockDet",
                DEADLOCK_DETECTION_TASK_STACK,
                NULL,
                DEADLOCK_DETECTION_TASK_PRIORITY,
                &xDeadlockDetectionTaskHandle);
                
    configASSERT(xDeadlockDetectionTaskHandle != NULL);
#endif
}

/**
 * 打印指定任务持有的所有互斥量
 * @param xTask 要检查的任务句柄
 */
static void prvPrintTaskHeldMutexes(TaskHandle_t xTask)
{
    const char *pcTaskName = pcTaskGetName(xTask);
    UBaseType_t uxHeldCount = 0;
    
    printf("任务 %s 持有的互斥量列表:\r\n", pcTaskName);
    
    /* 检查所有互斥量 */
    for (UBaseType_t i = 0; i < uxMutexCount; i++)
    {
        if (xMutexList[i].holder == xTask)
        {
            printf("  - %s (持有时间: %u ms)\r\n", 
                  xMutexList[i].mutexName != NULL ? xMutexList[i].mutexName : "未命名",
                  (unsigned int)((xTaskGetTickCount() - xMutexList[i].acquireTime) * portTICK_PERIOD_MS));
            uxHeldCount++;
        }
    }
    
    if (uxHeldCount == 0)
    {
        printf("  没有持有任何互斥量\r\n");
    }
}

/**
 * 复位系统（在检测到死锁时调用）
 */
void vDeadlockSystemReset(void)
{
    const char *pcCurrentTaskName = pcTaskGetName(xTaskGetCurrentTaskHandle());
    TaskHandle_t xInvolvedTasks[configMAX_MUTEX_TRACKING] = {NULL};
    UBaseType_t uxInvolvedTaskCount = 0;
    
    /* 打印死锁警告 */
    printf("检测到死锁！系统将重置...（触发任务: %s）\r\n", pcCurrentTaskName);
    
    /* 收集所有持有互斥量的任务 */
    for (UBaseType_t i = 0; i < uxMutexCount; i++)
    {
        if (xMutexList[i].holder != NULL)
        {
            /* 检查这个任务是否已经在列表中 */
            BaseType_t xFound = pdFALSE;
            for (UBaseType_t j = 0; j < uxInvolvedTaskCount; j++)
            {
                if (xInvolvedTasks[j] == xMutexList[i].holder)
                {
                    xFound = pdTRUE;
                    break;
                }
            }
            
            /* 如果任务不在列表中，添加它 */
            if (xFound == pdFALSE && uxInvolvedTaskCount < configMAX_MUTEX_TRACKING)
            {
                xInvolvedTasks[uxInvolvedTaskCount++] = xMutexList[i].holder;
            }
        }
    }
    
    /* 打印所有被锁定的互斥量状态 */
    printf("死锁相关的互斥量状态:\r\n");
    for (UBaseType_t i = 0; i < uxMutexCount; i++)
    {
        if (xMutexList[i].holder != NULL)
        {
            printf("互斥量 %s 被任务 %s 持有 (持有时间: %u ms)\r\n", 
                   xMutexList[i].mutexName != NULL ? xMutexList[i].mutexName : "未命名",
                   pcTaskGetName(xMutexList[i].holder),
                   (unsigned int)((xTaskGetTickCount() - xMutexList[i].acquireTime) * portTICK_PERIOD_MS));
        }
    }
    
    /* 打印每个相关任务持有的所有互斥量 */
    printf("\n相关任务持有的互斥量详情:\r\n");
    for (UBaseType_t i = 0; i < uxInvolvedTaskCount; i++)
    {
        prvPrintTaskHeldMutexes(xInvolvedTasks[i]);
    }
    
    /* 在实际的嵌入式系统中，这里应该调用系统复位函数 */
    /* 在模拟环境中，我们直接退出程序 */
    printf("模拟系统复位...\r\n");
    fflush(stdout);
    
    /* 退出程序 - 模拟系统复位 */
    exit(0);
}

/**
 * 死锁检测任务
 */
static void prvDeadlockDetectionTask(void *pvParameters)
{
    TickType_t xCurrentTime;
    
    /* 避免编译器警告 */
    (void)pvParameters;
    
    for (;;)
    {
        /* 每秒检查一次 */
        vTaskDelay(pdMS_TO_TICKS(1000));
        
        /* 获取当前时间 */
        xCurrentTime = xTaskGetTickCount();
        
        /* 获取访问互斥量列表的锁 */
        if (xMutexListLock != NULL && xSemaphoreTake(xMutexListLock, portMAX_DELAY) == pdTRUE)
        {
            /* 检查所有被跟踪的互斥量 */
            for (UBaseType_t i = 0; i < uxMutexCount; i++)
            {
                /* 检查是否有任务持有这个互斥量 */
                if (xMutexList[i].holder != NULL)
                {
                    /* 计算持有时间 */
                    TickType_t xHoldTime = xCurrentTime - xMutexList[i].acquireTime;
                    
                    /* 检查是否超过超时时间 */
                    if (xHoldTime > configDEADLOCK_DETECTION_TIMEOUT)
                    {
                        /* 获取任务名称 */
                        const char *pcHolderTaskName = pcTaskGetName(xMutexList[i].holder);
                        const char *pcCurrentTaskName = pcTaskGetName(xTaskGetCurrentTaskHandle());
                        
                        /* 打印死锁警告 */
                        printf("死锁检测: 互斥量 %s 被任务 %s 持有超过 %u 毫秒，当前任务 %s 正在等待或检测\r\n",
                               xMutexList[i].mutexName != NULL ? xMutexList[i].mutexName : "未命名",
                               pcHolderTaskName,
                               (unsigned int)(xHoldTime * portTICK_PERIOD_MS),
                               pcCurrentTaskName);
                        
                        /* 打印持有者任务持有的所有互斥量 */
                        prvPrintTaskHeldMutexes(xMutexList[i].holder);
                        
                        /* 释放互斥量列表的锁 */
                        xSemaphoreGive(xMutexListLock);
                        
                        /* 触发系统复位 */
                        vDeadlockSystemReset();
                        
                        /* 退出循环 */
                        break;
                    }
                }
            }
            
            /* 释放互斥量列表的锁 */
            xSemaphoreGive(xMutexListLock);
        }
    }
}

/**
 * 查找互斥量在列表中的位置
 */
static BaseType_t prvFindMutexInList(SemaphoreHandle_t mutex, UBaseType_t *puxIndex)
{
    BaseType_t xFound = pdFALSE;
    
    /* 遍历互斥量列表 */
    for (UBaseType_t i = 0; i < uxMutexCount; i++)
    {
        if (xMutexList[i].mutex == mutex)
        {
            *puxIndex = i;
            xFound = pdTRUE;
            break;
        }
    }
    
    return xFound;
} 