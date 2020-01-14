#ifndef HANDLER_H
#define HANDLER_H
#ifdef __cplusplus
extern "C" {
#endif

#include "http_parser.h"
#include "awsr.h"

/* */
void handler_init();

/* */
void handler_register_action(const char* url, void *action);

/* */
void* handler_handle_request(void *arg);
 
/* */
void handler_close();

/* */
int message_begin_cb (http_parser *p);

/* */
int request_url_cb (http_parser *p, const char *buf, size_t len);

/* */
int header_field_cb (http_parser *p, const char *buf, size_t len);  

/* */
int header_value_cb (http_parser *p, const char *buf, size_t len);

/* */
int headers_complete_cb(http_parser *p);

/* */
int body_cb (http_parser *p, const char *buf, size_t len);

/* */
int message_complete_cb(http_parser *p);


#ifdef __cplusplus
}
#endif
#endif
