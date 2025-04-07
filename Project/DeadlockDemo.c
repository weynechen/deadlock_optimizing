/*
 * FreeRTOS互斥锁死锁演示
 * 这个示例演示了如何通过不正确的互斥锁使用顺序导致死锁
 */

#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

/* 互斥锁句柄 */
static SemaphoreHandle_t xMutex1 = NULL;
static SemaphoreHandle_t xMutex2 = NULL;

/* 任务句柄 */
static TaskHandle_t xTask1Handle = NULL;
static TaskHandle_t xTask2Handle = NULL;

/* 任务优先级 */
#define TASK1_PRIORITY      (tskIDLE_PRIORITY + 1)
#define TASK2_PRIORITY      (tskIDLE_PRIORITY + 1)

/* 任务延迟时间 */
#define DEADLOCK_DELAY_MS   (500)

/* 任务堆栈大小 */
#define DEADLOCK_TASK_STACK_SIZE  (configMINIMAL_STACK_SIZE)

/* 任务函数 */
static void prvTask1(void *pvParameters);
static void prvTask2(void *pvParameters);

/* 初始化函数 */
void vStartDeadlockDemo(void)
{
    /* 创建互斥锁 */
    xMutex1 = xSemaphoreCreateMutex();
    xMutex2 = xSemaphoreCreateMutex();
    
    if (xMutex1 != NULL && xMutex2 != NULL)
    {
        /* 创建两个任务，它们将以相反的顺序获取互斥锁，从而导致死锁 */
        xTaskCreate(prvTask1, "DeadTask1", DEADLOCK_TASK_STACK_SIZE, NULL, TASK1_PRIORITY, &xTask1Handle);
        xTaskCreate(prvTask2, "DeadTask2", DEADLOCK_TASK_STACK_SIZE, NULL, TASK2_PRIORITY, &xTask2Handle);
        
        printf("死锁演示任务已启动\n");
    }
    else
    {
        printf("无法创建互斥锁，死锁演示未启动\n");
        
        /* 释放可能已创建的互斥锁 */
        if (xMutex1 != NULL)
        {
            vSemaphoreDelete(xMutex1);
        }
        
        if (xMutex2 != NULL)
        {
            vSemaphoreDelete(xMutex2);
        }
    }
}

/*
 * 任务1：首先获取互斥锁1，然后尝试获取互斥锁2
 */
static void prvTask1(void *pvParameters)
{
    (void)pvParameters;
    
    for (;;)
    {
        printf("任务1：尝试获取互斥锁1\n");
        if (xSemaphoreTake(xMutex1, portMAX_DELAY) == pdTRUE)
        {
            printf("任务1：成功获取互斥锁1\n");
            
            /* 故意延迟一段时间，增加发生死锁的几率 */
            vTaskDelay(pdMS_TO_TICKS(DEADLOCK_DELAY_MS));
            
            printf("任务1：尝试获取互斥锁2\n");
            if (xSemaphoreTake(xMutex2, portMAX_DELAY) == pdTRUE)
            {
                printf("任务1：成功获取互斥锁2\n");
                
                /* 使用两个互斥锁的临界区 */
                printf("任务1：进入临界区（持有两个互斥锁）\n");
                vTaskDelay(pdMS_TO_TICKS(500));
                printf("任务1：退出临界区\n");
                
                /* 释放互斥锁2 */
                xSemaphoreGive(xMutex2);
                printf("任务1：释放互斥锁2\n");
            }
            
            /* 释放互斥锁1 */
            xSemaphoreGive(xMutex1);
            printf("任务1：释放互斥锁1\n");
        }
        
        /* 短暂延迟后重新开始循环 */
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/*
 * 任务2：首先获取互斥锁2，然后尝试获取互斥锁1
 * 这将导致与任务1的死锁
 */
static void prvTask2(void *pvParameters)
{
    (void)pvParameters;
    
    /* 给任务1一点时间先运行 */
    vTaskDelay(pdMS_TO_TICKS(100));
    
    for (;;)
    {
        printf("任务2：尝试获取互斥锁2\n");
        if (xSemaphoreTake(xMutex2, portMAX_DELAY) == pdTRUE)
        {
            printf("任务2：成功获取互斥锁2\n");
            
            /* 故意延迟一段时间，增加发生死锁的几率 */
            vTaskDelay(pdMS_TO_TICKS(DEADLOCK_DELAY_MS));
            
            printf("任务2：尝试获取互斥锁1\n");
            if (xSemaphoreTake(xMutex1, portMAX_DELAY) == pdTRUE)
            {
                printf("任务2：成功获取互斥锁1\n");
                
                /* 使用两个互斥锁的临界区 */
                printf("任务2：进入临界区（持有两个互斥锁）\n");
                vTaskDelay(pdMS_TO_TICKS(500));
                printf("任务2：退出临界区\n");
                
                /* 释放互斥锁1 */
                xSemaphoreGive(xMutex1);
                printf("任务2：释放互斥锁1\n");
            }
            
            /* 释放互斥锁2 */
            xSemaphoreGive(xMutex2);
            printf("任务2：释放互斥锁2\n");
        }
        
        /* 短暂延迟后重新开始循环 */
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
} 