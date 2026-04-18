#ifndef ENV_MONITOR_H
#define ENV_MONITOR_H

#include <pthread.h>
#include <stdbool.h>

/* 环境数据结构：存储单次采集的完整快照 */
typedef struct {
    int time;           // 采样时刻 (s)
    int temperature;    // 温度
    int humidity;       // 湿度
    int light;          // 光照
} EnvData;

/* 阈值配置结构：存储告警触发条件 */
typedef struct {
    int tempMax;        // 温度告警上限 (预设为 35)
    int humMax;         // 湿度告警上限 (预设为 80)
} Threshold;

/* 全局共享变量 (临界资源) */
extern EnvData sharedData;
extern Threshold sharedThreshold;

/* 并发控制对象 (互斥锁与条件变量) */
extern pthread_mutex_t dataMux;
extern pthread_cond_t dataCond;
extern pthread_cond_t alarmCond;

/* 状态标志位 */
extern int newDataReady;
extern int alarmReady;
extern bool systemRunning;

/* 线程函数声明 */
void* CollectThreadFunc(void* arg);
void* DisplayThreadFunc(void* arg);
void* UploadThreadFunc(void* arg);
void* AlarmThreadFunc(void* arg);

#endif // ENV_MONITOR_H