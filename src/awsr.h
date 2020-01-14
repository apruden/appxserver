#ifndef awsr_h
#define awsr_h
#ifdef __cplusplus
extern "C" {
#endif

/* */
typedef struct awsr_event_data
{
    int efd; // epoll fd
    int fd; // socket fd
    int file_fd; //file fd for sendfile
    int file_offset;
    int file_size;
    int buf_size;
    char *buf;
} awsr_event_data;

/* */
typedef struct awsr_connection 
{
    int url_size;
    int method;
    int request_type;
    int request_size;
    int body_size;
    int file_fd;
    int file_size;
    int response_size;
    int response_status;
    int keep_alive;
    char **fields;
    char *url;
    char *request;
    char *response;
    char *body;
} awsr_connection;

/* */
void awsr_init(void* (*handler)(awsr_event_data*));

/* */
int awsr_run (const char* port);

void awsr_stop();

#ifdef __cplusplus
}
#endif
#endif
