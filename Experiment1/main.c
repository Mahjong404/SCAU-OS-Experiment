//
// Created by Surtr Muelsyse on 2026/4/18.
//
#include <stdio.h>
#include <pthread.h>
#include "env_monitor.h"

/* 全局变量定义 */
EnvData sharedData = {0, 0, 0, 0};
Threshold sharedThreshold = {35, 80}; // 规范要求的阈值
pthread_mutex_t dataMux = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t dataCond = PTHREAD_COND_INITIALIZER;
pthread_cond_t alarmCond = PTHREAD_COND_INITIALIZER;

int newDataReady = 0;
int alarmReady = 0;
bool systemRunning = true;

//在terminal中使用命令 gcc main.c threads.c -o EnvMonitor -lpthread 编译生成可执行文件 EnvMonitor.exe
// 运行 ./EnvMonitor.exe 来执行程序

int main() {
    pthread_t tid_collect, tid_display, tid_upload, tid_alarm;

    // 1. 创建并启动四个线程
    printf("[0] CREATE CollectThread\n");
    pthread_create(&tid_collect, NULL, CollectThreadFunc, NULL);

    printf("[0] CREATE DisplayThread\n");
    pthread_create(&tid_display, NULL, DisplayThreadFunc, NULL);

    printf("[0] CREATE UploadThread\n");
    pthread_create(&tid_upload, NULL, UploadThreadFunc, NULL);

    printf("[0] CREATE AlarmThread\n");
    pthread_create(&tid_alarm, NULL, AlarmThreadFunc, NULL);

    // 2. 阻塞主线程，等待所有子线程处理完毕退出
    pthread_join(tid_collect, NULL);
    pthread_join(tid_display, NULL);
    pthread_join(tid_upload, NULL);
    pthread_join(tid_alarm, NULL);

    // 3. 销毁互斥锁与条件变量，释放资源
    pthread_mutex_destroy(&dataMux);
    pthread_cond_destroy(&dataCond);
    pthread_cond_destroy(&alarmCond);

    printf("\nOnce the program has finished running, press Enter to exit...\n");
    getchar(); // 等待用户输入一个字符（按回车）后才继续执行

    return 0;
}