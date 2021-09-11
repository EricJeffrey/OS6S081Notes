#include "kernel/types.h"
#include "user/user.h"

void write2fd(int fd, char what) {
    int ret = write(fd, &what, 1);
    if (ret == -1) {
        printf("call to write failed.\n");
        exit(1);
    }
}
char readfromfd(int fd) {
    char res;
    int ret = read(fd, &res, 1);
    if (ret == -1) {
        printf("call to read failed.\n");
        exit(1);
    }
    return res;
}


int main(int argc, char *argv[]) {
    int rp[2];
    int wp[2];
    if (pipe(rp) == -1 || pipe(wp) == -1) {
        printf("call to pipe failed.\n");
        exit(1);
    }
    int child_pid = fork();
    if (child_pid == -1) {
        printf("call to fork failed.\n");
        exit(1);
    } else if (child_pid != 0) {
        close(rp[1]);
        close(wp[0]);
        int my_id = getpid();
        write2fd(wp[1], '!');
        readfromfd(rp[0]);
        printf("%d: received pong\n", my_id);
        wait(0);
    } else {
        close(rp[0]);
        close(wp[1]);
        int my_id = getpid();
        char what = readfromfd(wp[0]);
        printf("%d: received ping\n", my_id);
        write2fd(rp[1], what);
    }
    exit(0);
}