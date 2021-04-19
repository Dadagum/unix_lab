#include<stdlib.h>
#include<stdio.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<unistd.h>
#include<errno.h>
#include<string.h>
#define BUF_SIZE 1024

static void copy_file(int in_fd, int out_fd) {
    static char *buffer[BUF_SIZE];
    while (read(in_fd, buffer, BUF_SIZE) > 0) {
        write(out_fd, buffer, BUF_SIZE);
    }
}

static char* new_path(char *path, char *file) {
    static char new_path[FILENAME_MAX];
    int path_len = strlen(path);
    int file_len = strlen(file);
    if (path_len + file_len >= FILENAME_MAX) return NULL;
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
    } 
    // 构建 path/filename 的路径
    strncpy(new_path + path_len, tmp, file_len);
    return new_path;
}

// usage: ./my_cp src_file dest(file or dir)

int main(int argc, char *argv[]) {
    // 判断参数是否正确
    if (argc != 3) {
        printf("usage error! usage: ./my_cp src_file dest(file or dir)\n");
        return 0;
    }
    // 判断 src_file 是否存在
    if (access(argv[1], F_OK) != 0) {
        printf("src_file does not exist!\n");
        return 0;
    }
    // 判断 src_file 是否为目录
    struct stat statbuf;
    if (stat(argv[1], &statbuf) != 0) {
        perror(argv[0]);
        return 0;
    }
    if (S_ISDIR(statbuf.st_mode)) {
        printf("src_file must be regular file!\n");
        return 0;
    }
    int in_fd = open(argv[1], O_RDONLY);
    int out_fd;

    out_fd = open(argv[2], O_CREAT | O_EXCL | O_RDWR, S_IRUSR | S_IWUSR);
    if (out_fd < 0) {
        // dest_file 已经存在，可以选择覆盖或者追加
        if (errno == EEXIST) {
            printf("dest_file exists! type 'o' to overwrite or 'a' to append!\n");
            char cmd = getchar();
            switch (cmd)
            {
            case 'o':
                // 删除原有文件
                if (remove(argv[2]) != 0) {
                    printf("can not overwrite dest_file!\n");
                    return 0;
                }
                // 创建新的同名文件
                out_fd = open(argv[2], O_CREAT | O_EXCL | O_RDWR, S_IRUSR | S_IWUSR);
                break;
            case 'a' :
                // 追加模式
                out_fd = open(argv[2], O_RDWR | O_APPEND);
                if (out_fd < 0) {
                    perror(argv[0]);
                    return 0;
                }
                break;
            default:
                printf("unknown option. exit!\n");
                return 0;
            }

        } else if (errno == EISDIR) {
            // dest 为目录，构建新的文件，使用同名文件
            out_fd = open(new_path(argv[2], argv[1]), O_CREAT | O_EXCL | O_RDWR, S_IRUSR | S_IWUSR);
        } else {
            perror(argv[0]);
            return 0;
        }
    }

    // 复制内容
    copy_file(in_fd, out_fd);

    close(in_fd);
    close(out_fd);
    return 0;
}