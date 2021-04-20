#include<stdlib.h>
#include<stdio.h>
#include<sys/stat.h>
#include<dirent.h>
#include<unistd.h>
#include<errno.h>
#include<stdbool.h>
#include<string.h>
#include<linux/limits.h>
#include<pthread.h>
#include"cp_thread.h"

// 递归将 dir 目录下的所有文件 cp 到 cp_path 中，其中 cp 的过程由创建的新线程完成
// 主要有 3 个线程：main 线程创建遍历目录的线程（实验中称为 ls 线程）
// 然后由 ls 线程对每一个需要 cp 的文件又创建新的线程进行处理
// 尽管实验在扩展二没有要求 main 线程等待所有 cp 线程结束才离开，但是这个程序还是加上去了
// 从而有效避免由于 main 线程结束，而 cp 线程没有结束导致线程被迫结束的情况
// usage: ./cp_dir_thread [dir]
static char cp_path[] = "/tmp/cp_dir"; // 拷贝文件所在的地方
static char cp_elf_path[] = "/home/hongda/git/unix_lab/lab_2/my_cp"; // my_cp 程序所在位置

// 保证 main 结束前所有 cp 线程都完成的条件变量
pthread_cond_t can_exit = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
int alives = 0; // 还没有结束的 cp 线程数量

// 判断是否为 . 或者 .. 目录
static bool curr_or_parent(const char *file) {
    return !strcmp(file, ".") || !strcmp(file, "..");
}

// 遍历并拷贝文件
static void dir_traverse(char *dir_name) {
    // 构造新的相对路径
    char new_path[PATH_MAX];
    memset(new_path, 0, PATH_MAX);
    int base_len = strlen(dir_name);
    strncpy(new_path, dir_name, base_len);
    if (base_len >= PATH_MAX - 2) return;
    new_path[base_len] = '/';
    base_len++;

    DIR *dp;
    struct dirent *dirp;
    if ((dp = opendir(dir_name)) == NULL) {
        printf("can not opendir : '%s'\n", dir_name);
        _exit(EXIT_FAILURE);
    }
    // TODO: 以下部分都还没有改成多线程的内容，明天改
    while ((dirp = readdir(dp)) != NULL)
    {
        strncpy(new_path + base_len, dirp->d_name, strlen(dirp->d_name));
        // 目录文件: 递归进行文件 cp
        if (dirp->d_type == DT_DIR) {
            if (!curr_or_parent(dirp->d_name)) dir_traverse(new_path);
        } else {
            // 非目录文件：创建多线程进行 cp
            // TODO
        }
        // 父进程进行路径的回溯(重新置 0)
        memset(new_path + base_len, 0, strlen(dirp->d_name));
    }
}

// 遍历线程例程
static void *ls_routine(void *arg) {
    char *dir_name = (char*)arg;
    dir_traverse(dir_name);
    return NULL;
}

int main(int argc, char* argv[]) {
    // 这部分和 lab_4 的多进程版本相同
    if (argc > 2) {
        printf("usage error! usage: ./cp_ls [dir]\n");
        exit(EXIT_FAILURE);
    }
    struct stat st;
    // 目录不存在的话先创建
    if (stat(cp_path, &st) < 0) {
        mkdir(cp_path, 0777);
    }
    // 得到需要遍历的目录路径
    char dir_path[PATH_MAX];
    memset(dir_path, 0, PATH_MAX);
    if (argc == 1) {
        // 默认遍历当前的路径
        if (getcwd(dir_path, PATH_MAX) == NULL) {
            perror(argv[0]);
           exit(EXIT_FAILURE);
        }
    } else strncpy(dir_path, argv[1], strlen(argv[1]));

    // 创建遍历线程
    pthread_t ls_thr;
    if (pthread_create(&ls_thr, NULL, ls_routine, dir_path) != 0) {
        printf("fail to create ls thread!\n");
        exit(EXIT_FAILURE);
    }
    // 阻塞等待遍历线程结束
    if (pthread_join(ls_thr, NULL) != 0) {
        printf("fail to join ls thread!\n");
        exit(EXIT_FAILURE);
    }
    // 等待所有 cp 线程结束主线程才结束
    pthread_mutex_lock(&mutex);
    while (alives != 0) {
        pthread_cond_wait(&can_exit, &mutex);
    }
    exit(EXIT_SUCCESS);
}

