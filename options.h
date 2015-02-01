#ifndef INCLUDE_OPTIONS_H
#define INCLUDE_OPTIONS_H

#include <stdint.h>

#endif

typedef enum _MODE
{
    MODE_ADD = 0,
    MODE_MODIFY,
    MODE_REMOVE,
} MODE;

typedef enum _COMMAND
{
    COMMAND_TAG = 0,
    COMMAND_ATTR,
} COMMAND;

typedef struct _OPTIONS
{
    int command;

    int mode;

    char tag_name[128];
    uint32_t deep;
    uint32_t count;
    char new_tag_name[128];

    uint32_t attr_type;
    char attr_name[128];
    char attr_value[128];
    uint32_t resource_id;

    char input_file[128];
    char output_file[128];
}OPTIONS;

void Usage(void);
