🧠 # Memory Management Simulator

## Overview
This repository contains a simulation of a **multi-process memory management system**.  
It models how processes request memory, handle page faults, evictions, and disk I/O operations in a simplified, controlled environment.

---

📚 Modules & Components

🧩 Processes (CPU)

- Two simulated processes run concurrently in loops.  
- In each iteration, a process:
  1. Waits for a fixed delay (e.g. `INTER_MEM_ACCS_T`)
  2. Randomly chooses between a **read** or **write** operation (based on probability `WR_RATE`)
  3. Sends a memory access request to the **Memory Management Unit (MMU)**
  4. Waits for a response
  5. Repeats

> Note: These processes do not handle actual data — only simulated read/write requests using virtual addresses.

---

💾 Memory Management Unit (MMU)

- Maintains an array of **N** pages (slots), each marked as “valid” or “invalid”.
- Runs three internal threads:
  - **Main Thread** — Handles incoming memory access requests, checks for hits/misses, and manages page allocation.
  - **Eviction Thread** — Activates when memory is full, evicting pages until usage drops below a threshold (FIFO–clock hybrid policy).
  - **Snapshot / Printer Thread** — Periodically records the memory state, pausing reads/writes during snapshots for data consistency.

---

💽 Hard Disk (HD)

- Handles read/write requests from the MMU.
- Each disk operation takes a fixed duration (e.g. `HD_ACCS_T`).
- After completion, it notifies the MMU that the operation has finished.

---

⏳ Simulation Lifecycle & Termination

- The simulation runs for a specified duration (`SIM_TIME`).
- On success, it prints:  
  ```
  Successfully finished sim
  ```
- On error, the simulation ensures:
  - All threads terminate safely
  - Mutexes and condition variables are destroyed
  - Dynamically allocated memory is properly released

---

✅ Correctness & Robustness

- All system calls (`fork`, `pthread_create`, `msgsnd`, etc.) include **error checking**.  
- Synchronization primitives (mutexes, condition variables) are **properly initialized and destroyed**.  
- Critical sections are **kept minimal** to reduce locking overhead.  
- Output is **concise**, showing only relevant logs or simulation summaries.

---

🛠 How to Build & Run

1. **Clone the repository**
   ```bash
   git clone https://github.com/roeysalah/memory-management-simulation.git
   cd memory-management-simulation
   ```

2. **Compile / Build**
   ```bash
   make
   ```

3. **Run the Simulation**
   ```bash
   ./simulate
   ```

4. **Customize Parameters**  
   You can edit `config.h` (or the configuration file) to change simulation settings:
   - `INTER_MEM_ACCS_T`
   - `WR_RATE`
   - `MEM_WR_T`
   - `HIT_RATE`
   - `HD_ACCS_T`
   - `SIM_TIME`
   - `USED_SLOTS_TH`
   - etc.

---

