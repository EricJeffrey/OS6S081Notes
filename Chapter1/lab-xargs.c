#include "kernel/types.h"
#include "kernel/param.h"
#include "user/user.h"

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
int writewrapper(int fd, void *what, int n) { return _iowrapper(1, fd, what, n); }
void waitwrapper(int *status) {
    if (wait(status) == -1)
        pexit("call to wait failed.");
}
int openwrapper(char *path, int mode) {
    int ret = open(path, mode);
    if (ret == -1)
        pexit("call to open failed.");
    return ret;
}
int forkwrapper() {
    int ret = fork();
    if (ret == -1)
        pexit("call to fork failed.");
    return ret;
}

void forkandexec(char *cmd, char **argv) {
    int child_pid = forkwrapper();
    if (child_pid == 0) {
        exec(cmd, argv);
        exit(1);
    } else {
        wait(0);
    }
}

int main(int argc, char *argv[]) {
    const int bufsize = 512;
    char buf[bufsize];
    int index = 0;
    while (1) {
        if (index >= bufsize)
            pexit("line too long.");
        char ch;
        int ret = readwrapper(0, &ch, 1);
        if (ret == 0 || ch == '\n') {
            if (index > 0) {
                char *tmpargv[MAXARG];
                int tmpindex;
                for (tmpindex = 0; tmpindex < argc - 1; tmpindex++)
                    tmpargv[tmpindex] = argv[tmpindex + 1];
                buf[index] = 0;
                tmpargv[tmpindex++] = buf;
                tmpargv[tmpindex] = 0;
                forkandexec(tmpargv[0], tmpargv);
                index = 0;
            } else {
                // only car-return, do nothing
            }
            if (ret == 0)
                break;
        } else {
            buf[index++] = ch;
        }
    }
    exit(0);
}