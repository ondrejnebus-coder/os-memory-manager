#include "ram.h"
#include <stddef.h>
#include <string.h>

static tRam *g_ram = NULL;

static int is_power_of_two_u16(uint16_t x)
{
    return (x != 0) && ((x & (x - 1)) == 0);
}

static int is_power_of_two_u8(uint8_t x)
{
    return (x != 0) && ((x & (x - 1)) == 0);
}

static int bitmap_get(const uint8_t *bitmap, uint16_t index)
{
    uint16_t byte = index / 8;
    uint8_t bit   = (uint8_t)(index % 8);
    return (bitmap[byte] >> bit) & 0x1u;
}

static void bitmap_set(uint8_t *bitmap, uint16_t index)
{
    uint16_t byte = index / 8;
    uint8_t bit   = (uint8_t)(index % 8);
    bitmap[byte] |= (uint8_t)(1u << bit);
}

static void bitmap_clear(uint8_t *bitmap, uint16_t index)
{
    uint16_t byte = index / 8;
    uint8_t bit   = (uint8_t)(index % 8);
    bitmap[byte] &= (uint8_t)~(1u << bit);
}

int init_ram(void *memory, uint16_t size, uint8_t page_size)
{
    if (!is_power_of_two_u16(size) || size == 0)
    {
        return -1;
    }

    if (!is_power_of_two_u8(page_size) || page_size == 0 || page_size > size)
    {
        return -2;
    }

    if (memory == NULL)
    {
        return -3;
    }

    uint8_t *mem8 = (uint8_t *)memory;
    for (uint16_t i = 0; i < size; ++i)
    {
        if (mem8[i] != 0)
        {
            return -3;
        }
    }

    uint16_t frames_total = (uint16_t)(size / page_size);
    if (frames_total == 0)
    {
        return -4;
    }

    uint16_t bitmap_bytes = (uint16_t)((frames_total + 7u) / 8u);

    if ((uint32_t)sizeof(tRam) + (uint32_t)bitmap_bytes > (uint32_t)size)
    {
        return -4;
    }

    tRam *ram = (tRam *)mem8;

    uint8_t *bitmap = mem8 + sizeof(tRam);

    ram->size      = size;
    ram->page_size = page_size;
    ram->bitmap    = bitmap;

    memset(bitmap, 0, bitmap_bytes);

    uint32_t meta_bytes = (uint32_t)sizeof(tRam) + (uint32_t)bitmap_bytes;
    uint16_t meta_frames = (uint16_t)((meta_bytes + page_size - 1u) / page_size);

    for (uint16_t f = 0; f < meta_frames && f < frames_total; ++f)
    {
        bitmap_set(bitmap, f);
    }

    g_ram = ram;

    return frames_total;
}

void destroy_ram()
{
    if (g_ram == NULL)
    {
        return;
    }

    uint8_t *mem8 = (uint8_t *)g_ram;
    uint16_t size = g_ram->size;

    for (uint16_t i = 0; i < size; ++i)
    {
        mem8[i] = 0;
    }

    g_ram = NULL;
}

int falloc(uint16_t *frame_id, uint16_t number)
{
    if (g_ram == NULL)
    {
        return -1;
    }

    if (frame_id == NULL || number == 0)
    {
        return -2;
    }

    uint16_t frames_total = (uint16_t)(g_ram->size / g_ram->page_size);
    uint8_t *bitmap       = g_ram->bitmap;

    uint16_t run_start = 0;
    uint16_t run_len   = 0;
    int found          = 0;

    for (uint16_t i = 0; i < frames_total; ++i)
    {
        if (bitmap_get(bitmap, i) == 0)
        {
            if (run_len == 0)
            {
                run_start = i;
            }
            ++run_len;

            if (run_len == number)
            {
                found = 1;
                break;
            }
        }
        else
        {
            run_len = 0;
        }
    }

    if (!found)
    {
        return -1;
    }

    for (uint16_t i = 0; i < number; ++i)
    {
        bitmap_set(bitmap, (uint16_t)(run_start + i));
    }

    *frame_id = run_start;
    return 0;
}

void ffree(uint16_t frame_id, uint16_t number)
{
    if (g_ram == NULL)
    {
        return;
    }

    if (number == 0)
    {
        return;
    }

    uint16_t frames_total = (uint16_t)(g_ram->size / g_ram->page_size);
    uint8_t *bitmap       = g_ram->bitmap;

    if (frame_id >= frames_total)
    {
        return;
    }

    uint32_t end = (uint32_t)frame_id + (uint32_t)number;
    if (end > frames_total)
    {
        end = frames_total;
    }

    for (uint16_t i = frame_id; i < (uint16_t)end; ++i)
    {
        bitmap_clear(bitmap, i);
    }
}

const tRam *get_ram_state()
{
    return g_ram;
}
