
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <sys/ipc.h>
#include <sys/msg.h>


///Time Definers
#define INTER_MEM_ACCS_T 0.00001	//time between Process's iterations (10^-9 * 10^4 )
#define MEM_WR_T 0.000001			// The time it takes to Process for Write (10^-9 * 10^3 )
#define TIME_BETWEEN_SNAPSHOTS 0.1	// Amount of time between each Printer's iterations (10^-9 * 10^8 )
#define HD_ACCS_T	0.001		// Time between each hard disk's (HD) iteration (10^-9 * 10^6 )
#define SIM_TIME 1				// Time of simulation

//Process Definers
#define PROCESS1 4
#define PROCESS2 5

///Case Definers
#define HD 0
#define SLEEP 1
#define MMU 1
#define EVICT 2
#define EVICTER 2
#define ACK 2
#define WRITE 1
#define READ 0



// Memory Definers

#define N 5						// Number of max memory slots
#define USED_SLOTS_TH 3 		// Evictor will clear memory slots until the number of slots in memory are below USED_SLOTS_TH


//Page Definers
#define EMPTY 0
#define CLEAN 1
#define DIRTY 2
#define PAGE_FAULT 0
#define NOT_PAGE_FAULT 1
#define WR_RATE 0.5				// Probability of Process requesting to Write
#define HIT_RATE 0.5			// Probability of Process succesfully Writing/Reading
#define ANY 3


// Struct for the information

struct buffer{
    long class;
    int info;
    int source;
};

//Main function for MMU
void *MMU_main();	//MMU main function

//Function Used in MMU
void *evicterPages();	// Evicts pages from the memory when the memory is full
void *hardDisk();	// Function for The hard disk
void *printer();	//Function to print screenshots of the memory





//Mutexs
pthread_mutex_t lock;	// Mutex for access to memory
pthread_mutexattr_t attr;

//Global Variables

int msqid[6];	// Array of ID's
int memory[N];	// Array representing memory, In our case we choose N=5
int numPages = 0;	// represents how many pages are in the memory



void main()
{

    void *status;
    int request,result,process;
    pthread_t mmu;
    pthread_t evict;
    pthread_t hd;
    pthread_t print;
    pid_t pid[2];
    struct buffer message;
    int i;
    for (i=0;i<N;i++)
    {
        memory[i]=EMPTY;
    }

    for(i=0;i<6;i++)
    {
        msqid[i] = msgget(i, 0600 | IPC_CREAT); // For Inter-Process Communication
    }

    //Init Mutexes

    if (pthread_mutexattr_init(&attr) != 0)
        {
        perror("Error to create pthread_mutex_attr_init()");
        exit(1);
        }


    if (pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK_NP) != 0) 	// Init. "attr" as ERRORCHECK
        {
        perror("Error to pthread_mutexattr_settype()");
        exit(1);
        }


    if(pthread_mutex_init(&lock, &attr) != 0 )
        {

        perror("Error creating Mutex.\n Exiting Program.\n");
        exit(1);
        }

    //Creating Threads for MMU,HARD DISK,EVICTER AND PRINT

    if(pthread_create(&mmu, NULL, MMU_main, NULL) != 0)
        {
        perror("Error to creating MMU.\nExiting The Program.\n");
        exit(1);
        }

    if(pthread_create(&hd, NULL, hardDisk, NULL) != 0)
        {
        perror("Error to creating HD.\nExiting The Program.\n");
        exit(1);
        }

    if(pthread_create(&evict, NULL, evicterPages, NULL) != 0)
        {
        perror("Error to creating Evicter.\nExiting The Program.\n");
        exit(1);
        }

    if(pthread_create(&print, NULL, printer, NULL) != 0)
        {
        perror("Error to creating Printer.\nExiting The Program.\n");
        exit(1);
        }


    //Creating the process (in our case there is 2 )

    for(int i=0;i<2;i++)
    {
        pid[i] = fork();
        if(pid[i] == -1)	//Error occured
            {
            perror("Error to create a Child Process \n");
            exit(EXIT_FAILURE);
            }
        else if(pid[i] == 0) //The Process is The Child
            {

            if(i==0) // This is The first child
            {
                process = PROCESS1;
            }
            else // This is The second child
            {
                process = PROCESS2;
            }
            while(1)
            {
               sleep((unsigned int) INTER_MEM_ACCS_T);

                if(rand()&1)	// Probability of 0.5 if request is WRITE/READ
                    {
                    request = WRITE;
                    }
                else
                {
                    request = READ;
                }

                //Sending request to The Mmu
                message.class = 1;
                message.source = process;
                message.info = request;

                result = msgsnd(msqid[MMU], &message, sizeof(message), 0);
                if(result != 0)
                {
                    perror("Error for Requesting MMU from Process\n");
                    exit(EXIT_FAILURE);
                }

                //Waiting For ack
                result = 0; // clean the var that hold the message req

                while(result == 0)	// Until we get ack or there is an error
                    {
                    result = msgrcv(msqid[process], &message, sizeof(message), 0, 0);	// Read message from Process i Mailbox
                    }
                if(result < 0)
                {
                    perror("Error To receiving ACK from MMU\n");
                    exit(EXIT_FAILURE);
                }
            }
            }
    }

    sleep(SIM_TIME);

    // Simulation Over - Killing all Process /Threads
    for(int i=0;i<2;i++)
        {
        kill(pid[i], SIGINT);
        }
    pthread_kill(print, SIGINT);	// Kill Printer
    pthread_kill(evict, SIGINT);	// Kill Evicter
    pthread_kill(mmu, SIGINT);		// Kill MMU
    pthread_kill(hd, SIGINT);		// Kill HD
    pthread_mutex_destroy(&lock);	// Destroy the mutex "lock"
}


void *printer()
/* At the beginning of the simulation, and later every TIME_BETWEEN_SNAPSHOTS, the printer prints the "memory" */
{
    double sleepTime;
    int memoryPages[N];
    int error;
    while(1)
    {
        error = EBUSY;
        while(error == EBUSY){
            error = pthread_mutex_trylock(&lock);
            }
        if(error){
            printf("Error to unlock mutex\n");
            exit(EXIT_FAILURE);
        }
        else{
            // CRITICAL SECTION
            for(int i=0;i<N;i++){
                memoryPages[i] = memory[i];
            }
        }
        error = pthread_mutex_unlock(&lock);
        if(error == EPERM){
            printf("Error to unlock mutex\n");
        }
        for(int i=0;i<N;i++){
            if(memoryPages[i] == EMPTY){
                printf("%d|-\n",i);
            }
            else if(memoryPages[i] == CLEAN){
                printf("%d|0\n",i);
            }
            else if(memoryPages[i] == DIRTY){
                printf("%d|1\n",i);
            }
        }

        printf("\n\n");	// Spacing between prints
        // Sleep between every print
        sleepTime = (double)TIME_BETWEEN_SNAPSHOTS;
        sleep(sleepTime);
    }
}
void *MMU_main()
/* In this function we will simulates the MMU,
 the MMU will receive messages and send ACKs */
{
    double sleepTime;
    int isPageFault;
    int indexProcess;
    int writeOrRead;
    int result;
    struct buffer message;
    message.class = 1;
    int reqHD = 0;		// flag for if to writeOrRead to Read a page from HD
    int error;
    int randPage;

    while(1){
        result = 0;
        // Process requests checking
        while(result == 0){
            result = msgrcv(msqid[MMU], &message, sizeof(message), 0, 0);
        }
        if(result < 0){
            perror("Error receiving request\n");
            exit(EXIT_FAILURE);
        }
        writeOrRead = message.info;
        indexProcess = message.source;
        if(numPages){   // If memory is not empty
            if(rand()&1){
                isPageFault = NOT_PAGE_FAULT;
            }
            else{
                isPageFault = PAGE_FAULT;
            }
            if(isPageFault == NOT_PAGE_FAULT){
                if(writeOrRead == WRITE)
                {
                    //Sleep to simulate writing
                    sleepTime = (double)MEM_WR_T;
                    sleep(sleepTime);
                    //Random page is now DIRTY
                    randPage = (rand() % ( (numPages-1) + 1));
                    //Try to change in memory
                    error = EBUSY;
                    while(error == EBUSY){
                        error = pthread_mutex_trylock(&lock);
                        }
                    if(error){
                        printf("Error to unlock mutex\n");
                        exit(EXIT_FAILURE);
                        }
                    else{	//	Enter the critical section
                        memory[randPage] = DIRTY;
                    }
                    error = pthread_mutex_unlock(&lock); // Unlock mutex
                    if(error == EPERM){
                        printf("Error to unlock mutex\n");
                    }
                }
                result = 0;
                result = msgsnd(msqid[indexProcess], &message, sizeof(message), 0);
                if(result != 0){
                    perror("Error to send An ACK to the Process\n");
                    exit(EXIT_FAILURE);
                }
            }
            else if(isPageFault == PAGE_FAULT){
                if(numPages == N)	//Memory is full
                    {
                    // Wake up Evicter
                    struct buffer wakeup;
                        wakeup.class = 1;
                        wakeup.info = ANY;
                        wakeup.source = ANY;
                    result = msgsnd(msqid[EVICTER], &wakeup, sizeof(wakeup), 0);
                    if(result != 0){
                        perror("Error to waking up Evicter\n");
                        exit(EXIT_FAILURE);
                    }
                    // Wait for ACK
                    result = 0;
                    while(result == 0){
                        result = msgrcv(msqid[MMU], &wakeup, sizeof(wakeup), 0, 0);
                    }
                    if(result < 0){
                        perror("Error to woken up by Evicter\n");
                        exit(EXIT_FAILURE);
                    }
                    }	
            }
            }	
            if(numPages == 0 || isPageFault == PAGE_FAULT)
            {
                // Request page from hard disc
                struct buffer pg_req_from_hd;
                pg_req_from_hd.class = 1;
                pg_req_from_hd.info = ANY;
                pg_req_from_hd.source = MMU;
                result = msgsnd(msqid[HD], &pg_req_from_hd, sizeof(pg_req_from_hd), 0);
                if(result != 0){
                    perror("Error to sending READ to HD\n");
                    exit(EXIT_FAILURE);
                }
                // Wait for ACK
                result = 0;
                while(result == 0)
                {
                    result = msgrcv(msqid[MMU], &pg_req_from_hd, sizeof(pg_req_from_hd), 0, 0);
                }
                if(result < 0)
                {
                    perror("Error to receiving ACK from HD\n");
                    exit(EXIT_FAILURE);
                }
                //Try to save in memory
                error = EBUSY;
                while(error == EBUSY){
                    error = pthread_mutex_trylock(&lock);
                    }
                if(error){
                    printf("Error to lock mutex \n");
                    exit(EXIT_FAILURE);
                }
                else{	//Enter the critical section
                    memory[numPages] = CLEAN;	
                    numPages = numPages + 1;
                }
                error = pthread_mutex_unlock(&lock); 
                if(error == EPERM){
                    printf("Error to unlock mutex \n");
                }
                //Send ACK
                message.info = ACK;
                message.source = MMU;
                result = 0;
                result = msgsnd(msqid[indexProcess], &message, sizeof(message), 0);
                if(result != 0)
                {
                    perror("Error to send ACK to Process\n");
                    exit(EXIT_FAILURE);
                }
            }
    }

}
void *hardDisk()
/*
In This function the hard disk, receives requests from the MMU
and ACKs them.
*/
{
    double sleepTime;
    int result;
    struct buffer message;
    message.class = 1;
    int request;
    while(1)
    {
        result = 0;
        //Check for Receiving requests
        while(result == 0)
        {
            result = msgrcv(msqid[HD], &message, sizeof(message), 0, 0);	// Receive request
        }
        if(result<0)
        {
            perror("Error to receiving requests\n");
            exit(EXIT_FAILURE);
        }
        //Sleep
        else
        {
            sleepTime = (double)HD_ACCS_T;
            sleep(sleepTime);
        }
        //Requesting Ack
        request = message.source;
        message.info = ACK;
        message.source = HD;
        result = msgsnd(msqid[request], &message, sizeof(message), 0);
        if(result != 0 )
        {
            perror("Error to sending ACK from HD\n");
            exit(EXIT_FAILURE);
        }
    }
}

void *evicterPages()
/*
In This function We helps the MMU
clear memory slots when the memory is full.
*/
{
    int result = 0;
    int switch_mode = SLEEP;	// For switch case
    int error = 0;
    int pg_to_evict = N - USED_SLOTS_TH;  //pages the Evictor need to evict
    struct buffer wakeup;
    wakeup.class = 1;
    int i;

    while(1)
    {
        switch(switch_mode)
        {
            case SLEEP:		// The Evicter sleeps until woken up

            result = 0;
            while(result == 0)
            {
                result = msgrcv(msqid[EVICTER], &wakeup, sizeof(wakeup), 0, 0);
            }

            if(result < 0)
            {
                perror("Evicter failed being woken up by MMU\n");
                exit(EXIT_FAILURE);
            }
            switch_mode = EVICT;
            break;

            case EVICT:

                i = N-1;

                while(i != 1)
                {
                    error = EBUSY;
                    while(error == EBUSY)
                        {
                        error = pthread_mutex_trylock(&lock);
                        }
                    if(error)
                    {
                        printf("Error to lock mutex \n");
                        exit(EXIT_FAILURE);
                    }
                    else // Enter the Critical section
                    {

                        memory[i] = EMPTY;
                        numPages = numPages - 1;
                    }
                    error = pthread_mutex_unlock(&lock);
                    if(error == EPERM)
                    {
                        printf("Error to tried unlock mutex\n");
                    }
                    if(i == (N-1))	//after clearing the first page
                        {
                        wakeup.info = ANY;	//only the Evicter can wake up the MMU
                        wakeup.source = ANY;
                        result = 0;
                        result = msgsnd(msqid[MMU], &wakeup, sizeof(wakeup), 0);
                        if(result !=0 )
                        {
                            perror("Error to wake up MMU\n");
                            exit(EXIT_FAILURE);
                        }
                        }
                    i = i-1;
                }
                switch_mode = SLEEP;	// Evicter finishe to clearing the pages and now going back to sleep !
                break;
        }
    }
}
