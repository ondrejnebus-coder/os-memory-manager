#include "pager.h"
#include "task.h"
#include "ram.h"
#include "mmu.h"

#include <stddef.h>
#include <stdint.h>

int page_fault(int pid, uint16_t virtual_address)
{
    tTaskStruct *task = get_task_struct(pid);
    if (!task)
    {
        return -1;
    }

    const tRam *ram = get_ram_state();
    if (!ram)
    {
        return -3;
    }

    uint16_t page_size   = ram->page_size;
    uint16_t frames_total = (uint16_t)(ram->size / page_size);

    uint16_t page_index = (uint16_t)(virtual_address / page_size);
    if (page_index >= PAGE_TABLE_SIZE)
    {
        return -4;
    }

    uint8_t *ram_base = (uint8_t *)ram;
    uint8_t *addr_space = (uint8_t *)task->address_space;
    if (!addr_space)
    {
        return -3;
    }

    tPageTableEntry *fault_page = &task->page_table[page_index];

    if (fault_page->r == 0 && fault_page->w == 0 && fault_page->x == 0)
    {
        return -4;
    }

    if (fault_page->p_bit)
    {
        return -2;
    }

    int loaded_pages = 0;
    int victim_idx   = -1;
    int victim_class = 5;
    uint16_t victim_frame = 0;

    for (uint16_t i = 0; i < PAGE_TABLE_SIZE; ++i)
    {
        tPageTableEntry *e = &task->page_table[i];

        if (e->p_bit)
        {
            ++loaded_pages;

            int cls = (e->r_bit ? 2 : 0) + (e->m_bit ? 1 : 0);

            if (cls < victim_class)
            {
                victim_class = cls;
                victim_idx   = (int)i;
                victim_frame = e->frame_id;
            }

            if (e->m_bit && e->frame_id < frames_total)
            {
                uint32_t frame_start = (uint32_t)e->frame_id * (uint32_t)page_size;
                uint8_t *src = ram_base + frame_start;
                uint8_t *dst = addr_space + (uint32_t)i * (uint32_t)page_size;

                for (uint16_t b = 0; b < page_size; ++b)
                {
                    dst[b] = src[b];
                }
            }
        }

        e->r_bit = 0;
        e->m_bit = 0;
    }

    uint16_t frame_id = 0;
    int can_use_new_frame = (task->max_frames == 0 || loaded_pages < task->max_frames);
    int falloc_res = -1;

    if (can_use_new_frame)
    {
        falloc_res = falloc(&frame_id, 1);
    }

    if (falloc_res != 0)
    {
        if (victim_idx < 0)
        {
            return -3;
        }

        frame_id = victim_frame;

        tPageTableEntry *vict = &task->page_table[victim_idx];
        vict->p_bit   = 0;
        vict->frame_id = 0;
    }

    if (frame_id >= frames_total)
    {
        return -3;
    }

    uint32_t frame_start = (uint32_t)frame_id * (uint32_t)page_size;
    uint8_t *dst = ram_base + frame_start;
    uint8_t *src = addr_space + (uint32_t)page_index * (uint32_t)page_size;

    for (uint16_t b = 0; b < page_size; ++b)
    {
        dst[b] = src[b];
    }

    fault_page->frame_id = frame_id;
    fault_page->p_bit    = 1;

    return 0;
}
