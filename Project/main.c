/*
 * FreeRTOS简化示例 - 死锁演示
 */

/* 标准库包含 */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <signal.h>

/* FreeRTOS相关头文件 */
#include "FreeRTOS.h"
#include "task.h"

/* 死锁演示程序头文件 */
#include "deadlock_demo.h"
/* 我们自己的死锁检测模块 */
#include "deadlock_detection.h"

/* 声明我们的死锁演示入口函数 */
extern void vStartDeadlockDemo(void);

/* 退出标志 */
static bool bExiting = false;

/* 信号处理函数 */
static void vExitSignal(int iSig)
{
    if (iSig == SIGINT)
        bExiting = true;
}

/* 简单检查任务 */
static void prvCheckTask(void *pvParameters)
{
    TickType_t xNextWakeTime;
    const TickType_t xCycleFrequency = pdMS_TO_TICKS(2500UL);

    /* 防止编译器警告 */
    (void)pvParameters;

    /* 初始化下一次唤醒时间 */
    xNextWakeTime = xTaskGetTickCount();

    for(;;)
    {
        /* 延迟到下一个周期 */
        vTaskDelayUntil(&xNextWakeTime, xCycleFrequency);

        /* 检查是否需要退出 */
        if (bExiting)
            vTaskEndScheduler();
        
        /* 打印运行状态信息 */
        printf("系统运行中 - 时间节拍: %u\n", (unsigned int)xTaskGetTickCount());
        fflush(stdout);
    }
}

/* 主函数 */
int main(void)
{
    TaskHandle_t xTask;
    
    printf("FreeRTOS 死锁演示程序启动\n");
    printf("按Ctrl+C可以退出程序\n");
    
    /* 创建一个简单的检查任务 */
    xTaskCreate(prvCheckTask, "Check", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, &xTask);
    
    /* 启动死锁演示程序 */
    vStartDeadlockDemo();
    
    /* 注册信号处理函数，以便可以正常退出 */
    signal(SIGINT, vExitSignal);
    
    /* 启动调度器 */
    vTaskStartScheduler();

    /* 如果执行到这里，说明内存不足 */
    return 0;
}

/* 
 * FreeRTOS回调函数
 */
void vApplicationMallocFailedHook(void)
{
    printf("内存分配失败!\n");
    exit(-1);
}

void vApplicationIdleHook(void)
{
    /* 空实现 */
}

void vApplicationTickHook(void)
{
    /* 空实现 */
}

void vAssertCalled(unsigned long ulLine, const char * const pcFileName)
{
    taskENTER_CRITICAL();
    {
        printf("[断言失败] %s:%lu\n", pcFileName, ulLine);
        fflush(stdout);
    }
    taskEXIT_CRITICAL();
    exit(-1);
}
