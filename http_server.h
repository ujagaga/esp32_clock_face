#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

extern void HTTP_SERVER_process(void);
extern void HTTP_SERVER_init(void);
extern void HTTP_SERVER_start(void);
extern void HTTP_SERVER_stop(void);
extern bool HTTP_SERVER_isRunning(void);
extern bool HTTP_SERVER_clientConnected(void);   // best-effort active-browser check

#endif
