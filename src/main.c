#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <syslog.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
//#include "leveldb/db.h"
//#include "leveldb/c.h"
#include "awsr.h"
#include "handler.h"

int app_index(awsr_connection* conn)
{

}

int app_static(awsr_connection* conn)
{
    syslog(LOG_INFO, "Serving static.");

    char* base = "/home/alex/static/";
    char* name = "toto.jpg";
    char filename[255];
    strcpy(filename, base);
    strcat(filename, name);

    //TODO:
    // * change socket fd to TCP_CORK
    // * write response fields to fd.
    // * open file
    // * sendfile

    int static_fd = open(filename, O_RDONLY);

    if (static_fd == -1)
    {
        syslog(LOG_INFO, "Unable to open file.");
    }

    /* get the size of the file to be sent */
    struct stat stat_buf;
    fstat(static_fd, &stat_buf);

    char *test_response = (char *)calloc(250 , sizeof(char));
    strcpy(test_response ,"HTTP/1.1 200 OK\r\n"
            "Date: Tue, 18 Dec 2012 03:08:47 GMT\r\n"
            "Server: Apache\r\n"
            "Last-Modified: Mon, 09 Mar 1998 21:56:49 GMT\r\n"
            "ETag: \"273f2386-4fb1-328fa57bbea40\"\r\n"
            "Accept-Ranges: bytes\r\n");

    char buffer[16];
    snprintf(buffer, sizeof(buffer), "%d", stat_buf.st_size);

    strcat(test_response, "Content-Length: ");
    strcat(test_response, buffer);
    strcat(test_response, "\r\n");
    strcat(test_response, "Content-Type: image/jpeg\r\n\r\n");
    conn->response = test_response;
    conn->response_size = 250;

    /* copy file using sendfile */
    conn->file_fd = static_fd;
    conn->file_size = stat_buf.st_size;

    syslog(LOG_INFO, "Serving static response send to epoll.");
}

int app_hello(awsr_connection* conn)
{
    char *test_response = (char *)calloc(250 , sizeof(char));
    strcpy(test_response ,"HTTP/1.1 200 OK\r\n"
            "Connection: close\r\n"
            "Content-Type: text/html; charset=utf-8\r\n\r\n"
            "<html><head><title>hi</title></head><body>hello world!</body></html>\n");
    conn->response = test_response;
    conn->response_size = 250;

    return 0;
}

int app_blog(awsr_connection* conn)
{
    //char *response = (char *)malloc(2048 * sizeof(char));
    //strcpy(response, output.c_str());
    //conn->response = response;

    return 0;
}

/* */
void my_sig_handler (int a) 
{
    syslog(LOG_INFO, "Interrupt handled.");
    handler_close();
    awsr_stop();
    closelog();
    exit(0);
}

/* */
int main(int argc, char* argv[])
{
    if(argc < 2)
    {
        exit(0);
    }

    signal (SIGINT,  my_sig_handler);

    /*    leveldb_options_t options;
          options.create_if_missing = true;
          leveldb_status_t status = leveldb_db_open(
          options, "/home/alex/tmp/test-leveldb", &db);
          assert(status.ok());
          */
    openlog("appxserver", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
    syslog(LOG_INFO, "Server started ...");

    handler_init();
    handler_register_action("/image", (void *)&app_static);
    handler_register_action("/hello", (void *)&app_hello);
    handler_register_action("/articles/?/?/?", (void *)&app_blog);
    awsr_init((void* (*)(awsr_event_data*))&handler_handle_request);
    awsr_run(argv[1]);

    return 0;
}
