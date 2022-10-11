/*
    my application interface
*/
#include <stdio.h>
#include <stdlib.h>
#include "memoryHandler.h" 

namespace myApp
{
    struct ServiceRequest
    {
        string text;
    };
    //the number of characters, the number of digits and the number of letters
    struct ServiceResponse
    {
        int nChar;
        int nDigits;
        int nLetters;
    };

    void callService(ServiceRequest & req, ServiceResponse & res) {
        int id = getFreeBuffer ();                      /* take a buffer out of fifo of free buffers */
        putRequestData ( data , id );                   /* put request data on buffer */
        addNewPendingRequest ( id );                    /* add buffer to fifo of pending requests */
        waitForResponse ( id );                         /* wait (blocked) until a response is available */
        ServiceResponse res = getResponseData ( id );   /* take response out of buffer */
        releaseBuffer ( id );                           /* buffer is free, so add it to fifo of free buffers */

    }

    void processService(){
        int id = getPendingRequest ();                  /* take a buffer out of fifo of pending requests */
        ServiceRequest req = getRequestData ( id );     /* take the request */
        ServiceResponse res = produceResponse ( req );  /* produce a response */
        putResponseData ( res , id );                   /* put response data on buffer */
        signalResponseIsAvailable ( id );               /* so client is waked up */
    }

}