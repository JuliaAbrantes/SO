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
    enum { FREE_BUFFER=0, PENDING_REQUEST};

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
        int semAccess[2]; //semaphore to see if can access
        int semEmpty[2]; //semaphore to see if empty
        int ResponseAvaiable[NBUFFERS]; //semaphore array to see if response avaible
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
#if __DEBUG__
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
        FIFO *fifo = &sharedArea->fifo[FREE_BUFFER];
        for (uint32_t i = 0; i < NBUFFERS; i++)
        {
            fifo->tokens[i] = i;
        }
        fifo->ii = fifo->ri = 0;
        fifo->cnt = NBUFFERS;

        /* init fifo 1 (pending requests) */
        fifo = &sharedArea->fifo[PENDING_REQUEST];
        for (uint32_t i = 0; i < NBUFFERS; i++)
        {
            fifo->tokens[i] = NBUFFERS; // used to check for errors
        }
        fifo->ii = fifo->ri = 0;
        fifo->cnt = 0;

        /* 
         * TODO point
         * Init synchronization elements
         */
        psem_up(sharedArea->semAccess[PENDING_REQUEST], PENDING_REQUEST); //both can be accessed
        psem_up(sharedArea->semAccess[FREE_BUFFER], FREE_BUFFER);
        psem_down(sharedArea->semEmpty[PENDING_REQUEST], PENDING_REQUEST); //pending request is empty
        for(int i = 0; i<NBUFFERS; i++)
                psem_up(sharedArea->semEmpty[FREE_BUFFER], FREE_BUFFER);      //free buffer has NBUFFERS elements
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
        /* detach shared memory from process addressing space */
        pshmdt(SharedArea);

        /* 
         * TODO point
        *  Destroy the shared memory
        */
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
#if __DEBUG__
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
                //sharedArea->semAccess[idx]    //Access semaphore
                //sharedArea->semEmpty[idx]     //empty semaphore
        psem_down(sharedArea->semAccess, idx);

        /* Insert */
        gaussianDelay(0.1, 0.5);
        sharedArea->fifo[idx]->tokens[sharedArea->fifo[idx]->ii] = id;
        sharedArea->fifo[idx]->ii = (sharedArea->fifo[idx]->ii + 1) % FIFOSZ;
        sharedArea->fifo[idx]->cnt++;

        /* unlock access and increment fullness */
        psem_up(sharedArea->semAccess, idx);
        psem_up(sharedArea->semEmpty, idx);

    }

    /* -------------------------------------------------------------------- */

    /* Retrieve a token from a fifo  */

    static uint32_t fifoOut(uint32_t idx)
    {
#if __DEBUG__
        fprintf(stderr, "%s(idx: %u)\n", __FUNCTION__, idx);
#endif

        require(idx == FREE_BUFFER or idx == PENDING_REQUEST, "idx is not valid");

        /* 
         * TODO point
         * Replace with your code, 
         * avoiding race conditions and busy waiting
         */
        int id;
        psem_down(sharedArea->semAccess, idx);

        /* Retreive */
        gaussianDelay(0.1, 0.5);
        id = sharedArea->fifo[idx]->tokens[sharedArea->fifo[idx]->ri];
        sharedArea->fifo[idx]->ri = (sharedArea->fifo[idx]->ri + 1) % FIFOSZ;
        sharedArea->fifo[idx]->cnt++;

        /* unlock access and increment fullness */
        psem_up(sharedArea->semAccess, idx);
        psem_down(sharedArea->semEmpty, idx);
        return id;
    }

    /* -------------------------------------------------------------------- */
    /* -------------------------------------------------------------------- */

    uint32_t getFreeBuffer()
    {
#if __DEBUG__
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
#if __DEBUG__
        fprintf(stderr, "%s(token: %u, ...)\n", __FUNCTION__, token);
#endif

        require(token < NBUFFERS, "token is not valid");
        require(data != NULL, "data pointer can not be NULL");

        /* 
         * TODO point
         * Replace with your code, 
         */
        sharedArea.pool[token].req = data;
    }

    /* -------------------------------------------------------------------- */

    void submitRequest(uint32_t token)
    {
#if __DEBUG__
        fprintf(stderr, "%s(token: %u)\n", __FUNCTION__, token);
#endif

        require(token < NBUFFERS, "token is not valid");

        /* 
         * TODO point
         * Replace with your code, 
         */
        
        fifoIn(PENDING_REQUEST, token)
    }

    /* -------------------------------------------------------------------- */

    void waitForResponse(uint32_t token)
    {
#if __DEBUG__
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
#if __DEBUG__
        fprintf(stderr, "%s(token: %u, ...)\n", __FUNCTION__, token);
#endif

        require(token < NBUFFERS, "token is not valid");
        require(resp != NULL, "resp pointer can not be NULL");

        /* 
         * TODO point
         * Replace with your code, 
         */
        resp = sharedArea->pool[token].resp;
    }

    /* -------------------------------------------------------------------- */

    void releaseBuffer(uint32_t token)
    {
#if __DEBUG__
        fprintf(stderr, "%s(token: %u)\n", __FUNCTION__, token);
#endif

        require(token < NBUFFERS, "token is not valid");

        /* 
         * TODO point
         * Replace with your code, 
         */
        fifoIn(FREE_BUFFER, token)
    }

    /* -------------------------------------------------------------------- */
    /* -------------------------------------------------------------------- */

    uint32_t getPendingRequest()
    {
#if __DEBUG__
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
#if __DEBUG__
        fprintf(stderr, "%s(token: %u, ...)\n", __FUNCTION__, token);
#endif

        require(token < NBUFFERS, "token is not valid");
        require(data != NULL, "data pointer can not be NULL");

        /* 
         * TODO point
         * Replace with your code, 
         */
        return sharedArea->pool[token].req;
    }

    /* -------------------------------------------------------------------- */

    void putResponseData(uint32_t token, Response *resp)
    {
#if __DEBUG__
        fprintf(stderr, "%s(token: %u, ...)\n", __FUNCTION__, token);
#endif

        require(token < NBUFFERS, "token is not valid");
        require(resp != NULL, "resp pointer can not be NULL");

        /* 
         * TODO point
         * Replace with your code, 
         */
        sharedArea->pool[token].resp = resp;
    }

    /* -------------------------------------------------------------------- */

    void notifyClient(uint32_t token)
    {
#if __DEBUG__
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
