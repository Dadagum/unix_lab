#ifndef CP_THREAD_H
#define CP_THREAD_H

// 传递给 cp 线程的参数结构
typedef struct cp_arg {
    char *src_file;
    char *dest;
} cp_arg;

// 拷贝文件例程
void *cp_routine(void *arg);

#endif