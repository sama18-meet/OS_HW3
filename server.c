#include "segel.h"
#include "request.h"
#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdbool.h>
//
// server.c: A very, very simple web server
//
// To run:
//  ./server <portnum (above 2000)>
//
// Repeatedly handles HTTP requests sent to this port number.
// Most of the work is done within routines written in request.c
//

enum SchedAlg {BLOCK, DT, DH, RANDOM};

// HW3: Parse the new arguments too
void getargs(int *port, int* threads, int* queue_size, enum SchedAlg* schedalg, int argc, char *argv[])
{
    if (argc != 5) {
        fprintf(stderr, "Usage: %s <port> <threads> <queue_size> <schedalg>\n", argv[0]);
        exit(1);
    }
    *port = atoi(argv[1]);
    *threads = atoi(argv[2]);
    *queue_size = atoi(argv[3]);
    if (*threads <= 0 || *queue_size <= 0) {
        fprintf(stderr, "threads and queue_size must be positive int\n");
        exit(1);
    }
    if (strcmp("block", argv[4]) == 0) { *schedalg = BLOCK; }
    else if (strcmp("dt", argv[4]) == 0) { *schedalg = DT; }
    else if (strcmp("dh", argv[4]) == 0) { *schedalg = DH; }
    else if (strcmp("random", argv[4]) == 0) { *schedalg = RANDOM; }
    else {
        fprintf(stderr, "schedalg must be one of block/dt/dh/random\n");
        exit(0);
    }
}

pthread_mutex_t queue_mutex;///no one change the req_mux and waiting
pthread_mutex_t active_queue_mutex;///////no one change the active queue

pthread_cond_t queue_not_empty;///send when not empty
pthread_cond_t queue_not_full;///send when not full
///////////////////////////////
int* waiting_connections_queue;
int* active_connections_list;
ClientRequest* requestsArray;
////////////////////////////////
int active_connections_counter;
int waiting_connections_counter;

void enqueueConnection(int connfd, int max_queue_size, struct timeval req_arrival_time) { // TODO: remove second arg and assert
    requestsArray[waiting_connections_counter].request_fd=connfd;
    requestsArray[waiting_connections_counter].request_arrival=req_arrival_time;
    waiting_connections_counter++;
}

void *startHandlerThread(void* args) {
    ClientRequest Curr;

    while (1) {
        pthread_mutex_lock(&queue_mutex);
        while (waiting_connections_counter == 0) {
            pthread_cond_wait(&queue_not_empty, &queue_mutex);
        }
        ////part3
        struct timeval nowDispatching;
        gettimeofday(&nowDispatching, NULL);
        ////////////////////////////////////////
        Curr= requestsArray[0];
        Curr.request_dispatch=nowDispatching;
        Curr.request_arrival=requestsArray[0].request_arrival;
        Curr.request_fd=requestsArray[0].request_fd;
        //////////////////////////////////////ss
        for (int i = 0; i < waiting_connections_counter - 1; i++) {
            requestsArray[i].request_fd=requestsArray[i+1].request_fd;
            requestsArray[i].request_arrival=requestsArray[i+1].request_arrival;
        }
        waiting_connections_counter--;
        pthread_cond_signal(&queue_not_full);
        pthread_mutex_unlock(&queue_mutex);

//////////////////////////////////////////////////////////////////////////
// POTENTIAL BUG:: Should active_connections and waiting connections have the same mutex?
        pthread_mutex_lock(&queue_mutex);
        active_connections_counter++;
        pthread_mutex_unlock(&queue_mutex);
/////////////////////////////////////////////////////////////////////////
        requestHandle(Curr.request_fd,Curr, (ThreadStats*)args);
        Close(Curr.request_fd);
///////////////////////////////////////////////////////////////////////////
        pthread_mutex_lock(&queue_mutex);
        active_connections_counter--;
        pthread_mutex_unlock(&queue_mutex);
        pthread_cond_signal(&queue_not_full); //brodcast or signal ? // and when?

    }
}

int main(int argc, char *argv[])
{
    int listenfd, connfd, clientlen;
    int port, threads, queue_size;
    enum SchedAlg schedalg;
    struct sockaddr_in clientaddr;

    getargs(&port, &threads, &queue_size, &schedalg, argc, argv);
    // init queue

    active_connections_list = malloc(sizeof(int) * queue_size);
    if (active_connections_list == NULL) {
        exit(1);
    }

    requestsArray = (ClientRequest*)malloc(queue_size * sizeof(ClientRequest)); /// for part3


    active_connections_counter = 0;
    waiting_connections_counter = 0;

///init all
    pthread_mutex_init(&queue_mutex, NULL);
    pthread_mutex_init(&active_queue_mutex,NULL);
    //////////////////////////////////////////////////////////////
    pthread_cond_init(&queue_not_empty, NULL);
    pthread_cond_init(&queue_not_full, NULL);

    // thread pool
    pthread_t* threadpool = malloc(sizeof(pthread_t)*threads);
    ThreadStats* threadStatsArr = malloc(sizeof(ThreadStats)*threads);
    if (threadpool == NULL || threadStatsArr == NULL) {
        exit(1);
    }
    for (int i=0; i<threads; ++i) {
        if (pthread_create(&threadpool[i], NULL, &startHandlerThread, (void*) &(threadStatsArr[i])) != 0) {
            perror("Failed to create thread");
            exit(1);
        }
        else {
            threadStatsArr[i].threadId = i;
            threadStatsArr[i].totalRequests = 0;
            threadStatsArr[i].staticRequests = 0;
            threadStatsArr[i].dynamicRequests = 0;
        }
    }

    bool we_listen=false;
    listenfd = Open_listenfd(port); // open and return a listening socket on port
    /////now we need to listen
    while (1) {
        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd, (SA *) &clientaddr, (socklen_t *) &clientlen);
        ////for part 3
        struct timeval now_arriving;
        gettimeofday(&now_arriving, NULL);


        ///////here starts the fun
        pthread_mutex_lock(&queue_mutex);//no one should change here but me,no other god here except me

        while (waiting_connections_counter + active_connections_counter >= queue_size) {

            if (schedalg == BLOCK) {
                // TODO: Change this to block instead of busy wait // done
                while (waiting_connections_counter + active_connections_counter >= queue_size)
                {
                    pthread_cond_wait(&queue_not_full, &queue_mutex); // wait till queue_not_full
                }
            }
            else if (schedalg == DT) { /// we drop this request
                close(connfd);
                we_listen=true;
                break;
            }
            else if (schedalg == DH) {
                if (waiting_connections_counter==0)
                {
                    close(connfd);
                    pthread_mutex_unlock(&queue_mutex);
                }
                waiting_connections_counter--;
                int oldestRequest = requestsArray[0].request_fd; //sama made sure that the oldest is always in 0 ?
                close(oldestRequest);
                for (int i = 0; i < waiting_connections_counter; ++i) {
                    requestsArray[i].request_fd = requestsArray[i + 1].request_fd;
                    requestsArray[i].request_arrival = requestsArray[i + 1].request_arrival;
                }
                pthread_mutex_unlock(&queue_mutex);

            } else if (schedalg == RANDOM) {

            }
        }

        if (we_listen==true)
        {
            we_listen = false;
            pthread_mutex_unlock(&queue_mutex);
            continue;
        }
        ////if no need for using our policy , simply insert


        enqueueConnection(connfd,queue_size,now_arriving);
        pthread_cond_signal(&queue_not_empty);
        pthread_mutex_unlock(&queue_mutex);
    }


    //
    // HW3: In general, don't handle the request in the main thread.
    // Save the relevant info in a buffer and have one of the worker threads
    // do the work.
    //



    for (int i=0; i<threads; ++i) {
        if (pthread_join(threadpool[i], NULL) != 0) {
            perror("Failed to join thread");
            exit(1);
        }
    }

    pthread_mutex_destroy(&queue_mutex);
    pthread_cond_destroy(&queue_not_empty);

    return 0;
}


    


 
