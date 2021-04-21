#ifndef PHILOSOPHER_H
#define PHILOSOPHER_H
#include<pthread.h>

// 哲学家状态定义
typedef enum p_stat {
    INIT, // 初始状态
    HUNGRY, // 饥饿
    EATING, // 就餐
    THINKING// 思考
} p_stat;

// 哲学家定义
typedef struct phil {
    pthread_t pt;
    p_stat stat;
    int idx;
} phil;

// 筷子定义
typedef struct chop {
    pthread_mutex_t mtx;
    int idx;
} chop;

#endif