#include "mmu.h"
#include "ram.h"

#include <stddef.h>

static tPageTableEntry *g_page_table = NULL;

static uint8_t *get_ram_base()
{
    const tRam *ram = get_ram_state();
    if (!ram)
    {
        return NULL;
    }
    return (uint8_t *)ram;
}

void set_page_table(tPageTableEntry *page_table)
{
    g_page_table = page_table;
}

int get_physical_address(uint16_t virtual_address, uint16_t *physical_address)
{
    const tRam *ram = get_ram_state();

    if (physical_address == NULL)
    {
        return -3;
    }

    if (ram == NULL)
    {
        return -5;
    }

    if (g_page_table == NULL)
    {
        return -4;
    }

    uint16_t page_size   = ram->page_size;
    uint16_t page_index  = (uint16_t)(virtual_address / page_size);
    uint16_t page_offset = (uint16_t)(virtual_address % page_size);

    tPageTableEntry *entry = &g_page_table[page_index];

    if (entry->r == 0 && entry->w == 0 && entry->x == 0)
    {
        return -2;
    }

    if (entry->p_bit == 0)
    {
        return -1;
    }

    uint32_t phys32 = (uint32_t)entry->frame_id * (uint32_t)page_size + (uint32_t)page_offset;

    *physical_address = (uint16_t)phys32;
    return 0;
}

int fetch_instruction(uint16_t virtual_address, uint8_t *data)
{
    if (data == NULL)
    {
        return -3;
    }

    const tRam *ram = get_ram_state();
    if (!ram)
    {
        return -5;
    }

    if (!g_page_table)
    {
        return -4;
    }

    uint16_t page_size   = ram->page_size;
    uint16_t page_index  = (uint16_t)(virtual_address / page_size);
    uint16_t page_offset = (uint16_t)(virtual_address % page_size);

    tPageTableEntry *entry = &g_page_table[page_index];

    if (entry->r == 0 && entry->w == 0 && entry->x == 0)
    {
        return -2;
    }

    if (entry->x == 0)
    {
        return -3;
    }

    if (entry->p_bit == 0)
    {
        return -1;
    }

    uint32_t phys32 = (uint32_t)entry->frame_id * (uint32_t)page_size + (uint32_t)page_offset;

    uint8_t *base = get_ram_base();
    if (!base)
    {
        return -5;
    }

    *data = base[phys32];

    entry->r_bit = 1;
    return 0;
}

int load_data(uint16_t virtual_address, uint8_t *data)
{
    if (data == NULL)
    {
        return -3;
    }

    const tRam *ram = get_ram_state();
    if (!ram)
    {
        return -5;
    }

    if (!g_page_table)
    {
        return -4;
    }

    uint16_t page_size   = ram->page_size;
    uint16_t page_index  = (uint16_t)(virtual_address / page_size);
    uint16_t page_offset = (uint16_t)(virtual_address % page_size);

    tPageTableEntry *entry = &g_page_table[page_index];

    if (entry->r == 0 && entry->w == 0 && entry->x == 0)
    {
        return -2;
    }

    if (entry->r == 0)
    {
        return -3;
    }

    if (entry->p_bit == 0)
    {
        return -1;
    }

    uint32_t phys32 = (uint32_t)entry->frame_id * (uint32_t)page_size + (uint32_t)page_offset;

    uint8_t *base = get_ram_base();
    if (!base)
    {
        return -5;
    }

    *data = base[phys32];

    entry->r_bit = 1;
    return 0;
}

int store_data(uint16_t virtual_address, uint8_t data)
{
    const tRam *ram = get_ram_state();
    if (!ram)
    {
        return -5;
    }

    if (!g_page_table)
    {
        return -4;
    }

    uint16_t page_size   = ram->page_size;
    uint16_t page_index  = (uint16_t)(virtual_address / page_size);
    uint16_t page_offset = (uint16_t)(virtual_address % page_size);

    tPageTableEntry *entry = &g_page_table[page_index];

    if (entry->r == 0 && entry->w == 0 && entry->x == 0)
    {
        return -2;
    }

    if (entry->w == 0)
    {
        return -3;
    }

    if (entry->p_bit == 0)
    {
        return -1;
    }

    uint32_t phys32 = (uint32_t)entry->frame_id * (uint32_t)page_size + (uint32_t)page_offset;

    uint8_t *base = get_ram_base();
    if (!base)
    {
        return -5;
    }

    base[phys32] = data;

    entry->r_bit = 1;
    entry->m_bit = 1;
    return 0;
}
