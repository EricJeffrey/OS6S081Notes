#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

void pexit(const char *msg) {
    printf("%s\n", msg);
    exit(1);
}
int _iowrapper(int ioro, int fd, void *what, int n) {
    int ret = ioro == 0 ? read(fd, what, n) : write(fd, what, n);
    if (ret == -1)
        pexit("call to read/write failed.");
    return ret;
}
int readwrapper(int fd, void *what, int n) { return _iowrapper(0, fd, what, n); }
int openwrapper(char *path, int mode) {
    int ret = open(path, mode);
    if (ret == -1)
        pexit("call to open failed.");
    return ret;
}
int statwrapper(char *path, struct stat *statbuf) {
    int ret = stat(path, statbuf);
    if (ret == -1)
        pexit("call to stat failed.");
    return ret;
}

void walkdirpath(char *path, char *name, char *target) {
    // printf("%s\n", path);
    struct stat st;
    statwrapper(path, &st);
    if (st.type == T_DIR) {
        char buf[512];
        memset(buf, 0, sizeof(buf));
        strcpy(buf, path);
        char *p = buf + strlen(buf);
        if (*(p - 1) != '/') {
            *p = '/';
            p += 1;
        }
        int fd = openwrapper(path, 0);
        struct dirent de;
        while (readwrapper(fd, &de, sizeof(de)) == sizeof(de)) {
            if (de.inum == 0)
                continue;
            memmove(p, de.name, DIRSIZ);
            p[DIRSIZ] = 0;
            if (strcmp(de.name, ".") != 0 && strcmp(de.name, "..") != 0)
                walkdirpath(buf, de.name, target);
        }
    } else {
        if (name != 0 && strcmp(name, target) == 0)
            printf("%s\n", path);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 3)
        pexit("find take exactly 2 arguments");
    struct stat st;
    statwrapper(argv[1], &st);
    if (st.type != T_DIR)
        pexit("first argument of find should be directory");
    walkdirpath(argv[1], 0, argv[2]);
    exit(0);
    return 0;
}
