# Emergency Ambulance Dispatch & Fleet Optimization System 🚑

**Academic Project (PBL):** Data Structures in C  
**Institution:** Graphic Era Hill University (GEHU), Bhimtal  
**Team Leader:** Jatin (Core Engine, Graph Routing & System Integration)  
**Team Members:** Kartik, Mansi, Mahima  

---

## 📌 Project Overview
An automated decision-support emergency dispatch system simulating a 999/112 city-wide emergency center. The system identifies the nearest available ambulance, calculates the fastest driving route through a weighted city road network, monitors hospital bed and ICU capacities, and logs incidents in real-time.

### Chosen Data Structures & Algorithmic Complexities
| Component / Challenge | Data Structure Solution | Time / Space Efficiency | Assigned Member |
| :--- | :--- | :--- | :--- |
| **City Road Network & Routing** | Graph (Adjacency List) + Dijkstra's Algorithm | $O(V + E)$ space, $O(V^2)$ / $O(E \log V)$ time | **Jatin** |
| **Nearest Available Vehicle** | Binary Min-Heap Priority Queue | $O(1)$ lookup, $O(\log N)$ extraction | **Kartik** |
| **Dynamic Ambulance Fleet** | Singly Linked List | $O(1)$ dynamic insertion | **Mansi** |
| **Incoming Emergency Calls** | FIFO Queue | $O(1)$ enqueue / dequeue | **Mahima** |
| **Incident History Review** | LIFO Stack | $O(1)$ push / pop | **Mahima** |
| **Data Persistence** | Flat-File Disk I/O (`.txt` files) | Zero external DBMS dependency | **Mansi** |

---

## 🏗️ Architecture & Module Contracts

```
[ Incoming Emergency Call ]
            │
            ▼
┌──────────────────────────────────────┐
│ Step 1 (Mahima): Call Intake Queue   │  --> call_system.h / call_system.c
│ - FIFO Queue handles triage fairly   │
└──────────────────┬───────────────────┘
                   │
                   ▼
┌──────────────────────────────────────┐
│ Step 2 (Jatin): Graph & Dijkstra     │  --> graph.h / graph.c (COMPLETED)
│ - Evaluates road network distances   │
└──────────────────┬───────────────────┘
                   │
                   ▼
┌──────────────────────────────────────┐
│ Step 3 (Kartik): Min-Heap Dispatch   │  --> dispatch_heap.h / dispatch_heap.c
│ - Min-Heap extracts closest vehicle  │
│ - Updates: AVAILABLE -> EN_ROUTE     │
└──────────────────┬───────────────────┘
                   │
                   ▼
┌──────────────────────────────────────┐
│ Step 4 (Mansi): Fleet & Persistence  │  --> fleet_hospital.h / fleet_hospital.c
│ - Linked list updates status         │
│ - Deducts hospital bed capacities    │
│ - Saves state to disk (.txt)         │
└──────────────────┬───────────────────┘
                   │
                   ▼
┌──────────────────────────────────────┐
│ Step 5 (Mahima): Historical Stack    │  --> call_system.h / call_system.c
│ - Pushes completed call to Stack     │
│ - Dispatcher UI displays live status │
└──────────────────────────────────────┘
```

---

## 📂 Project Directory Structure

```text
├── common.h            # Master data structures, enums (AmbulanceStatus, PriorityLevel, RouteResult)
├── graph.h             # Graph & Dijkstra declarations (Jatin)
├── graph.c             # Adjacency list and Dijkstra implementation (Jatin)
├── dispatch_heap.h     # Min-Heap & State transition header (Kartik)
├── fleet_hospital.h    # Singly linked list, hospital tracker & persistence header (Mansi)
├── call_system.h       # FIFO Queue & LIFO Stack header (Mahima)
├── city_map.txt        # Plain-text road network configuration
├── test_graph.c        # Standalone test runner for core graph engine
├── build.bat           # Automated compilation script for Windows (GCC)
├── TEAM_BRIEFING.md    # Detailed implementation guidelines for each member
└── README.md           # Project documentation
```

---

## 🚀 How to Build and Run (Windows / GCC)

### Prerequisites
Make sure you have GCC (MinGW-w64) installed and in your system PATH.

### 1. Test Jatin's Graph & Dijkstra Engine
To compile and test the routing engine standalone:
```powershell
gcc -Wall -Wextra -std=c99 -O2 graph.c test_graph.c -o test_graph.exe
.\test_graph.exe
```
Or run:
```powershell
.\build.bat
```

### 2. Full System Build (Once all team members finish their `.c` files)
```powershell
gcc -Wall -Wextra -std=c99 -O2 graph.c dispatch_heap.c fleet_hospital.c call_system.c main.c -o ambulance_system.exe
.\ambulance_system.exe
```

---

## 👥 Contribution Guide for Team Members

1. **Clone the repository:**
   ```bash
   git clone <repo-url>
   cd "data structure pbl"
   ```
2. **Review your contract header:**
   * **Kartik:** Read `dispatch_heap.h` and implement `dispatch_heap.c`.
   * **Mansi:** Read `fleet_hospital.h` and implement `fleet_hospital.c`.
   * **Mahima:** Read `call_system.h` and implement `call_system.c`.
3. **Do not modify `common.h`** without discussing with Jatin to avoid breaking system-wide types.
4. Refer to `TEAM_BRIEFING.md` for specific function responsibilities and viva preparation answers.
