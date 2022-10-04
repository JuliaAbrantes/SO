/*
 *
 * \author (2016) Artur Pereira <artur at ua.pt>
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <stdint.h>
#include <iostream>

#include "ull.h" //para usar fora ull::Register
//using ull; //faz com que integre automaticamente

namespace ull
{
    /* ************************************************* */

    /* The information support data structure  */
    struct Register
    {
        uint32_t nmec;       //!< student number
        const char *name;    //!< student name
    };

    /* The linked-list support data structure */
    struct Node 
    {
        Register reg;
        struct Node *next;
        //da para ter mais listas sem ter mais elementos, 
        //ou seja, mais next ordenados por outros critérios mas usando os mesmos elementos
    };

    static Node *head = NULL;

    /* ************************************************* */

    void reset()
    {
    }

    /* ************************************************* */

    void load(const char *fname)
    {
    }

    /* ************************************************* */

    void print()
    {
        Node *p = new Node();
        for (p = head; p != NULL; p = p->next) {
            std::cerr << p->reg.nmec;
            std::cerr << p->reg.name;
        }
    }

    /* ************************************************* */

    void insert(uint32_t nmec, const char *name)
    {
        Node *p = new Node();
        Register regis;
        regis.nmec = nmec;
        regis.name = name;
        p->reg = regis;

        Node *p1 = new Node();
        Node *p2 = new Node();
        for(p1 = NULL, p2 = head; p2!=NULL; p1 = p2, p2->next) {
            std::cerr << p1;
            if(p2->reg.nmec > p->reg.nmec) break;
        }

        if(p1 == NULL) {        //Se p1 é null, inserir no começo da lista
            p->next = p2;
            head = p;
        }
        else if(p2 == NULL) {   //Se p2 é null, inserir no fim da lista
            p->next = NULL;
            p2->next = p;
        }
        else {                  //se p1!= null && p2!= null, p1, aponta pra p, e p aponta pra p2
            p->next = p2;
            p1->next = p;
        }

        
    }

    /* ************************************************* */

    const char *query(uint32_t nmec)
    {
        return NULL;
    }

    /* ************************************************* */

    void remove(uint32_t nmec)
    {
    }

    /* ************************************************* */
}