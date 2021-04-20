#include<stdlib.h>
#include<stdio.h>
#include<sys/stat.h>
#include<dirent.h>
#include<unistd.h>
#include<errno.h>
#include<linux/limits.h>
#include<sys/types.h>
#include<pwd.h>
#include<grp.h>
#include<time.h>
#include<stdbool.h>
#include<string.h>
#define MONTH 12

static char curr_path[PATH_MAX];
static char* month[MONTH];

static void init_month_arr() {
    month[0] = "Jan"; month[1] = "Feb"; month[2] = "Mar"; month[3] = "Apr"; month[4] = "May"; month[5] = "Jun";
    month[6] = "Jul"; month[7] = "Aug"; month[8] = "Sep"; month[9] = "Oct"; month[10] = "Nov"; month[11] = "Dec";
}

static char file_type(mode_t mode) {
    if (S_ISREG(mode)) return '-';
    else if (S_ISDIR(mode)) return 'd';
    else if (S_ISCHR(mode)) return 'c';
    else if (S_ISBLK(mode)) return 'b';
    else if (S_ISFIFO(mode)) return 'p';
    else if (S_ISSOCK(mode)) return 's';
    else return 'l';
}

static char *file_perm(mode_t mode) {
    static char perms[10];
    for (int i = 0; i < 9; ++i) perms[i] = '-';
    perms[9] = '\0';
    if (S_IRUSR & mode) perms[0] = 'r';
    if (S_IWUSR & mode) perms[1] = 'w';
    if (S_IXUSR & mode) perms[2] = 'x';
    if (S_IRGRP & mode) perms[3] = 'r';
    if (S_IWGRP & mode) perms[4] = 'w';
    if (S_IXGRP & mode) perms[5] = 'x';
    if (S_IROTH & mode) perms[6] = 'r';
    if (S_IWOTH & mode) perms[7] = 'w';
    if (S_IXOTH & mode) perms[8] = 'x';
    return perms;
}

static void print_file_info(struct stat *buf) {
    // 打印文件类型
    printf("%c", file_type(buf->st_mode));
    // 打印权限（所有者， 组用户，其它用户权限）
    printf("%s ", file_perm(buf->st_mode));
    // 打印出硬链接或者子目录个数
    printf("%ld ", buf->st_nlink);
    // 打印文件所有者名称
    struct passwd *pw = getpwuid(buf->st_uid);
    printf("%s ", pw ? pw->pw_name : "unknown_usr");
    // 打印文件所有者所在组名称
    struct group *gp = getgrgid(buf->st_gid);
    printf("%s ", gp ? gp->gr_name : "unknown_grp");
    // 打印文件大小
    printf("%ld ", buf->st_size);
    // 打印文件最后修改时间
    struct tm *time = localtime(&(buf->st_mtim));
    printf("%s %d %d:%d ", month[time->tm_mon], time->tm_mday, time->tm_hour, time->tm_min);
}

// 判断 file 是否为 . 或者 .. 目录
static bool curr_or_parent(const char *file) {
    int len = strlen(file);
    return (len == 1 && file[0] == '.') || (len == 2 && file[0] == '.' && file[1] == '.');
}

int main(int argc, char *argv[]) {
    bool show_all = false; // 默认展示所有文件（包括隐藏文件）的属性
    bool show_curr_parent = false; // 默认展示当前文件夹和父文件夹的属性
    int (*stat_ptr)(const char*, struct stat*) = stat; // 默认展示软链接指向文件的属性
    init_month_arr(); // 初始化月表
    // 接受命令行参数
    // -a : 展示所有文件
    // -p : 展示当前文件夹和父文件夹的属性
    // -l : 展示软链接本身的属性
    int opt;
    while ((opt = getopt(argc, argv, ":apl")) != -1)
    {
        switch (opt)
        {
        case 'a':
            show_all = true;
            break;
        case 'p':
            show_curr_parent = true;
            break;
        case 'l':
            stat_ptr = lstat;
            break;
        case '?': 
            printf("unknown option: %c\n", optopt);
            return 0;
        default:
            break;
        }
    }
    
    // 打开当前目录
    DIR *dp;
    struct dirent *dirp;
    if (getcwd(curr_path, PATH_MAX) == NULL) {
        perror(argv[0]);
        return 0;
    }
    if ((dp = opendir(curr_path)) == NULL) {
        perror(argv[0]);
        return 0;
    }
    // 循环遍历当前目录的目录项
    struct stat item;
    while ((dirp = readdir(dp)) != NULL) {
        if (dirp->d_name[0] == '.') {
            bool flag = curr_or_parent(dirp->d_name);
            // 不对隐藏文件进行显示，跳过
            if (!show_all && !flag) continue;
            // 不对 . 和 .. 进行展示
            if (!show_curr_parent && flag) continue;
        } 
        if (stat_ptr(dirp->d_name, &item) < 0) {
            perror(argv[0]);
            return 0;
        }
        // 打印出文件的属性
        print_file_info(&item);
        printf("%s\n", dirp->d_name);
    }
    return 0;
}