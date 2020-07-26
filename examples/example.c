#define GDBSTUB_IMPLEMENTATION
#define GDBSTUB_DEBUG
#include <gdbstub.h>

#include <stdbool.h>
#include <stdio.h>

typedef struct context
{
    // whatever

} context_t;

const char TARGET_CONFIG[] = 
"<?xml version=\"1.0\"?>"
"<!DOCTYPE feature SYSTEM \"gdb-target.dtd\">"
"<target version=\"1.0\">"
"</target>";

const char MEMORY_MAP[] = 
"<?xml version=\"1.0\"?>"
"<memory-map>"
"</memory-map>";

void gdb_connected(context_t * ctx)
{
    printf("Connected\n");
}

void gdb_disconnected(context_t * ctx)
{
    printf("Disconnected\n");
}

void gdb_start(context_t * ctx)
{
    printf("Starting\n");
}

void gdb_stop(context_t * ctx)
{
    printf("Stopping\n");
}

void gdb_step(context_t * ctx)
{
    printf("Stepping\n");
}

void gdb_set_breakpoint(context_t * ctx, uint32_t address)
{
    printf("Set breakpoint %08X\n", address);
}

void gdb_clear_breakpoint(context_t * ctx, uint32_t address)
{
    printf("Clear breakpoint %08X\n", address);
}

ssize_t gdb_get_memory(context_t * ctx, char * buffer, size_t buffer_length, uint32_t address, size_t length)
{
    printf("Getting memory %08X, %08lX\n", address, length);
    return snprintf(buffer, buffer_length, "00000000");
}

ssize_t gdb_get_register_value(context_t * ctx, char * buffer, size_t buffer_length, int reg)
{
    printf("Getting register value #%d\n", reg);
    return snprintf(buffer, buffer_length, "00000000");
}

ssize_t gdb_get_general_registers(context_t * ctx, char * buffer, size_t buffer_length)
{
    printf("Getting general registers\n");
    return snprintf(buffer, buffer_length, "00000000");
}

int main(int argc, char ** argv)
{
    context_t ctx;

    gdbstub_config_t config;
    config.port = 5678;
    config.user_data = &ctx;
    config.connected = (gdbstub_connected_t)gdb_connected;
    config.disconnected = (gdbstub_disconnected_t)gdb_disconnected;
    config.start = (gdbstub_start_t)gdb_start;
    config.stop = (gdbstub_stop_t)gdb_stop;
    config.step = (gdbstub_step_t)gdb_step;
    config.set_breakpoint = (gdbstub_set_breakpoint_t)gdb_set_breakpoint;
    config.clear_breakpoint = (gdbstub_clear_breakpoint_t)gdb_clear_breakpoint;
    config.get_memory = (gdbstub_get_memory_t)gdb_get_memory;
    config.get_register_value = (gdbstub_get_register_value_t)gdb_get_register_value;
    config.get_general_registers = (gdbstub_get_general_registers_t)gdb_get_general_registers;
    config.target_config = TARGET_CONFIG;
    config.target_config_length = sizeof(TARGET_CONFIG);
    config.memory_map = MEMORY_MAP;
    config.memory_map_length = sizeof(MEMORY_MAP);

    gdbstub_t * gdb = gdbstub_init(config);
    if (!gdb) {
        fprintf(stderr, "failed to create gdbstub\n");
        return 1;
    }

    bool running = true;
    while (running)
    {
        // Do not call more than a couple times per second
        gdbstub_tick(gdb);
    }

    gdbstub_term(gdb);

    return 0;
}