#include<stdlib.h>
#include<stdio.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<unistd.h>
#include<errno.h>
#include<string.h>
#include<stdbool.h>
#include<linux/limits.h>
#include<pthread.h>
#include"cp_thread.h"
#define BUF_SIZE 1024

// 协助与 main 线程同步
extern int alives;
extern pthread_cond_t can_exit;
extern pthread_mutex_t mutex;

static bool file_exists(char *filename) {
  struct stat buffer;   
  return (stat(filename, &buffer) == 0);
}

static void copy_file(int in_fd, int out_fd) {
    char *buffer[BUF_SIZE];
    int num_read;
    while ((num_read = read(in_fd, buffer, BUF_SIZE)) > 0) {
        write(out_fd, buffer, num_read);
    }
}

// 和多进程版本不同，这个函数需要传入 new_path 参数，返回的地址直接在 new_path 中
static void new_path(char *path, char *file, char *new_path) {
    int path_len = strlen(path);
    int file_len = strlen(file);
    if (path_len + file_len >= FILENAME_MAX) return;
    memcpy(new_path, path, path_len);
    // 检查 path 结尾是否已经有 / 符号
    if (new_path[path_len-1] != '/') {
        new_path[path_len] = '/';
        path_len++;
    }
    // 得到文件的名字 filename
    char *tmp = strrchr(file, '/');
    if (tmp == NULL) {
        tmp = file;
    } else tmp++;
    // 构建 path/filename 的路径
    strncpy(new_path + path_len, tmp, file_len);
}

// 成功返回新文件的 fd，失败返回 -1 或者
// 多线程版本默认覆盖
static int overwrite_or_append(char *file) {
    // 先删除原有文件
    if (remove(file) != 0) {
        printf("can not overwrite dest_file!\n");
        return -1;
    }
    // 创建新的同名文件
    return open(file, O_CREAT | O_EXCL | O_RDWR);
}

// cp 线程例程
void *cp_routine(void *arg) {
    pthread_detach(pthread_self()); // 采用分离线程的方式
    cp_arg* param = (cp_arg*) arg;
    char *src_file = param->src_file;
    char *dest = param->dest;
    if (!file_exists(src_file)) {
        printf("file '%s' does not exist!\n", src_file);
        return NULL;
    }
    int in_fd = open(src_file, O_RDONLY);
    if (in_fd < 0) {
        printf("cannot open '%s'\n", src_file);
        return NULL;
    }
    struct stat src_stat, dest_stat;
    if (fstat(in_fd, &src_stat) != 0) {
        printf("cannot access '%s'\n", src_file);
        return NULL;
    }
    // 判断 src_file 是否为目录
    if (S_ISDIR(src_stat.st_mode)) {
        printf("src_file must not be a dir\n");
        return NULL;
    }
    int out_fd;
    // 查看 dest 是否存在，存在的话看是目录还是文件
    if (stat(dest, &dest_stat) != 0) {
        // dest 不存在，创建新文件
        if (errno == ENOENT) {
            out_fd = open(dest, O_CREAT | O_EXCL | O_RDWR);
            if (out_fd < 0) {
                printf("cannot create new file '%s'\n", dest);
                return NULL;
            }
        } 
    } else {
        // dest 已经存在
        if (S_ISDIR(dest_stat.st_mode)) {
            char new_name[PATH_MAX];
            memset(new_name, 0, PATH_MAX);
            // dest 为目录，构建新的文件，使用同名文件
            new_path(dest, src_file, new_name);
            // 查看新生成的文件名是否已经有文件存在，不存在则创建新的文件
            out_fd = file_exists(new_name) ? overwrite_or_append(new_name) : open(new_name, O_CREAT | O_EXCL | O_RDWR);
        } else {
            // dest 是已经存在的文件
            out_fd = overwrite_or_append(dest);
        }
    }
    // 复制内容
    copy_file(in_fd, out_fd);
    // 设置新文件的模式（和 src_file 一致）
    if (fchmod(out_fd, src_stat.st_mode) < 0) {
        printf("cannot change permission: '%s'\n", dest);
        return NULL;
    }
    close(in_fd);
    close(out_fd);
    // 释放参数的堆内存
    free(param->dest);
    free(param->src_file);
    free(param);
    // 更新 cp alives 线程数量
    pthread_mutex_lock(&mutex);
    --alives;
    // 最后一个活着的 cp 线程，此时可以唤醒 main 线程了
    if (alives == 0) pthread_cond_signal(&can_exit);
    pthread_mutex_unlock(&mutex);

    return NULL;
}