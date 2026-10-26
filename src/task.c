#include "task.h"
#include "ram.h"

#include <stddef.h>
#include <string.h>

static tTaskMgr *g_task_mgr = NULL;
static uint16_t g_task_mgr_frame_id = 0;
static uint16_t g_task_mgr_frames = 0;
static int g_next_pid = 1;

static uint8_t *get_ram_base()
{
    const tRam *ram = get_ram_state();
    if (!ram)
    {
        return NULL;
    }
    return (uint8_t *)ram;
}

int init_taskMgr()
{
    const tRam *ram = get_ram_state();
    if (!ram)
    {
        return -1;
    }

    if (g_task_mgr != NULL)
    {
        return 0;
    }

    uint16_t page_size = ram->page_size;
    uint16_t frames_total = (uint16_t)(ram->size / ram->page_size);

    (void)frames_total;

    uint32_t bytes_needed = (uint32_t)sizeof(tTaskMgr);

    uint16_t frames_needed = (uint16_t)((bytes_needed + page_size - 1u) / page_size);

    uint16_t frame_id = 0;
    int res = falloc(&frame_id, frames_needed);
    if (res != 0)
    {
        return -1;
    }

    uint8_t *base = get_ram_base();
    if (!base)
    {
        ffree(frame_id, frames_needed);
        return -1;
    }

    tTaskMgr *mgr = (tTaskMgr *)(base + (uint32_t)frame_id * (uint32_t)page_size);

    memset(mgr, 0, sizeof(tTaskMgr));

    for (int i = 0; i < TASK_TABLE_SIZE; ++i)
    {
        mgr->tasks[i].pid = -1;
    }

    g_task_mgr = mgr;
    g_task_mgr_frame_id = frame_id;
    g_task_mgr_frames = frames_needed;
    g_next_pid = 1;

    return 0;
}

void destroy_taskMgr()
{
    if (!g_task_mgr)
    {
        return;
    }

    ffree(g_task_mgr_frame_id, g_task_mgr_frames);

    g_task_mgr = NULL;
    g_task_mgr_frame_id = 0;
    g_task_mgr_frames = 0;
    g_next_pid = 1;
}

static tTaskStruct *find_task(int pid)
{
    if (!g_task_mgr)
    {
        return NULL;
    }

    for (int i = 0; i < TASK_TABLE_SIZE; ++i)
    {
        if (g_task_mgr->tasks[i].pid == pid)
        {
            return &g_task_mgr->tasks[i];
        }
    }
    return NULL;
}

static tTaskStruct *find_free_task_slot()
{
    if (!g_task_mgr)
    {
        return NULL;
    }

    for (int i = 0; i < TASK_TABLE_SIZE; ++i)
    {
        if (g_task_mgr->tasks[i].pid == -1)
        {
            return &g_task_mgr->tasks[i];
        }
    }
    return NULL;
}

static int generate_pid()
{
    int start = g_next_pid;
    if (start < 1)
        start = 1;

    int pid = start;
    while (find_task(pid) != NULL)
    {
        ++pid;
        if (pid == start)
        {
            return -1;
        }
    }

    g_next_pid = pid + 1;
    return pid;
}

int create_task(const tPageTableEntry *page_table, uint8_t max_frames, void *address_space)
{
    if (!g_task_mgr || !get_ram_state())
    {
        return -3;
    }

    if (page_table == NULL || address_space == NULL)
    {
        return -2;
    }

    tTaskStruct *slot = find_free_task_slot();
    if (!slot)
    {
        return -1;
    }

    int pid = generate_pid();
    if (pid < 0)
    {
        return -1;
    }

    slot->pid = pid;
    slot->max_frames = max_frames;
    slot->address_space = address_space;

    memcpy(slot->page_table, page_table, sizeof(slot->page_table));

    return pid;
}

int destroy_task(int pid)
{
    tTaskStruct *task = find_task(pid);
    if (!task)
    {
        return -1;
    }

    memset(task->page_table, 0, sizeof(task->page_table));
    task->address_space = NULL;
    task->max_frames = 0;
    task->pid = -1;

    return 0;
}

const tTaskMgr *get_task_mgr()
{
    return g_task_mgr;
}

tTaskStruct *get_task_struct(int pid)
{
    return find_task(pid);
}
