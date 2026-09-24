# SimOS

An operating system kernel simulator in C++. It models the parts of a kernel
that processes actually notice — creation and termination, who gets the CPU,
waiting on a disk, and where a page lives — and is written to the awkward
cases rather than the happy path.

## What it simulates

**Process lifecycle.** PIDs are allocated by increment and never reused. A new
process either takes an idle CPU or joins the back of the ready queue. `fork`
gives the child a fresh PID, queues it, and records the parent/child edge in
the process table.

**Termination, properly.** This is where the semantics get particular:

- A process that exits while its parent is *not* waiting becomes a **zombie**
  rather than disappearing — the parent still has a right to reap it.
- A parent already blocked in `wait` is resumed by any one zombie child; the
  remaining zombies stay put for that parent's next `wait`.
- Exit **cascades** to every descendant, so a terminating process never leaves
  orphans behind.

**Scheduling.** A timer interrupt preempts the running process and returns it
to the back of the ready queue, handing the CPU to whatever is next.

**Disk I/O.** Each disk keeps its own request queue and serves one read at a
time. The requesting process blocks until its read completes, then rejoins the
ready queue.

**Memory.** A page-to-frame table tracks which process owns which frame. Frames
are released the moment their process terminates, so the table never drifts out
of sync with the set of live processes.

## Layout

| File | Contents |
| --- | --- |
| `SimOS.h` | Public interface, with the pre/post conditions each call must hold |
| `SimOS.cpp` | Implementation — process table, ready queue, disk queues, frame table |
| `main.cpp` | Entry point |

## Building

```bash
g++ -std=c++17 -Wall -Wextra -o simos main.cpp SimOS.cpp
./simos
```

## Notes

Written for a systems course. The interesting part isn't the scheduler loop —
it's that a process exiting at the wrong moment has to leave a zombie, and a
parent exiting first must not leave an orphan. Those two rules drive most of
the bookkeeping in `SimOS.cpp`.
