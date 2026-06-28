#ifndef TCP_SERVER_H
#define TCP_SERVER_H

// JSON-over-TCP API server (port TCP_API_PORT). One JSON object per line in,
// one JSON object per line out. Mirrors the HTTP /api/* actions and can also
// start/stop the HTTP server to save CPU.
extern void TCP_init(void);
extern void TCP_process(void);

#endif
