//
// Created by Surtr Muelsyse on 2026/4/18.
//
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "env_monitor.h"

// 采集线程 (生产者)
void* CollectThreadFunc(void* arg) {
    FILE *file = fopen("test.txt", "r");
    if (!file) {
        perror("无法打开 test.txt");
        systemRunning = false;
        return NULL;
    }

    int time, temp, hum, light;

    // 循环读取每一行测试数据
    while (fscanf(file, "%d %d %d %d", &time, &temp, &hum, &light) == 4) {
        // 打印申请与开始日志
        printf("[%d] Collect APPLY temp=%d hum=%d light=%d\n", time, temp, hum, light);
        printf("[%d] Collect START\n", time);

        // --- 申请互斥锁，进入临界区 ---
        pthread_mutex_lock(&dataMux);

        sharedData.time = time;
        sharedData.temperature = temp;
        sharedData.humidity = hum;
        sharedData.light = light;

        newDataReady = 1; // 标记新数据已就绪

        // 判断是否触发告警
        if (temp > sharedThreshold.tempMax || hum > sharedThreshold.humMax) {
            alarmReady = 1;
        } else {
            alarmReady = 0;
        }

        // --- 释放互斥锁，退出临界区 ---
        pthread_mutex_unlock(&dataMux);

        // --- 发送同步唤醒事件 ---
        // 唤醒显示和上传线程
        pthread_cond_broadcast(&dataCond);

        // 若有超限，唤醒告警线程
        if (alarmReady == 1) {
            pthread_cond_signal(&alarmCond);
        }

        printf("[%d] Collect END\n", time);

        // 模拟等待，确保本轮数据被消费者处理完毕（对应需求中：一次完整处理流程）
        usleep(100000);
    }

    fclose(file);

    // 数据读取完毕，触发系统退出流程
    pthread_mutex_lock(&dataMux);
    systemRunning = false;
    pthread_mutex_unlock(&dataMux);

    // 唤醒所有可能在阻塞中的线程，使其能够安全退出
    pthread_cond_broadcast(&dataCond);
    pthread_cond_broadcast(&alarmCond);

    return NULL;
}

// 显示线程 (消费者)
void* DisplayThreadFunc(void* arg) {
    int lastProcessedTime = -1; // 记录上次处理的时刻，避免重复处理
    EnvData localData;

    while (1) {
        pthread_mutex_lock(&dataMux);

        // 若系统运行中，且没有新数据（或者新数据已经处理过），则挂起等待
        while (systemRunning && (newDataReady == 0 || sharedData.time == lastProcessedTime)) {
            pthread_cond_wait(&dataCond, &dataMux);
        }

        // 检查系统是否已退出
        if (!systemRunning && sharedData.time == lastProcessedTime) {
            pthread_mutex_unlock(&dataMux);
            break;
        }

        printf("[%d] Display APPLY\n", sharedData.time);

        // 拷贝共享数据到本地，并更新最后处理记录
        localData = sharedData;
        lastProcessedTime = localData.time;

        pthread_mutex_unlock(&dataMux);

        // 脱离锁执行业务逻辑，提高并发度
        printf("[%d] Display START temp=%d hum=%d light=%d\n", localData.time, localData.temperature, localData.humidity, localData.light);
        printf("[%d] Display END\n", localData.time);
    }
    return NULL;
}

// 上传线程 (消费者)
void* UploadThreadFunc(void* arg) {
    int lastProcessedTime = -1;
    EnvData localData;

    while (1) {
        pthread_mutex_lock(&dataMux);

        while (systemRunning && (newDataReady == 0 || sharedData.time == lastProcessedTime)) {
            pthread_cond_wait(&dataCond, &dataMux);
        }

        if (!systemRunning && sharedData.time == lastProcessedTime) {
            pthread_mutex_unlock(&dataMux);
            break;
        }

        printf("[%d] Upload APPLY\n", sharedData.time);

        localData = sharedData;
        lastProcessedTime = localData.time;

        pthread_mutex_unlock(&dataMux);

        printf("[%d] Upload START temp=%d hum=%d light=%d\n", localData.time, localData.temperature, localData.humidity, localData.light);
        printf("[%d] Upload END\n", localData.time);
    }
    return NULL;
}

// 告警线程 (条件消费者)
void* AlarmThreadFunc(void* arg) {
    EnvData localData;

    while (1) {
        pthread_mutex_lock(&dataMux);

        // 仅当 alarmReady == 1 时才被唤醒继续
        while (systemRunning && alarmReady == 0) {
            pthread_cond_wait(&alarmCond, &dataMux);
        }

        if (!systemRunning && alarmReady == 0) {
            pthread_mutex_unlock(&dataMux);
            break;
        }

        localData = sharedData;
        alarmReady = 0; // 受理告警后重置标志位

        pthread_mutex_unlock(&dataMux);

        // 判定告警类型
        const char* type = "";
        if (localData.temperature > sharedThreshold.tempMax && localData.humidity > sharedThreshold.humMax) {
            type = "TEMP_HUM";
        } else if (localData.temperature > sharedThreshold.tempMax) {
            type = "TEMP";
        } else if (localData.humidity > sharedThreshold.humMax) {
            type = "HUM";
        }

        if (type[0] != '\0') {
            printf("[%d] Alarm APPLY type=%s\n", localData.time, type);
            printf("[%d] Alarm START\n", localData.time);
            printf("[%d] Alarm END\n", localData.time);
        }
    }
    return NULL;
}