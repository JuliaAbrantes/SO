/*
    My application implementation
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <stdint.h>

#include "fifo.h"
#include "delays.h"
#include "process.h"


namespace memoryHandler {

    /* when using shared memory, the size of the data structure must be fixed */
    struct FIFO
    { 
        int semid;          ///< syncronization semaphore array
        uint32_t ii;        ///< point of insertion
        uint32_t ri;        ///< point of retrieval
        uint32_t cnt;       ///< number of items stored
        ITEM slot[FIFOSZ];  ///< storage memory
    };

    int fifoId = -1;
    FIFO *fifo = NULL;

    /* ************************************************* */

    /* index of access, full and empty semaphores */
    #define ACCESS 0
    #define NITEMS 1
    #define NSLOTS 2

    /* ************************************************* */

    static void down(int semid, unsigned short index)
    {
        struct sembuf op = {index, -1, 0};
        psemop(semid, &op, 1);
    }

    /* ************************************************* */

    static void up(int semid, unsigned short index)
    {
        struct sembuf op = {index, 1, 0};
        psemop(semid, &op, 1);
    }
    
    /* ************************************************* */
    
    /* create a FIFO in shared memory, initialize it, and return its id */
    void create(void)
    {
        /* create the shared memory */
        fifoId = pshmget(IPC_PRIVATE, sizeof(FIFO), 0600 | IPC_CREAT | IPC_EXCL);

        /*  attach shared memory to process addressing space */
        fifo = (FIFO*)pshmat(fifoId, NULL, 0);

        /* init fifo */
        uint32_t i;
        for (i = 0; i < FIFOSZ; i++)
        {
            fifo->slot[i].id = 99;
            fifo->slot[i].value = 99999;
        }
        fifo->ii = fifo->ri = 0;
        fifo->cnt = 0;

        /* create access, full and empty semaphores */
        fifo->semid = psemget(IPC_PRIVATE, 3, 0600 | IPC_CREAT | IPC_EXCL);

        /* init semaphores */
        for (i = 0; i < FIFOSZ; i++)
        {
            up(fifo->semid, NSLOTS);
        }
        up(fifo->semid, ACCESS);
    }

    /* ************************************************* */

    void destroy()
    {
        /* detach shared memory from process addressing space */
        pshmdt(fifo);

        /* destroy the shared memory */
        pshmctl(fifoId, IPC_RMID, NULL);
    }


    /* take a buffer out of fifo of free buffers */
    void getFreeBuffer () {

    }

    /* put request data on buffer */
    struct* void putRequestData (struct& data , int id) {

    }          

    /* add buffer to fifo of pending requests */         
    void addNewPendingRequest (int id) {

    }

    /* wait (blocked) until a response is available */
    void waitForResponse (int id) {

    }

    /* take response out of buffer */
    struct* getResponseData (int id) {

    }

    /* buffer is free, so add it to fifo of free buffers */
    void releaseBuffer (int id) {
        
    }
}