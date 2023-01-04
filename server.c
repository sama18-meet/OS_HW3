#include "segel.h"
#include "request.h"
#include <assert.h>

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

pthread_mutex_t queue_mutex;
pthread_cond_t queue_not_empty;

int* waiting_connections_queue;
int* active_connections_list;
int active_connections_counter;
int waiting_connections_counter;


int dequeueConnection() {
    int connfd;

    pthread_mutex_lock(&queue_mutex);
    while (waiting_connections_counter == 0) {
        pthread_cond_wait(&queue_not_empty, &queue_mutex);
    }

    connfd = waiting_connections_queue[0];
    for (int i = 0; i < waiting_connections_counter - 1; i++) {
        waiting_connections_queue[i] = waiting_connections_queue[i + 1];
    }
    waiting_connections_counter--;
    active_connections_list[active_connections_counter++] = connfd;
    pthread_mutex_unlock(&queue_mutex);
    return connfd;
}

void enqueueConnection(int connfd, int max_queue_size) { // TODO: remove second arg and assert
    pthread_mutex_lock(&queue_mutex);
    waiting_connections_queue[waiting_connections_counter++] = connfd;
    pthread_cond_signal(&queue_not_empty);
    pthread_mutex_unlock(&queue_mutex);
    assert(waiting_connections_counter+active_connections_counter <= max_queue_size);
}

void* startHandlerThread(void* args) {
    while (1) {
        int connfd = dequeueConnection();
	requestHandle(connfd);
	Close(connfd);
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
    waiting_connections_queue = malloc(sizeof(int)*queue_size);
    if (waiting_connections_queue == NULL) {
        exit(1);
    }
    active_connections_list = malloc(sizeof(int)*queue_size);
    if (active_connections_list == NULL) {
        exit(1);
    }
    active_connections_counter = 0;
    waiting_connections_counter = 0;

    pthread_mutex_init(&queue_mutex, NULL);
    pthread_cond_init(&queue_not_empty, NULL);

    // thread pool
    pthread_t* threadpool = malloc(sizeof(pthread_t)*threads);
    if (threadpool == NULL) {
        exit(1);
    }
    for (int i=0; i<threads; ++i) {
        if (pthread_create(&threadpool[i], NULL, &startHandlerThread, NULL) != 0) {
            perror("Failed to create thread");
            exit(1);
        }
    }
    listenfd = Open_listenfd(port); // open and return a listening socket on port
    if (schedalg == BLOCK) {
        while (1) {
            if (active_connections_counter < threads) { // TODO: Change this to block instead of busy wait
	        clientlen = sizeof(clientaddr);
	        connfd = Accept(listenfd, (SA *)&clientaddr, (socklen_t *) &clientlen);
                enqueueConnection(connfd, queue_size);
            }

	    // 
	    // HW3: In general, don't handle the request in the main thread.
	    // Save the relevant info in a buffer and have one of the worker threads 
	    // do the work. 
	    // 
        }
    }

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


    


 
