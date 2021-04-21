#include<stdlib.h>
#include<stdio.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<unistd.h>
#include<errno.h>
#include<string.h>
#include<stdbool.h>
#include<linux/limits.h>
#define BUF_SIZE 1024

// 仿写 cp 命令
// usage: ./my_cp src_file dest(file or dir)

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

static void new_path(char *path, char *file, char *new_path) {
    int path_len = strlen(path);
    int file_len = strlen(file);
    if (path_len + file_len >= PATH_MAX) return;
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

// 调用前保证 file 已经存在，让用户决定覆盖还是追加内容
// 成果返回新文件的 fd，失败返回 -1 或者
static int overwrite_or_append(char *file) {
    printf("file '%s' exists! type 'o' to overwrite or 'a' to append!\n", file);
    char cmd = getchar();
    switch (cmd)
    {
    case 'o':
        // 先删除原有文件
        if (remove(file) != 0) {
            printf("can not overwrite dest_file!\n");
            return -1;
        }
        // 创建新的同名文件
        return open(file, O_CREAT | O_EXCL | O_RDWR);
    case 'a' :
        // 追加模式
        return open(file, O_RDWR | O_APPEND);
    default:
        printf("unknown option. exit!\n");
        return -1;
    }
}

int main(int argc, char *argv[]) {
    // 判断参数是否正确
    if (argc != 3) {
        printf("usage error! usage: ./my_cp src_file dest(file or dir)\n");
        return 0;
    }
    // 判断 src_file 是否存在
    if (!file_exists(argv[1])) {
        printf("src_file does not exist!\n");
        return 0;
    }
    int in_fd = open(argv[1], O_RDONLY);
    if (in_fd < 0) {
        perror(argv[0]);
        return 0;
    }
    struct stat src_stat, dest_stat;
    if (fstat(in_fd, &src_stat) != 0) {
        perror(argv[0]);
        return 0;
    }
    // 判断 src_file 是否为目录
    if (S_ISDIR(src_stat.st_mode)) {
        printf("src_file must not be a dir\n");
        return 0;
    }
    int out_fd;
    // 查看 dest 是否存在，存在的话看是目录还是文件
    if (stat(argv[2], &dest_stat) != 0) {
        // dest 不存在，创建新文件
        if (errno == ENOENT) {
            out_fd = open(argv[2], O_CREAT | O_EXCL | O_RDWR);
            if (out_fd < 0) {
                perror(argv[0]);
                return 0;
            }
        } 
    } else {
        // dest 已经存在
        if (S_ISDIR(dest_stat.st_mode)) {
            // dest 为目录，构建新的文件，使用同名文件
            char new_name[PATH_MAX];
            memset(new_name, 0, PATH_MAX);
            new_path(argv[2], argv[1], new_name);
            // 查看新生成的文件名是否已经有文件存在，不存在则创建新的文件
            out_fd = file_exists(new_name) ? overwrite_or_append(new_name) : open(new_name, O_CREAT | O_EXCL | O_RDWR);
        } else {
            // dest 是已经存在的文件
            out_fd = overwrite_or_append(argv[2]);
        }
    }
    // 复制内容
    copy_file(in_fd, out_fd);
    // 设置新文件的模式（和 src_file 一致）
    if (fchmod(out_fd, src_stat.st_mode) < 0) {
        perror(argv[0]);
        return 0;
    }
    close(in_fd);
    close(out_fd);
    return 0;
}