/*
 * FreeRTOS死锁检测演示
 * 此示例故意创建一个死锁场景来测试死锁检测功能
 */

#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "deadlock_detection.h"

/* 定义任务优先级 */
#define TASK1_PRIORITY      (tskIDLE_PRIORITY + 1)
#define TASK2_PRIORITY      (tskIDLE_PRIORITY + 1)

/* 定义任务栈大小 */
#define TASK_STACK_SIZE     (configMINIMAL_STACK_SIZE)

/* 定义任务句柄 */
static TaskHandle_t xTask1Handle = NULL;
static TaskHandle_t xTask2Handle = NULL;

/* 定义互斥锁 */
static SemaphoreHandle_t xMutex1 = NULL;
static SemaphoreHandle_t xMutex2 = NULL;

/* 任务函数声明 */
static void vTask1(void *pvParameters);
static void vTask2(void *pvParameters);

/* 演示入口函数 */
void vStartDeadlockDemo(void)
{
    printf("开始死锁检测演示\r\n");
    
    /* 初始化死锁检测模块 */
    vDeadlockDetectionInit();
    
    /* 创建互斥量 */
    xMutex1 = xCreateMutexWithDeadlockDetection("Mutex1");
    xMutex2 = xCreateMutexWithDeadlockDetection("Mutex2");
    
    /* 创建任务 */
    xTaskCreate(vTask1, "Task1", TASK_STACK_SIZE, NULL, TASK1_PRIORITY, &xTask1Handle);
    xTaskCreate(vTask2, "Task2", TASK_STACK_SIZE, NULL, TASK2_PRIORITY, &xTask2Handle);
}

/*
 * 任务1 - 首先获取互斥量1，然后尝试获取互斥量2
 */
static void vTask1(void *pvParameters)
{
    /* 避免编译器警告 */
    (void)pvParameters;
    
    for (;;)
    {
        printf("任务1: 尝试获取互斥量1\r\n");
        
        /* 获取互斥量1 */
        if (xTakeMutexWithDeadlockDetection(xMutex1, portMAX_DELAY) == pdTRUE)
        {
            printf("任务1: 成功获取互斥量1\r\n");
            
            /* 短暂延迟，确保任务2有机会获取互斥量2 */
            vTaskDelay(pdMS_TO_TICKS(100));
            
            printf("任务1: 尝试获取互斥量2\r\n");
            
            /* 尝试获取互斥量2 - 可能导致死锁 */
            if (xTakeMutexWithDeadlockDetection(xMutex2, portMAX_DELAY) == pdTRUE)
            {
                printf("任务1: 成功获取互斥量2\r\n");
                
                /* 使用互斥量保护的资源 */
                vTaskDelay(pdMS_TO_TICKS(10));
                
                /* 释放互斥量2 */
                xGiveMutexWithDeadlockDetection(xMutex2);
                printf("任务1: 释放互斥量2\r\n");
            }
            
            /* 释放互斥量1 */
            xGiveMutexWithDeadlockDetection(xMutex1);
            printf("任务1: 释放互斥量1\r\n");
        }
        
        /* 延迟一段时间再尝试 */
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

/*
 * 任务2 - 首先获取互斥量2，然后尝试获取互斥量1
 */
static void vTask2(void *pvParameters)
{
    /* 避免编译器警告 */
    (void)pvParameters;
    
    /* 短暂延迟，确保任务1先运行 */
    vTaskDelay(pdMS_TO_TICKS(50));
    
    for (;;)
    {
        printf("任务2: 尝试获取互斥量2\r\n");
        
        /* 获取互斥量2 */
        if (xTakeMutexWithDeadlockDetection(xMutex2, portMAX_DELAY) == pdTRUE)
        {
            printf("任务2: 成功获取互斥量2\r\n");
            
            /* 短暂延迟，确保任务1有机会获取互斥量1 */
            vTaskDelay(pdMS_TO_TICKS(100));
            
            printf("任务2: 尝试获取互斥量1\r\n");
            
            /* 尝试获取互斥量1 - 可能导致死锁 */
            if (xTakeMutexWithDeadlockDetection(xMutex1, portMAX_DELAY) == pdTRUE)
            {
                printf("任务2: 成功获取互斥量1\r\n");
                
                /* 使用互斥量保护的资源 */
                vTaskDelay(pdMS_TO_TICKS(10));
                
                /* 释放互斥量1 */
                xGiveMutexWithDeadlockDetection(xMutex1);
                printf("任务2: 释放互斥量1\r\n");
            }
            
            /* 释放互斥量2 */
            xGiveMutexWithDeadlockDetection(xMutex2);
            printf("任务2: 释放互斥量2\r\n");
        }
        
        /* 延迟一段时间再尝试 */
        vTaskDelay(pdMS_TO_TICKS(500));
    }
} 