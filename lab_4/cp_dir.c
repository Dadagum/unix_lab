#include<stdlib.h>
#include<stdio.h>
#include<sys/stat.h>
#include<dirent.h>
#include<unistd.h>
#include<errno.h>
#include<stdbool.h>
#include<string.h>
#include<linux/limits.h>
#include<sys/wait.h>

// 递归将 dir 目录下的所有文件 cp（调用 my_cp 程序） 到 cp_path 中
// usage: ./cp_dir [dir]
// 实验要求，以下写代码中了，难道传参不香吗？
static char cp_path[] = "/tmp/cp_dir"; // 拷贝文件所在的地方
static char cp_elf_path[] = "/home/hongda/git/unix_lab/lab_2/my_cp"; // my_cp 程序所在位置

// 判断是否为 . 或者 .. 目录
static bool curr_or_parent(const char *file) {
    return !strcmp(file, ".") || !strcmp(file, "..");
}

// 递归遍历目录并复制文件
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
    while ((dirp = readdir(dp)) != NULL)
    {
        strncpy(new_path + base_len, dirp->d_name, strlen(dirp->d_name));
        // 目录文件: 递归进行文件 cp
        if (dirp->d_type == DT_DIR) {
            if (!curr_or_parent(dirp->d_name)) dir_traverse(new_path);
        } else {
            // 非目录文件：调用 my_cp 程序进行文件拷贝
            switch (fork())
            {
            case 0:
                // 执行 my_cp 程序
                execl(cp_elf_path, cp_elf_path, new_path, cp_path, (char*) NULL);
                _exit(EXIT_SUCCESS);
                break;
            default:
                break;
            }
        }
        // 父进程进行路径的回溯(重新置 0)
        memset(new_path + base_len, 0, strlen(dirp->d_name));
    }
}

int main(int argc, char* argv[]) {
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
    // 进行目录文件遍历
    dir_traverse(dir_path);
    // 释放 child
    while (1) {
        pid_t child = wait(NULL);
        if (child == -1) {
            if (errno == ECHILD) exit(EXIT_SUCCESS);
            else {
                printf("unknown error!\n");
                exit(EXIT_FAILURE);
            }
        } 
    }
}