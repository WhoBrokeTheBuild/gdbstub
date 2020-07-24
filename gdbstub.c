#include "gdbstub.h"

#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>
#include <stdbool.h>
#include <string.h>

#if defined(WIN32)
#include <Windows.h>
#define close closesocket
#else
#include <unistd.h>
#endif

#define GDBSTUB_BUFFER_LENGTH 4096
#define GDBSTUB_PACKET_LENGTH 2048

typedef enum
{
    GDB_STATE_NO_PACKET,
    GDB_STATE_IN_PACKET,
    GDB_STATE_IN_CHECKSUM,

} gdbstate_t;

struct gdbstub
{
    gdbstub_config_t config;

    int server;
    int client;

    uint8_t buffer[GDBSTUB_BUFFER_LENGTH];
    ssize_t buffer_length;

    gdbstate_t state;
    uint8_t packet[GDBSTUB_BUFFER_LENGTH];
    ssize_t packet_length;
    uint8_t packet_checksum;

    uint8_t checksum[2];
    ssize_t checksum_length;
};

void _gdbstub_recv(gdbstub_t * gdb);

void _gdbstub_send(gdbstub_t * gdb, const char * data);

void _gdbstub_process_packet(gdbstub_t * gdb);

gdbstub_t * gdbstub_init(gdbstub_config_t config)
{
    gdbstub_t * gdb = (gdbstub_t *)malloc(sizeof(gdbstub_t));
    if (!gdb) {
        fprintf(stderr, "out of memory\n");
        return NULL;
    }

    gdb->config = config;
    gdb->server = -1;
    gdb->client = -1;
    gdb->state = GDB_STATE_NO_PACKET;
    gdb->packet_length = 0;
    gdb->packet_checksum = 0;
    gdb->checksum_length = 0;

    int result;
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(gdb->config.port);
    addr.sin_addr.s_addr = INADDR_ANY;

    gdb->server = socket(AF_INET, SOCK_STREAM, 0);
    if (gdb->server < 0) {
        perror("socket failed");
        free(gdb);
        return NULL;
    }

    result = fcntl(gdb->server, F_SETFL, fcntl(gdb->server, F_GETFL, 0) | O_NONBLOCK);
    if (result < 0) {
        perror("fcntl O_NONBLOCK failed");
        free(gdb);
        return NULL;
    }

    int reuse = 1;
    result = setsockopt(gdb->server, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse));
    if (result < 0) {
        perror("setsockopt SO_REUSEADDR failed");
        free(gdb);
        return NULL;
    }

#ifdef SO_REUSEPORT
    result = setsockopt(gdb->server, SOL_SOCKET, SO_REUSEPORT, (const char*)&reuse, sizeof(reuse));
    if (result < 0) {
        perror("setsockopt SO_REUSEPORT failed");
        free(gdb);
        return NULL;
    }
#endif

    result = bind(gdb->server, (struct sockaddr *)&addr, sizeof(addr));
    if (result < 0) {
        perror("bind failed");
        free(gdb);
        return NULL;
    }

    result = listen(gdb->server, 1);
    if (result < 0) {
        perror("listen failed");
        free(gdb);
        return NULL;
    }

    printf("listening for gdb on port %hu\n", gdb->config.port);
    return gdb;
}

void gdbstub_term(gdbstub_t * gdb)
{
    if (gdb) {
        if (gdb->client >= 0) {
            close(gdb->client);
            gdb->client = -1;
        }

        if (gdb->server >= 0) {
            close(gdb->server);
            gdb->server = -1;
        }

        free(gdb);
    }
}

void gdbstub_tick(gdbstub_t * gdb)
{
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);

    if (gdb->client < 0) {
        gdb->client = accept(gdb->server, (struct sockaddr *)&addr, &addrlen);
        if (gdb->client >= 0) {
            printf("accepted gdb connection\n");
            fcntl(gdb->client, F_SETFL, fcntl(gdb->client, F_GETFL, 0) | O_NONBLOCK);
        }
    }
    else {
        _gdbstub_recv(gdb);
    }
}

void _gdbstub_send(gdbstub_t * gdb, const char * data)
{
    int length = strlen(data);
    uint8_t checksum = 0;
    for (int i = 0; i < length; ++i) {
        checksum += data[i];
    }
    
    gdb->packet_length = snprintf((char *)gdb->packet, GDBSTUB_PACKET_LENGTH, 
        "$%s#%02x", data, checksum);

    printf("Sending %s\n", gdb->packet);

    int bytes = send(gdb->client, gdb->packet, gdb->packet_length, 0);
    printf("sent %d bytes\n", bytes);
    if (bytes < 0) {
        perror("lost gdb connection");
        close(gdb->client);
        gdb->client = -1;
        gdb->state = GDB_STATE_NO_PACKET;
    }
}

void _gdbstub_recv(gdbstub_t * gdb)
{
    gdb->buffer_length = recv(gdb->client, gdb->buffer, GDBSTUB_BUFFER_LENGTH, 0);
    if (gdb->buffer_length < 0 && errno != EAGAIN) {
        perror("lost gdb connection");
        close(gdb->client);
        gdb->client = -1;
        gdb->state = GDB_STATE_NO_PACKET;
        return;
    }

    for (int i = 0; i < gdb->buffer_length; ++i) {
        char c = gdb->buffer[i];

        switch (gdb->state)
        {
        case GDB_STATE_NO_PACKET:
            if (c == '$') {
                gdb->state = GDB_STATE_IN_PACKET;
                gdb->packet_length = 0;
                gdb->packet_checksum = 0;
            }
            else if (c == 3) {
                // break
            }
            break;
        case GDB_STATE_IN_PACKET:
            if (c == '#') {
                gdb->state = GDB_STATE_IN_CHECKSUM;
                gdb->checksum_length = 0;
            }
            else {
                gdb->packet[gdb->packet_length++] = c;
                gdb->packet_checksum += c;
            }
            break;
        case GDB_STATE_IN_CHECKSUM:
            gdb->checksum[gdb->checksum_length++] = c;
            if (gdb->checksum_length == 2) {
                int checksum;
                sscanf((char *)gdb->checksum, "%2x", &checksum);
                if (gdb->packet_checksum != checksum) {
                    // freak out?
                }

                send(gdb->client, "+", 1, 0);

                gdb->packet[gdb->packet_length] = '\0';
                _gdbstub_process_packet(gdb);

                gdb->state = GDB_STATE_NO_PACKET;
            }
        }
    }
}

void _gdbstub_process_packet(gdbstub_t * gdb)
{
    printf("Processing %s\n", gdb->packet);
    switch (gdb->packet[0])
    {
    case 'c':
        if (gdb->config.start) {
            gdb->config.start(gdb->config.user_data);
        }
        return;
    case 'D':
        _gdbstub_send(gdb, "OK");
        close(gdb->client);
        gdb->client = -1;
        gdb->state = GDB_STATE_NO_PACKET;
        return;
    case 'g':
        if (gdb->config.get_registers) {
            gdb->buffer_length = gdb->config.get_registers(gdb->config.user_data, gdb->buffer, GDBSTUB_BUFFER_LENGTH);
            _gdbstub_send(gdb, (char *)gdb->buffer);
        }
        return;
    case 'H':
        _gdbstub_send(gdb, "OK");
        return;
    case 'p':
        _gdbstub_send(gdb, "");
        return;
    case 'q':
        // qSupported - We support no features
        if (strncmp((char *)gdb->packet, "qSupported", 10) == 0) {
            _gdbstub_send(gdb, "PacketSize=1024");
            return;
        }
        // qC - We have no thread ID
        else if (gdb->packet[1] == 'C') {
            _gdbstub_send(gdb, "QC00");
            return;
        }
        // qAttached - We created a new process
        else if (strncmp((char *)gdb->packet, "qAttached", 9) == 0) {
            _gdbstub_send(gdb, "1");
            return;
        }
        // qTStatus - There is no trace running
        else if (strncmp((char *)gdb->packet, "qTStatus", 8) == 0) {
            _gdbstub_send(gdb, "T0");
            return;
        }
        // 
        else if (strncmp((char *)gdb->packet, "qTfP", 4) == 0) {
            _gdbstub_send(gdb, "");
            return;
        }
        else if (strncmp((char *)gdb->packet, "qTfV", 4) == 0) {
            _gdbstub_send(gdb, "");
            return;
        }
        else if (strncmp((char *)gdb->packet, "qTsP", 4) == 0) {
            _gdbstub_send(gdb, "");
            return;
        }
        else if (strncmp((char *)gdb->packet, "qfThreadInfo", 12) == 0) {
            _gdbstub_send(gdb, "lm0");
            return;
        }
        break;
    case 's':
        if (gdb->config.step) {
            gdb->config.step(gdb->config.user_data);
        }
        return;
    case 'v':
        _gdbstub_send(gdb, "");
        return;
    case 'z':
        if (gdb->config.clear_breakpoint) {
            uint32_t address;
            sscanf((char *)gdb->packet, "z0,%x", &address);
        }
        _gdbstub_send(gdb, "OK");
        break;
    case 'Z':
        if (gdb->config.set_breakpoint) {
            uint32_t address;
            sscanf((char *)gdb->packet, "Z0,%x", &address);
        }
        _gdbstub_send(gdb, "OK");
        break;
    case '?':
        if (gdb->config.stop) {
            gdb->config.stop(gdb->config.user_data);
        }
        _gdbstub_send(gdb, "S00");
        return;
    }

    fprintf(stderr, "unknown gdb command '%s'\n", gdb->packet);
}
