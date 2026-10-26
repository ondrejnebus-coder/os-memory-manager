# OS Memory Manager

A small paging-based memory manager written in C. The project implements the core
pieces of a virtual-memory subsystem: a bitmap frame allocator, an MMU with page-table
permission checks, an NRU page-replacement policy with dirty-page write-back, and a
task manager for up to eight tasks.

This was built as an Operating Systems course project at FIIT STU in Bratislava.

## Implemented components

### RAM and frame allocation (`src/ram.c`)

- Initializes a RAM region divided into equally sized frames.
- Keeps a usage bitmap inside the managed RAM itself.
- `falloc` uses a first-fit search for consecutive free frames.
- `ffree` releases a range of frames.
- Validates power-of-two sizes and page sizes, and refuses uninitialized memory.

### MMU (`src/mmu.c`)

- Installs a page table with `set_page_table`.
- Translates virtual addresses to physical frame offsets.
- Checks read, write and execute permission bits.
- Distinguishes page faults, segmentation faults, null output pointers and a missing
  page table.
- Updates the referenced and modified bits used by the pager.

### Pager (`src/pager.c`)

- Implements a local NRU (Not Recently Used) replacement policy.
- Evicts a page when a task has no free frame left.
- Writes modified pages back through the task address-space callback.
- Respects the per-task frame limit.

### Task manager (`src/task.c`)

- Creates and destroys tasks and assigns process IDs.
- Stores each task's page table and address-space handle in RAM.
- Releases all frames owned by a task when it is destroyed.
- Exposes the task table and per-PID task records.

## Project layout

```text
src/
  types.h        page-table entry and shared types
  ram.c/.h       RAM model, bitmap allocator
  mmu.c/.h       address translation and permissions
  pager.c/.h     NRU page replacement
  task.c/.h      task manager
Makefile
```

## Build

Requirements: GCC and `make` on Linux/macOS. The code is C11-compatible.

```bash
make
```

The shared library is written to `build/libospager.so`.

On Windows, a direct GCC command can be used instead:

```powershell
gcc -shared -fPIC -Wall -Wextra -Werror -O2 -Isrc `
  -Dmalloc=__forbidden_malloc -Dcalloc=__forbidden_calloc `
  -Drealloc=__forbidden_realloc -Dfree=__forbidden_free `
  src/*.c -o build/libospager.dll
```

The original course test executable is not included. The repository exposes the
library API declared in the headers, so it can be linked against a custom test
harness.

## API summary

| Header | Functions |
|---|---|
| `ram.h` | `init_ram`, `destroy_ram`, `falloc`, `ffree`, `get_ram_state` |
| `mmu.h` | `set_page_table`, `get_physical_address`, `fetch_instruction`, `load_data`, `store_data` |
| `pager.h` | `page_fault` |
| `task.h` | `init_taskMgr`, `destroy_taskMgr`, `create_task`, `destroy_task`, `get_task_mgr`, `get_task_struct` |

## Author

Ondrej Nebus - FIIT STU, Bratislava.
