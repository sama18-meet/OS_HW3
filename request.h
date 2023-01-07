#ifndef __REQUEST_H__



typedef struct clientRequest{
    int request_fd;
   // int current_index;
    struct timeval request_arrival;
    struct timeval request_dispatch;
} *ClientRequest;

void requestHandle(int fd, ClientRequest Curr);
#endif
