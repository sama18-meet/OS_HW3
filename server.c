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


int main(int argc, char *argv[])
{
    int listenfd, connfd, clientlen;
    int port, threads, queue_size;
    enum SchedAlg schedalg;
    struct sockaddr_in clientaddr;

    getargs(&port, &threads, &queue_size, &schedalg, argc, argv);

    // 
    // HW3: Create some threads...
    //

    listenfd = Open_listenfd(port);
    while (1) {
	clientlen = sizeof(clientaddr);
	connfd = Accept(listenfd, (SA *)&clientaddr, (socklen_t *) &clientlen);

	// 
	// HW3: In general, don't handle the request in the main thread.
	// Save the relevant info in a buffer and have one of the worker threads 
	// do the work. 
	// 
	requestHandle(connfd);

	Close(connfd);
    }

}


    


 
