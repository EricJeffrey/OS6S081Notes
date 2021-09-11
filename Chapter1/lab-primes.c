#include "kernel/types.h"
#include "user/user.h"

void pexit(const char *msg) {
    printf("%s\n", msg);
    exit(1);
}
int forkwrapper() {
    int ret = fork();
    if (ret == -1)
        pexit("call to fork failed.");
    return ret;
}
void pipewrapper(int *p) {
    if (pipe(p) == -1)
        pexit("call to pipe failed.");
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

void child_work(int p[2]) {
    close(p[1]);
    int q;
    int ret = readwrapper(p[0], &q, 4);
    // printf("child: %d got number: %d\n", getpid(), q);
    if (ret == 0)
        exit(0);
    printf("prime %d\n", q);
    int pp[2];
    int child_pid = -1;
    while (1) {
        int num;
        ret = readwrapper(p[0], &num, 4);
        // printf("child: %d got number: %d\n", getpid(), num);
        if (ret == 0)
            break;
        if (num % q != 0) {
            // create child only once
            if (child_pid == -1) {
                pipewrapper(pp);
                child_pid = forkwrapper();
                if (child_pid == 0) {
                    child_work(pp);
                    // won't be executed here because child_work call exit directly
                } else {
                    close(pp[0]);
                }
            }
            writewrapper(pp[1], &num, 4);
        }
    }
    // close write so that read will got EOF(i.e. 0)
    close(pp[1]);
    if (child_pid != -1)
        waitwrapper(0);
    exit(0);
}

int main(int argc, char *argv[]) {
    // close unused default I/O fd
    close(0);
    close(2);
    int p[2];
    pipewrapper(p);
    int child_pid = forkwrapper();
    if (child_pid == 0) {
        child_work(p);
    } else {
        close(p[0]);
        for (int i = 2; i <= 35; i++)
            writewrapper(p[1], &i, 4);
        close(p[1]);
        waitwrapper(0);
    }
    exit(0);
}
