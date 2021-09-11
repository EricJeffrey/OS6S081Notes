#include "kernel/types.h"
#include "user/user.h"

int
main(int argc, char *argv[]){
    if (argc != 2) {
        char err_msg[] = "sleep require exactly 1 argument.\n";
        write(1, err_msg, strlen(err_msg));
        exit(1);
    }
    else {
        int t = atoi(argv[1]);

        sleep(t);
    }
    exit(0);
}