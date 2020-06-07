#include "backlog.h"

void circular_buf_init(circular_buf_t *c)
{
    c->pos = 0;
    for(uint8_t i=0; i < BACKLOG_BUF_SIZE; i++)
    {
        c->buffer[i] = 0;
    }
}

void circular_buf_append(circular_buf_t *c, uint32_t entry)
{
    c->buffer[c->pos] = entry;
    c->pos += 1;
    if(c->pos >= BACKLOG_BUF_SIZE) c->pos = 0;
}

void circular_buf_reset(circular_buf_t *c, uint32_t entry)
{
    for(uint8_t i=0; i < BACKLOG_BUF_SIZE; i++)
    {
        c->buffer[i] = entry;
    }
}

bool circular_buf_all_lower_than(circular_buf_t *c, uint32_t threshold)
{
    for(uint8_t i=0; i < BACKLOG_BUF_SIZE; i++)
    {
        // We don't consider the last entry that was added for this, since it may
        // already contain samples collected after PTT clearing.
        if(c->buffer[i] >= threshold && i != (c->pos -1) % BACKLOG_BUF_SIZE) return false;
    }
    return true;
}

bool circular_buf_any_lower_than(circular_buf_t *c, uint32_t threshold)
{
    for(uint8_t i=0; i < BACKLOG_BUF_SIZE; i++)
    {
        if(c->buffer[i] < threshold) return true;
    }
    return false;
}
