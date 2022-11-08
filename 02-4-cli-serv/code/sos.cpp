/*
 *  \brief SoS: Statistics on Strings, a simple client-server application
 *    that computes some statistics on strings
 *
 * \author (2022) Artur Pereira <artur at ua.pt>
 * \author (2022) Miguel Oliveira e Silva <mos at ua.pt>
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>
#include <stdint.h>

#include <new>

#include "sos.h"

#include "dbc.h"
//#define __DEBUG__
/*
 * TODO point
 * Uncomment the #include that applies
 */
#include "process.h"
//#include "thread.h"

namespace sos
{
    /** \brief Number of transaction buffers */
    #define  NBUFFERS         5

    /** \brief indexes for the fifos of free buffers and pending requests */
    enum { FREE_BUFFER=0, PENDING_REQUEST=1};

    /** \brief interaction buffer data type */
    struct BUFFER 
    {
        char req[MAX_STRING_LEN+1];
        Response resp;
    };

    /** \brief the fifo data type to store indexes of buffers */
    struct FIFO
    {
        uint32_t ii;               ///< point of insertion
        uint32_t ri;               ///< point of retrieval
        uint32_t cnt;              ///< number of items stored
        uint32_t tokens[NBUFFERS]; ///< storage memory
    };

    /** \brief the data type representing all the shared area.
     *    Fifo 0 is used to manage tokens of free buffers.
     *    Fifo 1 is used to manage tokens of pending requests.
     */
    struct SharedArea
    {
        /* A fix number of transaction buffers */
        BUFFER pool[NBUFFERS];

        /* A fifo for tokens of free buffers and another for tokens with pending requests */
        FIFO fifo[2];

        /*
         * TODO point
         * Declare here all you need to accomplish the synchronization,
         * semaphores (for implementation using processes) or
         * mutexes, conditions and condition variables (for implementation using threads)
         */
        int semAccessId; //semaphore to see if can access
        int semEmptyId; //semaphore to see if empty
        int ResponseAvaiable; //semaphore array to see if response avaible
    };

    /** \brief pointer to shared area dynamically allocated */
    SharedArea *sharedArea = NULL;
    int sharedAreaId = -1;


    /* -------------------------------------------------------------------- */

    /* Allocate and init the internal supporting data structure,
     *   including all necessary synchronization resources
     */
    void open(void)
    {
#ifdef __DEBUG__
        fprintf(stderr, "%s()\n", __FUNCTION__);
#endif

        require(sharedArea == NULL, "Shared area must not exist");

        /* 
         * TODO point
         * Allocate the shared memory
         */
        /* create the shared memory */
        sharedAreaId = pshmget(IPC_PRIVATE, sizeof(SharedArea), 0600 | IPC_CREAT | IPC_EXCL);
        /*  attach shared memory to process addressing space */
        sharedArea = (SharedArea*)pshmat(sharedAreaId, NULL, 0);
        /* init fifo 0 (free buffers) */
        //FIFO *fifo = &sharedArea->fifo[FREE_BUFFER];
        for (uint32_t i = 0; i < NBUFFERS; i++)
        {
            sharedArea->fifo[FREE_BUFFER].tokens[i] = i;
        }
        sharedArea->fifo[FREE_BUFFER].ii = sharedArea->fifo[FREE_BUFFER].ri = NBUFFERS-1;
        sharedArea->fifo[FREE_BUFFER].cnt = NBUFFERS;

        /* init fifo 1 (pending requests) */
        //fifo = &sharedArea->fifo[PENDING_REQUEST];
        for (uint32_t i = 0; i < NBUFFERS; i++)
        {
            sharedArea->fifo[PENDING_REQUEST].tokens[i] = 0; // used to check for errors
        }
        sharedArea->fifo[PENDING_REQUEST].ii = sharedArea->fifo[PENDING_REQUEST].ri = 0;
        sharedArea->fifo[PENDING_REQUEST].cnt = 0;

        /* 
         * TODO point
         * Init synchronization elements
         */
        //TODO create 
        sharedArea->semAccessId = psemget(IPC_PRIVATE, 2, 0600 | IPC_CREAT | IPC_EXCL);
        sharedArea->semEmptyId = psemget(IPC_PRIVATE, 2, 0600 | IPC_CREAT | IPC_EXCL);
        sharedArea->ResponseAvaiable = psemget(IPC_PRIVATE, NBUFFERS, 0600 | IPC_CREAT | IPC_EXCL);

        for(int i = 0; i<NBUFFERS; i++) {
                psem_up(sharedArea->semEmptyId, FREE_BUFFER);      //free buffer has NBUFFERS elements
        }

        psem_up(sharedArea->semAccessId, FREE_BUFFER);
        psem_up(sharedArea->semAccessId, PENDING_REQUEST); //both can be accessed

    }

    /* -------------------------------------------------------------------- */

    /* Free all allocated synchronization resources and data structures */
    void destroy()
    {
        require(sharedArea != NULL, "sharea area must be allocated");

        /* 
         * TODO point
         * Destroy synchronization elements
         */
        psemctl(sharedArea->semAccessId, 0, IPC_RMID);
        psemctl(sharedArea->semEmptyId, 0, IPC_RMID);
        psemctl(sharedArea->ResponseAvaiable, 0, IPC_RMID);

        /* 
         * TODO point
        *  Destroy the shared memory
        */
        /* detach shared memory from process addressing space */
        pshmdt(sharedArea);
        /* destroy the shared memory */
        pshmctl(sharedAreaId, IPC_RMID, NULL);

        /* nullify */
        sharedArea = NULL;
    }

    /* -------------------------------------------------------------------- */
    /* -------------------------------------------------------------------- */

    /* Insertion a token into a fifo */
    static void fifoIn(uint32_t idx, uint32_t token)
    {
#ifdef __DEBUG__
        fprintf(stderr, "%s(idx: %u, token: %u)\n", __FUNCTION__, idx, token);
#endif

        require(idx == FREE_BUFFER or idx == PENDING_REQUEST, "idx is not valid");
        require(token < NBUFFERS, "token is not valid");

        /* 
         * TODO point
         * Replace with your code, 
         * avoiding race conditions and busy waiting
         */
        /* decrement emptiness, blocking if necessary, and lock access */
                //sharedArea->fifo[idx]         //get fifo
                //sharedArea->semAccessId[idx]    //Access semaphore
                //sharedArea->semEmpty[idx]     //empty semaphore
        psem_down(sharedArea->semAccessId, idx);

        /* Insert */
        //?gaussianDelay(0.1, 0.5);
        sharedArea->fifo[idx].tokens[sharedArea->fifo[idx].ii] = idx;
        sharedArea->fifo[idx].ii = (sharedArea->fifo[idx].ii + 1) % NBUFFERS;
        sharedArea->fifo[idx].cnt++;

        /* unlock access and increment fullness */
        psem_up(sharedArea->semAccessId, idx);
        psem_up(sharedArea->semEmptyId, idx);

    }

    /* -------------------------------------------------------------------- */

    /* Retrieve a token from a fifo  */

    static uint32_t fifoOut(uint32_t idx)
    {
#ifdef __DEBUG__
        fprintf(stderr, "%s(idx: %u)\n", __FUNCTION__, idx);
#endif

        require(idx == FREE_BUFFER or idx == PENDING_REQUEST, "idx is not valid");

        /* 
         * TODO point
         * Replace with your code, 
         * avoiding race conditions and busy waiting
         */
        int id;
        psem_down(sharedArea->semAccessId, idx);

        /* Retreive */
        //?gaussianDelay(0.1, 0.5);
        id = sharedArea->fifo[idx].tokens[sharedArea->fifo[idx].ri];
        sharedArea->fifo[idx].ri = (sharedArea->fifo[idx].ri + 1) % NBUFFERS;
        sharedArea->fifo[idx].cnt++;

        /* unlock access and increment fullness */
        psem_up(sharedArea->semAccessId, idx);
        psem_down(sharedArea->semEmptyId, idx);
        return id;
    }

    /* -------------------------------------------------------------------- */
    /* -------------------------------------------------------------------- */

    uint32_t getFreeBuffer()
    {
#ifdef __DEBUG__
        fprintf(stderr, "%s()\n", __FUNCTION__);
#endif

        /* 
         * TODO point
         * Replace with your code, 
         */
        return fifoOut(FREE_BUFFER);
    }

    /* -------------------------------------------------------------------- */

    void putRequestData(uint32_t token, const char *data)
    {
#ifdef __DEBUG__
        fprintf(stderr, "%s(token: %u, ...)\n", __FUNCTION__, token);
#endif

        require(token < NBUFFERS, "token is not valid");
        require(data != NULL, "data pointer can not be NULL");

        /* 
         * TODO point
         * Replace with your code, 
         */
        strcpy(sharedArea->pool[token].req, data);
    }

    /* -------------------------------------------------------------------- */

    void submitRequest(uint32_t token)
    {
#ifdef __DEBUG__
        fprintf(stderr, "%s(token: %u)\n", __FUNCTION__, token);
#endif

        require(token < NBUFFERS, "token is not valid");

        /* 
         * TODO point
         * Replace with your code, 
         */
        
        fifoIn(PENDING_REQUEST, token);
    }

    /* -------------------------------------------------------------------- */

    void waitForResponse(uint32_t token)
    {
#ifdef __DEBUG__
        fprintf(stderr, "%s(token: %u)\n", __FUNCTION__, token);
#endif

        require(token < NBUFFERS, "token is not valid");

        /* 
         * TODO point
         * Replace with your code, 
         * avoiding race conditions and busy waiting
         */
        
        psem_down(sharedArea->ResponseAvaiable, token);
    }

    /* -------------------------------------------------------------------- */

    void getResponseData(uint32_t token, Response *resp)
    {
#ifdef __DEBUG__
        fprintf(stderr, "%s(token: %u, ...)\n", __FUNCTION__, token);
#endif

        require(token < NBUFFERS, "token is not valid");
        require(resp != NULL, "resp pointer can not be NULL");

        /* 
         * TODO point
         * Replace with your code, 
         */
        resp = &sharedArea->pool[token].resp;
    }

    /* -------------------------------------------------------------------- */

    void releaseBuffer(uint32_t token)
    {
#ifdef __DEBUG__
        fprintf(stderr, "%s(token: %u)\n", __FUNCTION__, token);
#endif

        require(token < NBUFFERS, "token is not valid");

        /* 
         * TODO point
         * Replace with your code, 
         */
        fifoIn(FREE_BUFFER, token);
    }

    /* -------------------------------------------------------------------- */
    /* -------------------------------------------------------------------- */

    uint32_t getPendingRequest()
    {
#ifdef __DEBUG__
        fprintf(stderr, "%s()\n", __FUNCTION__);
#endif

        /* 
         * TODO point
         * Replace with your code, 
         */
        return fifoOut(PENDING_REQUEST);
    }

    /* -------------------------------------------------------------------- */

    void getRequestData(uint32_t token, char *data)
    {
#ifdef __DEBUG__
        fprintf(stderr, "%s(token: %u, ...)\n", __FUNCTION__, token);
#endif

        require(token < NBUFFERS, "token is not valid");
        require(data != NULL, "data pointer can not be NULL");

        /* 
         * TODO point
         * Replace with your code, 
         */
        data = sharedArea->pool[token].req;
    }

    /* -------------------------------------------------------------------- */

    void putResponseData(uint32_t token, Response *resp)
    {
#ifdef __DEBUG__
        fprintf(stderr, "%s(token: %u, ...)\n", __FUNCTION__, token);
#endif

        require(token < NBUFFERS, "token is not valid");
        require(resp != NULL, "resp pointer can not be NULL");

        /* 
         * TODO point
         * Replace with your code, 
         */
        sharedArea->pool[token].resp = *resp;
    }

    /* -------------------------------------------------------------------- */

    void notifyClient(uint32_t token)
    {
#ifdef __DEBUG__
        fprintf(stderr, "%s(token: %u)\n", __FUNCTION__, token);
#endif

        require(token < NBUFFERS, "token is not valid");

        /* 
         * TODO point
         * Replace with your code, 
         * avoiding race conditions and busy waiting
         */
        psem_up(sharedArea->ResponseAvaiable, token);
    }

    /* -------------------------------------------------------------------- */

}
