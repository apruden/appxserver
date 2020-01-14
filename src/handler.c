#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <syslog.h>
#include "awsr.h"
#include "handler.h"
#include "http_parser.h"
#include "url_trie.h"
#include "fields_trie.h"

void *resources[50];
url_trie trie;
fields_trie ftrie;
int trie_nkeys;

__thread char *url;
__thread char *body;
__thread char **fields;
__thread int current_field;
__thread int method;
__thread int keep_alive;
__thread struct http_parser parser;

/* */
http_parser_settings settings = { 
    .on_message_begin = message_begin_cb,
    .on_url = request_url_cb,
    .on_header_field = header_field_cb,
    .on_header_value = header_value_cb,
    .on_headers_complete = headers_complete_cb,
    .on_body = body_cb,
    .on_message_complete = message_complete_cb 
};

/* */
int message_begin_cb (http_parser *p)
{
    if(fields == NULL)
    {
        fields = (char **)calloc(30, sizeof(char *));

        int i;

        for(i = 0; i<30; i++)
        {
            fields[i] = (char*) malloc (512 * sizeof(char));
        }
    }

    if(url == NULL)
    {
        url = (char *) malloc(512 * sizeof(char));
    }

    return 0;
}

/* */
int request_url_cb (http_parser *p, const char *buf, size_t len)
{
    //syslog(LOG_INFO, "Parsing url %s %d", buf, len);
    memcpy(url, buf, len);
    url[len] = '\0';

    return 0;
}

/* */
int header_field_cb (http_parser *p, const char *buf, size_t len)   
{
    int idx = fields_trie_find(&ftrie, buf, len);

    if(idx > 0 )
    {
        current_field = idx;
    }

    return 0;
}

/* */
int header_value_cb (http_parser *p, const char *buf, size_t len)
{
    if (current_field > 0 )
    {
        memcpy(fields[current_field], buf, len);
        fields[current_field][len] = '\0';
    }

    current_field = 0;

    return 0;
}

/* */
int headers_complete_cb(http_parser *p)
{
    return 0;
}

/* */
int body_cb (http_parser *p, const char *buf, size_t len)
{
    if (body != NULL)
    {
        free(body);
    }

    body = (char *)malloc((len + 1) * sizeof(char));
    memcpy(body, buf, len);
    body[len] = '\0';

    return 0;
}

/* */
int message_complete_cb(http_parser *p)
{
    switch(p->method)
    {
        case HTTP_GET:
            method = 0; 
            break;
        case HTTP_POST:
            method = 1;
            break;
        default:
            method =  0;
    }

    return 0;
}

/* */
void handler_init()
{
    fields_trie_init(&ftrie);
    url_trie_init(&trie);
}

/* */
void handler_register_action(const char* url, void* action )
{
    url_trie_insert(&trie, url, trie_nkeys);
    resources[trie_nkeys - 1] = (int (*)(awsr_connection*))action;
    trie_nkeys++;
}

/* */
void handler_not_found(awsr_connection* conn)
{
    char *test_response = (char *)calloc(250 , sizeof(char));
    strcpy(test_response ,"HTTP/1.1 401 Not Found\r\n"
            "Connection: close\r\n"
            "Content-Type: text/html; charset=utf-8\r\n\r\n"
            "<html><head><title>Error 401</title></head><body>Resource not found.</body></html>\n");
    conn->response = test_response;
    conn->response_size = 250;
}


/* */
void handler_server_error(awsr_connection* conn)
{
    char *test_response = (char *)calloc(250 , sizeof(char));
    strcpy(test_response ,"HTTP/1.1 500 Server Error\r\n"
            "Connection: close\r\n"
            "Content-Type: text/html; charset=utf-8\r\n\r\n"
            "<html><head><title>Error 500</title></head><body>Server Error.</body></html>\n");
    conn->response = test_response;
    conn->response_size = 250;
}


/* */
void* handler_handle_request(void *arg)
{
    //TODO:
    // * comet: each new connection should store the fd on a thread safe map (channed id -> fd)
    //    * parse url. a special url will be used for commet, e.g. /channel/123
    //    * POST to channel will write to the fd and to db
    //    * GET to channel will read from db and return or will wait for new messages
    //syslog(LOG_INFO, "Handling request.");

    awsr_event_data *edata = (awsr_event_data *) arg;
    awsr_connection *conn = (awsr_connection *) calloc(1, sizeof(awsr_connection));
    conn->file_fd = 0;
    conn->file_size = 0;
    conn->request = edata->buf;
    conn->request_size = edata->buf_size;
    //syslog(LOG_INFO, "Handling request %s.", conn->request);
    http_parser_init(&parser, HTTP_REQUEST);
    size_t nparsed = http_parser_execute(&parser, &settings, conn->request, conn->request_size);
    conn->url = url;
    conn->method = method;
    conn->fields = fields;
    conn->keep_alive = 0;
    conn->body = body;

    //syslog(LOG_INFO, "Query parsed %s.", url);
    char *url_params[10];
    int i;

    for(i = 0; i < 10; i++)
    {
        url_params[i] = NULL;
    }

    int idx = url_trie_find(&trie, url, url_params); 

    if (idx > 0)
    {
        //syslog(LOG_INFO, "Executing action.");
        int result = ((int (*)(awsr_connection*)) resources[idx - 1])(conn);
        //TODO: create response in case of error.
    }
    else
    {
        //syslog(LOG_INFO, "Action not found.");
        handler_not_found(conn);
    }

    //syslog(LOG_INFO, "Executed action.");
    edata->file_fd = conn->file_fd;
    edata->file_size = conn->file_size;
    edata->file_offset = 0;
    edata->buf = conn->response;
    edata->buf_size = conn->response_size; 
    struct epoll_event event;
    event.data.ptr = edata;
    event.events = EPOLLOUT | EPOLLET | EPOLLONESHOT;

    if (epoll_ctl (edata->efd, EPOLL_CTL_MOD, edata->fd, &event) == -1)
    {
        //syslog(LOG_INFO, "Error epoll_ctl.");
        perror ("handle_request: epoll_ctl");
        //abort ();
    }

    for(i = 0; i < 10; i++)
    {
        if(url_params[i] != NULL){
            free(url_params[i]);
        }
    }

    if(conn->request != NULL)
    {
        free(conn->request);
    }

    free(conn);
}

/* */
void handler_close()
{
    if(url != NULL)
    {
        free(url);
    }

    if(body != NULL)
    {
        free(body);
    }

    free(fields);
}
