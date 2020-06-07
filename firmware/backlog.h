#ifndef _BACKLOG_H
#define _BACKLOG_H

#include <stdint.h>
#include <stdbool.h>

#define BACKLOG_BUF_SIZE 12

typedef struct
{
    uint32_t buffer[12];
    uint8_t pos;
} circular_buf_t;

void circular_buf_init(circular_buf_t *c);
void circular_buf_append(circular_buf_t *c, uint32_t entry);
void circular_buf_reset(circular_buf_t *c, uint32_t entry);
bool circular_buf_all_lower_than(circular_buf_t *c, uint32_t threshold);
bool circular_buf_any_lower_than(circular_buf_t *c, uint32_t threshold);

#endif
