/*
    SERVER SIDE
*/

#include <stdio.h>
#include <stdlib.h>
#include "delays.h"
#include "myApp.h"
using namespace std;

/* main process: it starts the simulation and launches the producer and consumer processes */
int main(int argc, char *argv[])
{
    while(true) {
        myApp::processService()
        gaussianDelay(10, 5);
    }
    return EXIT_SUCCESS;
}