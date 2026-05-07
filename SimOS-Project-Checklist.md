# SimOS project — ordered checklist

Use this in order. Check items off as you go. Requirements match **Home Project guide.pdf** (exact API names and behavior).

---

## 0. Before you code

- [ ] Read the full PDF again: due date, **no `main()` in submission**, **`SimOS.h`** name, **`g++ -std=c++20 *.cpp -o runme`**, no debug prints when submitting.
- [ ] Plan on paper: one `SimOS` class; **no** `using namespace std` in headers; **no** methods implemented in `.h` (only declarations); use **`std::vector` / `std::deque`** etc., **no** C-style arrays; **no** raw `new`/`delete` for your core structures.
- [ ] Remember: **simulated** RAM can be huge — track **frames/pages**, not one byte per simulated byte.

---

## 1. Project skeleton

- [ ] Create **`SimOS.h`**: paste the given `FileReadRequest`, `MemoryItem`, `MemoryUsage`, `NO_PROCESS`; declare class **`SimOS`** with **every** method from the spec (signatures must match exactly).
- [ ] Add your name in a comment at the **top of every** `.h` and `.cpp` file.
- [ ] Implement in one or more `.cpp` files (e.g. `SimOS.cpp`); keep **`main()`** only in a **local** test file you do **not** submit.

---

## 2. Internal model (design once, then implement)

- [ ] **PID**: next PID starts at **1**, increments forever; **never** reuse after terminate.
- [ ] **Per-process state** (conceptually): parent PID, children list, **zombie** flag or zombie queue, **memory**: which logical pages it “owns” (or page→PID mapping when resident), etc.
- [ ] **CPU**: single current PID or “idle” → `GetCPU()` returns `NO_PROCESS` when idle.
- [ ] **Ready queue**: `std::deque<int>`, **true FIFO**; **never** include the process currently on the CPU in `GetReadyQueue()`.
- [ ] **Time slice**: pick a constant quantum for round-robin; on `TimerInterrupt()`, running process goes to **back** of ready queue (unless spec edge cases you handle in fork/wait/disk — CPU rules still consistent with RR).
- [ ] **Memory**: `amountOfRAM` and `pageSize` → number of **frames** = `amountOfRAM / pageSize` (integer relationship as your sim assumes). Frames numbered **0 .. frames-1**. Pages numbered from **0** per process (logical address → page = `address / pageSize`).
- [ ] **LRU**: on eviction, remove **least recently used** frame; on **hit**, update “recently used” order; on **load** into multiple free frames, choose **lowest frame number** (per spec).
- [ ] **Disks**: one **FIFO queue** per disk + optional “currently serving” request; disk indices **0 .. numberOfDisks-1**.

---

## 3. Constructor `SimOS(numberOfDisks, amountOfRAM, pageSize)`

- [x] Store disk count, frame count (from RAM/page size), page size.
- [x] Initialize empty ready queue, idle CPU, empty memory book-keeping, empty disk structures.
- [x] Validate assumptions you need (e.g. `pageSize > 0`, RAM multiple of page size if your course expects that — PDF doesn’t spell it; align with lab/tests).

---

## 4. `NewProcess()`

- [x] Allocate new PID (increment from last).
- [x] Add process to your process table with parent = none or 0 per your design.
- [x] If CPU idle → run **on CPU**; else → **back** of ready queue.

---

## 5. Memory: `AccessMemoryAddress(address)`

- [ ] Running process only (caller must enforce CPU non-idle — see section 10).
- [ ] Compute **page number** for this process from `address` and `pageSize`.
- [ ] If page already in a frame → **update LRU**; done.
- [ ] If not resident: if a **free** frame exists, pick **lowest-numbered** free frame; else **evict LRU** frame (and if that frame held another process’s page, that mapping is removed).
- [ ] Map page → frame for **current** PID.

---

## 6. `GetMemory()`

- [ ] Return `MemoryUsage`: one `MemoryItem` per **occupied** frame, sorted by **increasing `frameNumber`**.
- [ ] **Do not** include frames for **zombie** processes (zombies use no memory).
- [ ] Only **terminated** zombies matter for this rule; living processes with pages must appear.

---

## 7. CPU scheduling helpers

- [ ] Implement “dispatch”: if CPU idle and ready queue non-empty, pop **front** of ready queue to CPU.
- [ ] `TimerInterrupt()`: if someone runs, move them to **back** of ready queue, then dispatch next.
- [ ] Keep **round-robin** consistent with “time limit per burst” description (slice end = timer interrupt).

---

## 8. `SimFork()`

- [ ] Child gets **new** PID (increment).
- [ ] Record parent/child relationship.
- [ ] Child goes to **end** of **ready** queue (not on CPU unless your rules say otherwise — spec: child **placed at end of ready-queue**).
- [ ] Decide memory model for fork: typically child shares or copies **address space** per your instructor’s expectation; PDF doesn’t detail fork memory — match **test driver** / lecture (often child starts with same logical pages, copy-on-write or duplicate mappings — clarify with course staff if tests fail).

---

## 9. `SimExit()` — hardest section

- [ ] **Free all RAM** for the exiting process **immediately** (remove its pages from frames; frames become free; update LRU structures).
- [ ] **Cascading termination**: when this process exits, **all descendants** must terminate too (recursive or multi-step); released memory for **all** of them; handle zombies/parent links so no orphans.
- [ ] If **parent is blocked in `wait`** already → child (or zombie handling) **immediately** unblocks parent: parent becomes runnable (**ready queue end** or **CPU** per `SimWait` rules).
- [ ] If parent has **not** called `wait` → exiting child becomes **zombie** (no memory).
- [ ] If exiting process is **on CPU**, CPU becomes idle or you dispatch; if it was only in ready queue, remove it there too (shouldn’t happen for `SimExit` — only **running** process exits).
- [ ] Clean up children links so `wait` and cascade stay consistent.

---

## 10. `SimWait()`

- [ ] Running process **waits for any child** to finish.
- [ ] If **at least one zombie child** exists → **stay on CPU** (spec: “proceeds right away”), **reap one** zombie (any), leave other zombies for later `wait`s.
- [ ] If **no** zombie yet → parent **blocks** (not on ready queue, not on CPU); when a child later exits and makes parent runnable, parent goes to **end of ready queue** **or** straight to CPU per spec wording — implement exactly as PDF: *“Once the wait is over, the process goes to the end of the ready-queue or the CPU.”*
- [ ] Coordinate with `SimExit` so the **first** terminating child can wake parent if parent is waiting.

---

## 11. Disk I/O

- [ ] `DiskReadRequest(diskNumber, fileName)`: **running** process enqueues request on that disk; process **stops using CPU immediately** (go to waiting-for-disk state, **not** in ready queue); dispatch next if any.
- [ ] `DiskJobCompleted(diskNumber)`: complete **one** job (FIFO); that process → **back** of **ready** queue; if disk has more, start next (you track “current” vs queue).
- [ ] `GetDisk(d)`: if busy → PID + `fileName` of **current** job; if idle → default `{0, ""}`.
- [ ] `GetDiskQueue(d)`: `deque` of **waiting** jobs in order **next to be served** first; **exclude** the currently served job.
- [ ] Invalid `diskNumber` → **`std::out_of_range`**.

---

## 12. Exception rules (global)

- [ ] Any call that **requires a running process** while **`GetCPU()` would be `NO_PROCESS`** → throw **`std::logic_error`**.

---

## 13. Getter sanity

- [ ] `GetCPU()` — idle → `NO_PROCESS`.
- [ ] `GetReadyQueue()` — front = head; **no** current CPU PID inside.
- [ ] `GetMemory()` — sorted by frame number; no zombie memory.
- [ ] Disk getters — match PDF defaults and queue semantics.

---

## 14. Local testing (before submit)

- [ ] Build with: `g++ -std=c++20 *.cpp -o runme` on Linux lab (or matching env).
- [ ] Test: RR + timer, fork/exit/wait, zombie + immediate wait, cascade exit, memory fill + LRU + low-frame allocation, disk FIFO + CPU release on request + completion.
- [ ] **Remove or disable all debug `cout`/prints** in files you submit.
- [ ] Submit **without** your `main()` test file if instructions say library only; confirm GradeScope file list with course.

---

## 15. Final submission pass

- [ ] No `main()` in submitted sources.
- [ ] No extra namespaces; header named **`SimOS.h`**.
- [ ] No prohibited patterns from PDF (globals, `goto`, etc.).
- [ ] Zip/rules: **no** archive if PDF says don’t submit archives — follow course upload steps exactly.

---

*If fork memory semantics or wait/runnable details are ambiguous, use the PDF wording and public tests; ask the instructor for edge cases.*
