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
