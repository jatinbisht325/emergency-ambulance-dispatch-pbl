# Emergency Ambulance Dispatch & Fleet Optimization System
## Team Technical Briefing & API Contracts (Graphic Era Hill University, Bhimtal)

**Team Leader:** Jatin (Core Engine, Graph System, Master Integration)  
**Team Members:** Kartik (Min-Heap & Dispatch), Mansi (Fleet, Hospitals & File I/O), Mahima (Call Queue/Stack & UI)  
**Timeline:** 5-Day Delivery Sprint  

---

### 1. System Data Flow Pipeline

```
1. Emergency Call Arrives (112/999)
   └── Mahima: Enqueues into FIFO CallQueue (fairness / first-come first-served)
       
2. Dispatch Triggered
   ├── Mansi: Filters list of ambulances where status == STATUS_AVAILABLE
   ├── Jatin: Runs Dijkstra to find shortest road distances from each available vehicle to patient node
   └── Kartik: Inserts (ambulance_id, distance) into MinHeap, calls extract_min() in O(1)
       
3. Route Execution & State Updates
   ├── Jatin: Traces turn-by-turn route (Base -> Patient -> Hospital) using parent[] array
   ├── Kartik: Transitions vehicle status: AVAILABLE -> EN_ROUTE -> AT_SCENE -> RETURNING -> AVAILABLE
   ├── Mansi: Deducts hospital bed/ICU capacity and persists updated fleet to disk (.txt)
   └── Mahima: Pushes completed incident record onto LIFO CallHistoryStack for supervisor audit
```

---

### 2. Responsibilities & Assigned Header Files

#### A. JATIN (Team Leader) — `graph.h` & `graph.c`
* **Status:** **COMPLETE & TESTED**
* **Delivered Files:**
  * [`common.h`](file:///c:/Users/jatin/OneDrive/New%20folder/data%20structure%20pbl/common.h): Global types, enums (`AmbulanceStatus`, `PriorityLevel`, `RouteResult`).
  * [`graph.h`](file:///c:/Users/jatin/OneDrive/New%20folder/data%20structure%20pbl/graph.h): Adjacency list declarations and Dijkstra routing engine.
  * [`graph.c`](file:///c:/Users/jatin/OneDrive/New%20folder/data%20structure%20pbl/graph.c): Full implementation of sparse city graph, file loader (`city_map.txt`), and Dijkstra with predecessor path reconstruction.
  * [`test_graph.c`](file:///c:/Users/jatin/OneDrive/New%20folder/data%20structure%20pbl/test_graph.c): Standalone test driver running realistic emergency scenarios.
* **Viva Focus:** Why Adjacency List over Matrix? $O(V + E)$ vs $O(V^2)$. Why Dijkstra over BFS? Accounts for road weights/delays.

---

#### B. KARTIK — `dispatch_heap.h` & `dispatch_heap.c`
* **Contract:** [`dispatch_heap.h`](file:///c:/Users/jatin/OneDrive/New%20folder/data%20structure%20pbl/dispatch_heap.h)
* **What Kartik Needs to Code:**
  1. `MinHeap` struct backed by an array `HeapNode elements[MAX_NODES]`.
  2. `heapify_up(MinHeap* heap, int index)`: Restores heap property after adding an ambulance ($O(\log N)$).
  3. `heapify_down(MinHeap* heap, int index)`: Sifts down after extracting root ($O(\log N)$).
  4. `heap_insert()` and `heap_extract_min()`: Returns the closest ambulance in $O(1)$ root lookup.
  5. State transition function: `transition_ambulance_status()`.
* **Viva Question:** *"What happens when an emergency call comes in but all ambulances are busy?"*  
  *Answer:* The heap of available vehicles is empty; the call remains safely queued in Mahima's FIFO Queue until any ambulance returns to `STATUS_AVAILABLE`.

---

#### C. MANSI — `fleet_hospital.h` & `fleet_hospital.c`
* **Contract:** [`fleet_hospital.h`](file:///c:/Users/jatin/OneDrive/New%20folder/data%20structure%20pbl/fleet_hospital.h)
* **What Mansi Needs to Code:**
  1. Dynamic Singly Linked List of `Ambulance` records (supports $O(1)$ head insertion).
  2. `find_ambulance(int id)`: Traverses list defensively, returning `NULL` if not found (preventing segfaults).
  3. Hospital capacity tracker: `admit_patient_to_hospital(int hospital_id, bool requires_icu)` (updates available regular and ICU beds).
  4. Flat-File Persistence: `load_fleet_from_file()` and `save_fleet_to_file()` using `fopen`, `fprintf`, `fscanf`.
* **Viva Question:** *"Why use a Linked List instead of a static array for the fleet?"*  
  *Answer:* Fleets are dynamic; vehicles are commissioned, decommissioned, or sent for servicing. A linked list has no rigid array size limit and avoids memory shifting.

---

#### D. MAHIMA — `call_system.h` & `call_system.c`
* **Contract:** [`call_system.h`](file:///c:/Users/jatin/OneDrive/New%20folder/data%20structure%20pbl/call_system.h)
* **What Mahima Needs to Code:**
  1. FIFO Queue (`front`, `rear`, `count`): Strict first-in, first-out emergency triage.
  2. LIFO Stack (`top`, `count`): Last-in, first-out history of resolved incidents.
  3. `enqueue_call()` and `dequeue_call()`.
  4. `push_history()` and `pop_history()`.
  5. Terminal Dashboard: Displays pending calls, dispatched vehicles, and turn-by-turn routes clearly.
* **Viva Question:** *"Why use a Queue for incoming calls and a Stack for history?"*  
  *Answer:* Incoming calls must be dispatched fairly in order of receipt (FIFO). Historical reviews require supervisors to immediately inspect the *most recently resolved* incident first (LIFO).

---

### 3. Compilation & Build Command

To compile Jatin's core engine independently:
```bash
gcc -Wall -Wextra -std=c99 -O2 graph.c test_graph.c -o test_graph.exe
./test_graph.exe
```

When all 4 members complete their `.c` files, the master build command will be:
```bash
gcc -Wall -Wextra -std=c99 -O2 graph.c dispatch_heap.c fleet_hospital.c call_system.c main.c -o ambulance_system.exe
```
