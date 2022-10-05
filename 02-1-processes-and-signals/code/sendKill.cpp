#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>

int main (int argc, char *argv[] ) {
    pid_t pid = atoi(argv[1]);
    kill(pid, SIGINT);
    return EXIT_SUCCESS;
}