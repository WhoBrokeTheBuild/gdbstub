#ifndef GDBSTUB_H
#define GDBSTUB_H

#include <stdint.h>
#include <sys/types.h>

typedef struct gdbstub_config gdbstub_config_t;

typedef void (*gdbstub_start_t)(void *);
typedef void (*gdbstub_stop_t)(void *);
typedef void (*gdbstub_step_t)(void *);
typedef void (*gdbstub_set_breakpoint_t)(void *, uint32_t);
typedef void (*gdbstub_clear_breakpoint_t)(void *, uint32_t);
typedef ssize_t (*gdbstub_get_memory_t)(void *, uint8_t *, size_t, uint32_t, size_t);
typedef ssize_t (*gdbstub_get_registers_t)(void *, uint8_t *, size_t);

struct gdbstub_config
{
    uint16_t port;

    void * user_data;

    gdbstub_start_t start;

    gdbstub_stop_t stop;

    gdbstub_step_t step;

    gdbstub_set_breakpoint_t set_breakpoint;

    gdbstub_clear_breakpoint_t clear_breakpoint;

    gdbstub_get_memory_t get_memory;

    gdbstub_get_registers_t get_registers;
};

typedef struct gdbstub gdbstub_t;

gdbstub_t * gdbstub_init(gdbstub_config_t config);

void gdbstub_term(gdbstub_t * gdb);

void gdbstub_tick(gdbstub_t * gdb);

#endif // GDBSTUB_H