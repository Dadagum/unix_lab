#include<unistd.h>
#include<stdlib.h>
#include<stdio.h>
#include<stdbool.h>
#include<errno.h>
#include"philosopher.h"

// 模拟哲学家就餐问题
// 死锁避免：奇数线程先拿 left，偶数线程先拿 right
// cnt : 哲学家数量; limit : 哲学家吃 limit 次就离开
// usage: ./phil [cnt] [limit] [sleep_time]

static int cnt = 5; 
static int limit = 5; 
static int sleep_time = 1;
static phil *p_list; 
static chop *c_list;

static inline int left_chop(int idx) {
    return (idx+1) % cnt;
}

static inline int right_chop(int idx) {
    return idx;
}

void *phil_routine(void *arg) {
    int times = 0; // 就餐次数
    int idx = *((int*) arg); 
    bool is_odd = idx % 2;
    int ret = 0;
    while (true) {
        switch (p_list[idx].stat)
        {
        case INIT:
            // 初始状态，进入思考状态
            printf("phil[%d] is ready!\n", idx);
            p_list[idx].stat = THINKING;
            break;
        case THINKING:
            printf("phil[%d] is thinking!\n", idx);
            sleep(sleep_time);
            p_list[idx].stat = HUNGRY; // 进入饥饿状态
            break;
        case HUNGRY:
            printf("phil[%d] is hungry!\n", idx);
            // 奇数线程先拿 left，偶数线程先拿 right
            if (is_odd) {
                ret = pthread_mutex_lock(&(c_list[left_chop(idx)].mtx));
                if (ret != 0) {
                    printf("%s:%d - lock error!\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                ret = pthread_mutex_lock(&(c_list[right_chop(idx)].mtx));
                if (ret != 0) {
                    printf("%s:%d - lock error!\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                } 
            } else {
                ret = pthread_mutex_lock(&(c_list[right_chop(idx)].mtx));
                if (ret != 0) {
                    printf("%s:%d - lock error!\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);         
                }
                ret = pthread_mutex_lock(&(c_list[left_chop(idx)].mtx));
                if (ret != 0) {
                    printf("%s:%d - lock error!\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE); 
                } 
            }
            // 执行到这里说明已经成功拿起筷子了，准备就餐
            p_list[idx].stat = EATING;
            break;
        case EATING:
            printf("phil[%d] is eating!\n", idx);
            sleep(sleep_time);
            // 就餐完毕，释放筷子，这里省略返回值
            pthread_mutex_unlock(&(c_list[right_chop(idx)].mtx));
            pthread_mutex_unlock(&(c_list[left_chop(idx)].mtx));
            printf("phil[%d] finish eating!\n", idx);
            ++times;
            if (times == limit) { // 就餐次数足够，线程结束
                printf("phil[%d] is leaving!\n", idx);
                pthread_exit(EXIT_SUCCESS);
            }
            p_list[idx].stat = THINKING; // 重新进入思考状态
            break;
        default:
            printf("%s:%d - phil[%d] should not be here!\n",  __FILE__, __LINE__, idx);
            exit(EXIT_FAILURE);
            break;
        }
    }
}


int main(int argc, char *argv[]) {
    // 参数检验与获取
    if (argc > 4) {
        printf("usage error! usage: ./phil [cnt].\n");
        exit(EXIT_FAILURE);
    }
    if (argc > 1) {
        cnt = atoi(argv[1]);
        if (cnt <= 1) {
            printf("cnt should be greater than 1.\n");
            exit(EXIT_FAILURE);
        }
    }
    if (argc > 2) {
        limit = atoi(argv[2]);
        if (limit < 1) {
            printf("limit should be greater than 0.\n");
            exit(EXIT_FAILURE);
        }
    }
    if (argc > 3) {
        sleep_time = atoi(argv[3]);
        if (sleep_time < 1) {
            printf("limit should be greater than 0.\n");
            exit(EXIT_FAILURE);
        }
    }

    // 初始化哲学家与筷子
    p_list = (phil*) malloc(sizeof(phil) * cnt);
    c_list = (chop*) malloc(sizeof(chop) * cnt);
    for (int i = 0; i < cnt; ++i) {
        p_list[i].stat = INIT;
        p_list[i].idx = i;
        c_list[i].idx = i;
        pthread_mutex_init(&(c_list[i].mtx), NULL);
    }

    // 下面为了简单都省略错误处理了
    // 创建哲学家线程
    for (int i = 0; i < cnt; ++i) {
        pthread_create(&(p_list[i].pt), NULL, phil_routine, &(p_list[i].idx));
    }

    // 主线程等待子线程执行完毕
    for (int i = 0; i < cnt; ++i) {
        pthread_join(p_list[i].pt, NULL);
    }
    // 释放内存
    for (int i = 0; i < cnt; ++i) {
        pthread_mutex_destroy(&(c_list[i].mtx));
    }
    free(p_list);
    free(c_list);
    printf("main thread is leaving!\n");
    exit(EXIT_SUCCESS);
}