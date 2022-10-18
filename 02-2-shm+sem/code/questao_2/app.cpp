/*
 *  @brief A simple FIFO, whose elements are pairs of integers,
 *      one being the id of the producer and the other the value produced
 *
 * @remarks safe, non busy waiting version
 *
 *  The following operations are defined:
 *     \li insertion of a value
 *     \li retrieval of a value.
 *
 * \author (2016-2022) Artur Pereira <artur at ua.pt>
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <stdint.h>

//#include "fifo.h"
#include "delays.h"
#include "process.h"

namespace app
{
    /** \brief internal storage size of <em>FIFO memory</em> */
    #define  FIFOSZ         5
    #define  MAX_STR_SIZE   20

    /*
     *  \brief Type of the shared data structure.
     */

    struct RESPONSE
    {
        uint32_t characters;
        uint32_t letters;
        uint32_t numbers;
    };    

    /* when using shared memory, the size of the data structure must be fixed */
    struct FIFO
    {
        int semid;          ///< syncronization semaphore array
        uint32_t ii;        ///< point of insertion
        uint32_t ri;        ///< point of retrieval
        uint32_t cnt;       ///< number of items stored
        uint32_t slot[FIFOSZ];  ///< storage memory
    };
    
    struct POOL{
        RESPONSE response;
        char request[MAX_STR_SIZE];
        bool signal;
    };

    struct SHMEM
    {
        POOL pool[FIFOSZ];
        FIFO fifo[2]; //fifo[0] : fifo free buffers | fifo[1] : fifo requests
    };

    int memId = -1;
    SHMEM *mem = NULL;

    /* ************************************************* */

    /* index of access, full and empty semaphores */
    #define ACCESS 0
    #define NITEMS 1 //se NITEMS == 0 então está vazio
    //#define NSLOTS 2 //se NSLOTS == 0 então está cheio //not necessary

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
        memId = pshmget(IPC_PRIVATE, sizeof(SHMEM), 0600 | IPC_CREAT | IPC_EXCL);

        /*  attach shared memory to process addressing space */
        mem = (SHMEM*)pshmat(memId, NULL, 0);


        /* CREATE FIFO BUFFER FREE */
        /* init fifo with all numbers from 0 to size of fifo*/
        uint32_t i;
        for (i = 0; i < FIFOSZ; i++)
        {
            mem->fifo[0].slot[i] = i;
        }
        mem->fifo[0].ii = FIFOSZ;   //insertion point //won't be
        mem->fifo[0].ri = 0;        //retreive point
        mem->fifo[0].cnt = FIFOSZ;  //fifo starts full

        /* create access and empty semaphores */
        mem->fifo[0].semid = psemget(IPC_PRIVATE, 2, 0600 | IPC_CREAT | IPC_EXCL);

        /* init semaphores */
        for (i = 0; i < FIFOSZ; i++) //fifo starts full
        {
            up(mem->fifo[0].semid, NITEMS);
        }
        up(mem->fifo[0].semid, ACCESS); //access alowed


        /* CREATE FIFO NEW REQUESTS */
        /* init fifo with all numbers 99*/
        for (i = 0; i < FIFOSZ; i++)
        {
            mem->fifo[1].slot[i] = 99;
        }
        mem->fifo[1].ii = 0;   //insertion point //won't be
        mem->fifo[1].ri = 0;   //retreive point
        mem->fifo[1].cnt = 0;  //fifo starts empty

        /* create access and empty semaphores */
        mem->fifo[1].semid = psemget(IPC_PRIVATE, 2, 0600 | IPC_CREAT | IPC_EXCL);

        /* init semaphores */
        for (i = 0; i < FIFOSZ; i++) //fifo starts empty
        {
            up(mem->fifo[1].semid, NITEMS);
        }
        up(mem->fifo[1].semid, ACCESS); //access alowed

        /*INITIALIZE POOL*/
        for (i = 0; i < FIFOSZ; i++) {
            //mem->pool[i].request = {'a','b','c'};
            mem->pool[i].response = {};
            mem->pool[i].signal = false; //response not avaible
        }

    }

    /* ************************************************* */

    void destroy()
    {
        /* detach shared memory from process addressing space */
        pshmdt(mem);

        /* destroy the shared memory */
        pshmctl(memId, IPC_RMID, NULL);
    }

    /* ************************************************* */

    /* put buffer id back */
    void fifoBuffersIn(uint32_t myBufferId) {
        FIFO fifo = mem->fifo[0];
        /* decrement emptiness, blocking if necessary, and lock access */
        down(fifo.semid, ACCESS);

        /* Insert free buffer id back */
        fifo.slot[fifo.ii] = myBufferId;
        gaussianDelay(0.1, 0.5);
        fifo.ii = (fifo.ii + 1) % FIFOSZ;
        fifo.cnt++;
        mem->pool[myBufferId].signal = false; //reset to response not avaiable

        /* unlock access and increment fullness */
        up(fifo.semid, ACCESS);
        up(fifo.semid, NITEMS);
    }

    /* get buffer id to connect */
    void fifoBuffersOut(uint32_t * myBufferId) {
        FIFO fifo = mem->fifo[0];
        /* decrement fullness, blocking if necessary, and lock access */
        down(fifo.semid, NITEMS);
        down(fifo.semid, ACCESS);

        /* Retrieve free buffer id */
        *myBufferId = fifo.slot[fifo.ri];
        fifo.slot[fifo.ri] = 99;
        fifo.ri = (fifo.ri + 1) % FIFOSZ;
        fifo.cnt--;
        //mem->pool[myBufferId].signal = false; //garantee it is not avaiable

        /* unlock access and increment fullness */
        up(fifo.semid, ACCESS);
    }

    /* create request to process*/
    void fifoRequestsIn(uint32_t myBufferId) {
        FIFO fifo = mem->fifo[1];
        /* decrement emptiness, blocking if necessary, and lock access */
        down(fifo.semid, ACCESS);

        /* Insert pair */
        fifo.slot[fifo.ii] = myBufferId;
        gaussianDelay(0.1, 0.5);
        fifo.ii = (fifo.ii + 1) % FIFOSZ;
        fifo.cnt++;

        /* unlock access and increment fullness */
        up(fifo.semid, ACCESS);
        up(fifo.semid, NITEMS);

    }

    /* get id to be processed */
    void fifoRequestsOut(uint32_t * myBufferId) {
        FIFO fifo = mem->fifo[1];
        /* decrement fullness, blocking if necessary, and lock access */
        down(fifo.semid, NITEMS);
        down(fifo.semid, ACCESS);

        /* Retrieve id from fifo of new requests */
        *myBufferId = fifo.slot[fifo.ri];
        fifo.slot[fifo.ri] = 99;
        fifo.ri = (fifo.ri + 1) % FIFOSZ;
        fifo.cnt--;

        /* unlock access and increment fullness */
        up(fifo.semid, ACCESS);

    }
    void getRequestPool(uint32_t myBufferId, char* value[]) {
        uint32_t n = 0;
        while(n < MAX_STR_SIZE){
            *value = &mem->pool[myBufferId].request[n++];
            value++;
        }
    }
    void putRequestPool(uint32_t myBufferId,char value[]) {
        uint32_t n = 0;
        while(n < MAX_STR_SIZE){
            mem->pool[myBufferId].request[n++] = *value++;
            value++;
        }
    }
    void getResponsePool(uint32_t myBufferId, RESPONSE* strStats) {
        strStats->characters = mem->pool[myBufferId].response.characters;
        strStats->letters = mem->pool[myBufferId].response.letters;
        strStats->numbers = mem->pool[myBufferId].response.numbers;
        /*
        RESPONSE* res =  &mem->pool[myBufferId].response;
        strStats = {.characters = res->characters, .letters = res->letters, .numbers = res->numbers };*/
    }
    void putResponsePool(uint32_t myBufferId, RESPONSE strStats) {
        mem->pool[myBufferId].response.characters = strStats.characters;
        mem->pool[myBufferId].response.letters = strStats.letters;
        mem->pool[myBufferId].response.numbers = strStats.numbers;
    }
    void signalResponseIsAvailable(uint32_t myBufferId) {
        mem->pool[myBufferId].signal = true; //response Avaible
    }

}
