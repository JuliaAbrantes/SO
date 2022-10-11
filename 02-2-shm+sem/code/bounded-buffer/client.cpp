/*
    CLIENT SIDE
*/

#include <stdio.h>
#include <stdlib.h>
#include "myApp.h"
using namespace std;

/* main process: it starts the simulation and launches the producer and consumer processes */
int main(int argc, char *argv[])
{
    ServiceRequest req = {"hello world"};
    ServiceResponse res;
    myApp::callService(req, res);
}