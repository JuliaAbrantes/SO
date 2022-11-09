/**
 * @file
 *
 * \brief A hospital pediatric urgency with a Manchester triage system.
 */

#include  <stdio.h>
#include  <stdlib.h>
#include  <string.h>
#include  <libgen.h>
#include  <unistd.h>
#include  <sys/wait.h>
#include  <sys/types.h>
#include  <thread.h>
#include  <math.h>
#include  <stdint.h>
#include  <signal.h>
#include  <utils.h>
#include  "settings.h"
#include  "pfifo.h"

//#include "thread.h"
#include "process.h"

#include <iostream>

#define USAGE "Synopsis: %s [options]\n" \
   "\t----------+-------------------------------------------\n" \
   "\t  Option  |          Description                      \n" \
   "\t----------+-------------------------------------------\n" \
   "\t -p num   | number of patients (dfl: 4)               \n" \
   "\t -n num   | number of nurses (dfl: 1)                 \n" \
   "\t -d num   | number of doctors (dfl: 1)                \n" \
   "\t -h       | this help                                 \n" \
   "\t----------+-------------------------------------------\n"

/**
 * \brief Patient data structure
 */
typedef struct
{
   char name[MAX_NAME+1];
   int done; // semaphore id if done
} Patient;

typedef struct
{
    int num_patients;
    Patient all_patients[MAX_PATIENTS];
    PriorityFIFO triage_queue;
    PriorityFIFO doctor_queue;
} HospitalData;

int hdid = -1;
HospitalData * hd = NULL;

/**
 *  \brief patient verification test
 */
#define check_valid_patient(id) do { check_valid_id(id); check_valid_name(hd->all_patients[id].name); } while(0)

int random_manchester_triage_priority();
void new_patient(Patient* patient); // initializes a new patient
void random_wait();

/* ************************************************* */

/* changes may be required to this function */
void init_simulation(uint32_t np)
{
   printf("Initializing simulation\n");
   //hd = (HospitalData*)mem_alloc(sizeof(HospitalData)); // mem_alloc is a malloc with NULL pointer verification
   //memset(hd, 0, sizeof(HospitalData));
   
   /* create the shared memory */
   hdid = pshmget(IPC_PRIVATE, sizeof(HospitalData), 0600 | IPC_CREAT | IPC_EXCL);
   /*  attach shared memory to process addressing space */
   hd = (HospitalData*)pshmat(hdid, NULL, 0);

   hd->num_patients = np;
   init_pfifo(&hd->triage_queue);
   init_pfifo(&hd->doctor_queue);

   for (int i = 0; i<hd->num_patients; i++) {
      hd->all_patients[i].done = psemget(IPC_PRIVATE, 1, 0600 | IPC_CREAT | IPC_EXCL);
   }

}

void end_simulation()
{
   /*finalie fifos*/
   fin_pfifo(&hd->triage_queue);
   fin_pfifo(&hd->doctor_queue);
   /*destroy patient done semaphore*/
   for(int i = 0; i<hd->num_patients; i++) {
      psemctl(hd->all_patients[i].done, 0, IPC_RMID, NULL);
   }
   /*detach shared memory*/
   pshmdt(hd);
   /*destroy shared memory*/
   pshmctl(hdid, IPC_RMID, NULL);
}

/* ************************************************* */

void nurse_iteration()
{
   printf("\e[34;01mNurse: get next patient\e[0m\n");
   uint32_t patient = retrieve_pfifo(&hd->triage_queue);
   check_valid_patient(patient);
   printf("\e[34;01mNurse: evaluate patient %u priority\e[0m\n", patient);
   uint32_t priority = random_manchester_triage_priority();
   printf("\e[34;01mNurse: add patient %u with priority %u to doctor queue\e[0m\n", patient, priority);
   insert_pfifo(&hd->doctor_queue, patient, priority);
}

/* ************************************************* */

void doctor_iteration()
{
   printf("\e[32;01mDoctor: get next patient\e[0m\n");
   uint32_t patient = retrieve_pfifo(&hd->doctor_queue);
   check_valid_patient(patient);
   printf("\e[32;01mDoctor: treat patient %u\e[0m\n", patient);
   random_wait();
   printf("\e[32;01mDoctor: patient %u treated\e[0m\n", patient);
   psem_up(hd->all_patients[patient].done, 0);
}

/* ************************************************* */

void patient_goto_urgency(int id)
{
   new_patient(&hd->all_patients[id]);
   check_valid_name(hd->all_patients[id].name);
   printf("\e[30;01mPatient %s (number %u): get to hospital\e[0m\n", hd->all_patients[id].name, id);
   insert_pfifo(&hd->triage_queue, id, 1); // all elements in triage queue with the same priority!
}

/* changes may be required to this function */
void patient_wait_end_of_consultation(int id)
{
   check_valid_name(hd->all_patients[id].name);
   psem_down(hd->all_patients[id].done, 0);
   printf("\e[30;01mPatient %s (number %u): health problems treated\e[0m\n", hd->all_patients[id].name, id);
}

/* changes are required to this function */
void patient_life(int id)
{
   patient_goto_urgency(id);
   patient_wait_end_of_consultation(id);
   exit(0);
}

/* ************************************************* */

int main(int argc, char *argv[])
{
   uint32_t npatients = 4;  ///< number of patients
   uint32_t nnurses = 1;    ///< number of triage nurses
   uint32_t ndoctors = 1;   ///< number of doctors

   /* command line processing */
   int option;
   while ((option = getopt(argc, argv, "p:n:d:h")) != -1)
   {
      switch (option)
      {
         case 'p':
            npatients = atoi(optarg);
            if (npatients < 1 || npatients > MAX_PATIENTS)
            {
               fprintf(stderr, "Invalid number of patients!\n");
               return EXIT_FAILURE;
            }
            break;
         case 'n':
            nnurses = atoi(optarg);
            if (nnurses < 1)
            {
               fprintf(stderr, "Invalid number of nurses!\n");
               return EXIT_FAILURE;
            }
            break;
         case 'd':
            ndoctors = atoi(optarg);
            if (ndoctors < 1)
            {
               fprintf(stderr, "Invalid number of doctors!\n");
               return EXIT_FAILURE;
            }
            break;
         case 'h':
            printf(USAGE, basename(argv[0]));
            return EXIT_SUCCESS;
         default:
            fprintf(stderr, "Non valid option!\n");
            fprintf(stderr, USAGE, basename(argv[0]));
            return EXIT_FAILURE;
      }
   }

   /* init simulation */
   init_simulation(npatients);
   
   /* launching the doctors */
   int doctorsPID[ndoctors];
   for (uint32_t id = 0; id < ndoctors; id++)
   {
      if ((doctorsPID[id] = pfork()) == 0)
      {
         while(true) {
            random_wait();
            doctor_iteration();
         }
      }
   } 
   
   /* launching the nurses */
   int nursesPID[nnurses];
   for (uint32_t id = 0; id < nnurses; id++)
   {
      if ((nursesPID[id] = pfork()) == 0)
      {
         while(true) {
            srand(getpid()); /* start random generator */
            random_wait();
            nurse_iteration();
         }
      }
   }

   /* launching the patients */
   int patientsPID[npatients];
   for (uint32_t id = 0; id < npatients; id++)
   {
      if ((patientsPID[id] = pfork()) == 0)
      {
         srand(getpid()); /* start random generator */
         random_wait();
         patient_life(id);
      }
   }

   for (uint32_t id = 0; id < npatients; id++)
   { //wait patients
      pwaitpid(patientsPID[id], NULL, 0);
   }
   for (uint32_t id = 0; id < ndoctors; id++)
   { //fake patients to doctor
      insert_pfifo(&hd->doctor_queue, 666, 1);
   }
   for (uint32_t id = 0; id < nnurses; id++)
   { //fake patients to nurse
      insert_pfifo(&hd->triage_queue, 666, 1);
   }
   for (uint32_t id = 0; id < ndoctors; id++)
   { //wait doctors
      pwaitpid(doctorsPID[id], NULL, 0);
   }
   for (uint32_t id = 0; id < nnurses; id++)
   { //wait nurses
      pwaitpid(nursesPID[id], NULL, 0);
   }

   end_simulation();

   return EXIT_SUCCESS;
}


/* YOU MAY IGNORE THE FOLLOWING CODE */

int random_manchester_triage_priority()
{
   int result;
   int perc = (int)(100*(double)rand()/((double)RAND_MAX)); // in [0;100]
   if (perc < 10)
      result = RED;
   else if (perc < 30)
      result = ORANGE;
   else if (perc < 50)
      result = YELLOW;
   else if (perc < 75)
      result = GREEN;
   else
      result = BLUE;
   return result;
}

static char **names = (char *[]) {"Ana", "Miguel", "Luis", "Joao", "Artur", "Maria", "Luisa", "Mario", "Augusto", "Antonio", "Jose", "Alice", "Almerindo", "Gustavo", "Paulo", "Paula", NULL};

char* random_name()
{
   static int names_len = 0;
   if (names_len == 0)
   {
      for(names_len = 0; names[names_len] != NULL; names_len++)
         ;
   }

   return names[(int)(names_len*(double)rand()/((double)RAND_MAX+1))];
}

void new_patient(Patient* patient)
{
   strcpy(patient->name, random_name());
}

void random_wait()
{
   usleep((useconds_t)(MAX_WAIT*(double)rand()/(double)RAND_MAX));
}

