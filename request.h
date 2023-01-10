#ifndef __REQUEST_H__

typedef struct threadStats {
    int threadId;
    int totalRequests;
    int staticRequests;
    int dynamicRequests;
} ThreadStats;

typedef struct clientRequest{
    int request_fd;
   // int current_index;
    struct timeval request_arrival;
    struct timeval request_dispatch;
} ClientRequest;

void requestHandle(int fd, ClientRequest Curr, ThreadStats* threadStats);
#endif
