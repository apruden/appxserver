#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/sendfile.h>
#include <errno.h>
#include <pthread.h>
#include <syslog.h>
#include "thpool.h"
#include "awsr.h"

#define MAXEVENTS 1024
#define CHUNK_SIZE 2048

int efd, sfd;
thpool *threadpool;
void* (*awsr_handler)(awsr_event_data* data);

/* */
typedef struct epoll_event epoll_event;

/* */
void awsr_init(void* (*handler)(awsr_event_data*))
{
    awsr_handler = handler;

    return;
}

/* */
static int make_socket_non_blocking (int sfd)
{
    int flags;
    flags = fcntl (sfd, F_GETFL, 0);

    if (flags == -1)
    {
        perror ("fcntl");
        return -1;
    }

    flags |= O_NONBLOCK;

    if (fcntl (sfd, F_SETFL, flags) == -1)
    {
        perror ("fcntl");
        return -1;
    }

    return 0;
}

/* */
static int create_and_bind (const char *port)
{
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    int sfd;

    memset (&hints, 0, sizeof (struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if (getaddrinfo (NULL, port, &hints, &result) != 0)
    {
        return -1;
    }

    for (rp = result; rp != NULL; rp = rp->ai_next)
    {
        sfd = socket (rp->ai_family, rp->ai_socktype, rp->ai_protocol);

        if (sfd == -1)
        {
            continue;
        }

        if (bind (sfd, rp->ai_addr, rp->ai_addrlen) == 0)
        {
            break;
        }

        close (sfd);
    }

    if (rp == NULL)
    {
        return -1;
    }

    freeaddrinfo (result);

    //syslog(LOG_INFO, "Socket created.");

    return sfd;
}

/* */
void* awsr_loop(void* arg)
{
    int n, i;
    epoll_event *events = (epoll_event *) malloc(sizeof(epoll_event) * MAXEVENTS);

    /* The event loop */
    while (1)
    {
        n = epoll_wait (efd, events, MAXEVENTS, -1);

        for (i = 0; i < n; i++)
        {
            if ((events[i].events & EPOLLERR) || (events[i].events & EPOLLHUP))
            {
                fprintf (stderr, "epoll error [%i]\n", events[i].data.fd);
                close (events[i].data.fd);
                continue;
            }
            else if (sfd == events[i].data.fd)
            {
                int infd;
                struct sockaddr in_addr;
                socklen_t in_len = sizeof(in_addr);

                while (1)
                {
                    infd = accept (sfd, &in_addr, &in_len);

                    if (infd == -1)
                    {
                        if ((errno == EAGAIN) ||
                                (errno == EWOULDBLOCK))
                        {
                            break;
                        }
                        else
                        {
                            perror ("accept");
                            break;
                        }
                    }

                    if (make_socket_non_blocking (infd) == -1)
                    {
                        perror("make_socket_non_blocking");
                        abort();
                    }

                    epoll_event event;
                    event.data.fd = infd;
                    event.events = EPOLLIN | EPOLLET | EPOLLONESHOT;

                    if (epoll_ctl(efd, EPOLL_CTL_ADD, infd, &event) == -1)
                    {
                        perror("epoll_ctl");
                        abort();
                    }
                }

                continue;
            }
            else if (events[i].events & EPOLLIN)
            {
                //syslog(LOG_INFO, "Reading from socket.");

                int done = 0;
                ssize_t count, total_read = 0;
                char* buf = (char *) malloc(2048 * sizeof(char));

                while (1)
                {
                    count = read(events[i].data.fd, buf, 2048);

                    if (count == -1)
                    {
                        if (errno != EAGAIN)
                        {
                            perror ("read");
                            done = 1;
                        }

                        break;
                    }
                    else if (count == 0)
                    {
                        done = 1;
                        break;
                    }

                    total_read += count;
                }

                if (done)
                {
                    close (events[i].data.fd);
                }

                //TODO: create connection and send to thread pool
                awsr_event_data *evdata = (awsr_event_data*) malloc(sizeof(awsr_event_data));
                evdata->fd = events[i].data.fd;
                evdata->efd = efd;
                evdata->buf = buf;
                evdata->buf_size = total_read;

                thpool_add_work(threadpool,(void *)awsr_handler, (void*)evdata);
            }
            else if(events[i].events & EPOLLOUT)
            {
                //syslog(LOG_INFO, "Writing response.");

                //TODO:
                // * when sending file, send a chunk of data and rearm epoll event

                awsr_event_data *evdata = (awsr_event_data*)events[i].data.ptr;


                if(evdata->buf_size > 0 )
                {
                    int resp_size = strlen(evdata->buf);

                    if(write(evdata->fd, evdata->buf, resp_size) == -1)
                    {
                        perror ("write");
                        abort ();
                    }
                }

                if(evdata->buf != NULL)
                {
                    free(evdata->buf);
                    evdata->buf = NULL;
                    evdata->buf_size = 0;
                }

                //sendfile
                if(evdata->file_fd != 0)
                {
                    //syslog(LOG_INFO, "Sending file chunk.");

                    off_t offset = (off_t)evdata->file_offset;
                    int rc = sendfile(evdata->fd, evdata->file_fd, NULL, CHUNK_SIZE);

                    if(rc == -1)
                    {
                    }

                    int new_offset = evdata->file_offset + rc;

                    if(new_offset < evdata->file_size)
                    {
                        //more to send
                        epoll_event event;
                        event.data.fd = events[i].data.fd;
                        evdata->file_offset = evdata->file_offset + CHUNK_SIZE;
                        event.data.ptr = evdata;
                        event.events = EPOLLOUT | EPOLLET | EPOLLONESHOT;

                        if (epoll_ctl(efd, EPOLL_CTL_MOD, evdata->fd, &event) == -1)
                        {
                            perror("epoll_ctl");
                            abort();
                        }

                        continue;
                    }
                    else 
                    {
                        //syslog(LOG_INFO, "Sending file end.");

                        char* end_buf = "\r\n";

                        if (write(evdata->fd, end_buf, 2) == -1)
                        {

                        }

                        //close opened file
                        close(evdata->file_fd);
                    }
                }
                else
                {
                    syslog(LOG_INFO, "No file in response.");
                }

                //syslog(LOG_INFO, "Closing socket.");

                //close socket
                close(evdata->fd);
                free(evdata);
            }
        }
    }

    free(events);
}


/* */
int awsr_run (const char* port)
{
    printf("awsr run\n");

    int s;
    threadpool = thpool_init(20);
    sfd = create_and_bind (port);

    if (sfd == -1)
    {
        abort ();
    }

    if (make_socket_non_blocking (sfd) == -1)
    {
        perror("make_socked_non_blocking");
        abort ();
    }

    if (listen (sfd, SOMAXCONN) == -1)
    {
        perror ("listen");
        abort ();
    }

    /* Prepare epoll */
    efd = epoll_create1 (0);

    if (efd == -1)
    {
        perror ("epoll_create");
        abort ();
    }

    epoll_event event;
    event.data.fd = sfd;
    event.events = EPOLLIN | EPOLLET;

    if (epoll_ctl (efd, EPOLL_CTL_ADD, sfd, &event) == -1)
    {
        perror ("epoll_ctl");
        abort ();
    }

    fprintf(stderr, "init loop thread\n");
    int LOOP_THREADS = 1, i=0;
    pthread_t threads[LOOP_THREADS];

    for (i=0; i<LOOP_THREADS; i++) {
        fprintf(stderr, "starting loop thread\n");
        pthread_create(&threads[i], NULL, &awsr_loop, NULL);
    }

    for (i=0; i<LOOP_THREADS; i++) {
        fprintf(stderr, "joinin loop thread\n");
        pthread_join(threads[i], NULL);
    }

    close(sfd);

    return 0;
}

/* */
void awsr_stop()
{
    close(sfd);
}
