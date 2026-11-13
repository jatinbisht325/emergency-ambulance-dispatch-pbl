/*
 * main.c  -  Emergency Ambulance Dispatch & Fleet Optimization System
 * Integration layer: ties together
 *   graph.*          (Jatin)  - city map + Dijkstra
 *   dispatch_heap.*  (Kartik) - min-heap to pick the closest ambulance
 *   fleet_hospital.* (Mansi)  - fleet list, hospital beds, file save/load
 *   call_system.*    (Mahima) - FIFO call queue, LIFO history stack
 *
 * REQUIRES: fleet_hospital.h must use common.h's AmbulanceStatus
 * (delete its own FleetStatus enum and #include "common.h").
 */

#include "common.h"
#include "graph.h"
#include "dispatch_heap.h"
#include "fleet_hospital.h"
#include "call_system.h"

#define MAP_FILE        "city_map.txt"
/* TODO: copy the value Jatin passes to create_graph() in test_graph.c.
 * If load_map_from_file() sets num_nodes itself, MAX_NODES is fine. */
#define NUM_CITY_NODES  MAX_NODES
#define MAX_ACTIVE      50

/* ------------------------------------------------------------------ */
/* Global application state                                            */
/* ------------------------------------------------------------------ */
static CityGraph        *g_city    = NULL;
static Fleet             g_fleet;
static HospitalList      g_hospitals;
static CallQueue        *g_queue   = NULL;
static CallHistoryStack *g_history = NULL;

/* Calls that have been dispatched but not yet completed */
static EmergencyCall *g_active[MAX_ACTIVE];
static int            g_active_count = 0;
static int            g_next_call_id = 1;

/* ------------------------------------------------------------------ */
/* Input helpers                                                       */
/* ------------------------------------------------------------------ */
static int read_int(const char *prompt)
{
    char buf[64];
    int value;
    printf("%s", prompt);
    if (!fgets(buf, sizeof(buf), stdin)) return -1;
    if (sscanf(buf, "%d", &value) != 1)  return -1;
    return value;
}

static void read_line(const char *prompt, char *out, size_t size)
{
    printf("%s", prompt);
    if (!fgets(out, (int)size, stdin)) { out[0] = '\0'; return; }
    out[strcspn(out, "\n")] = '\0';
}

/* ------------------------------------------------------------------ */
/* Startup: seed defaults if the save files are empty                  */
/* ------------------------------------------------------------------ */
static void seed_defaults(void)
{
    if (g_hospitals.count == 0) {
        for (int i = 0; i < g_city->num_nodes; i++) {
            if (g_city->landmarks[i].is_hospital) {
                hospital_add(&g_hospitals, i, g_city->landmarks[i].name, i,
                             10, 10, 1, 1);
            }
        }
    }
    if (g_fleet.count == 0) {
        int next_id = 101;
        for (int i = 0; i < g_city->num_nodes; i++) {
            if (g_city->landmarks[i].is_base) {
                fleet_add(&g_fleet, next_id++, i, STATUS_AVAILABLE, 0);
            }
        }
    }
}

/* ------------------------------------------------------------------ */
/* Hospital selection: nearest hospital to the patient with a free bed */
/* ------------------------------------------------------------------ */
static HospitalNode *pick_hospital(int from_node, int need_icu)
{
    HospitalNode *best = NULL;
    int best_dist = INF;

    for (HospitalNode *h = g_hospitals.head; h; h = h->next) {
        if (h->available_beds <= 0) continue;
        if (need_icu && !h->icu_ready) continue;

        RouteResult r = find_shortest_path(g_city, from_node, h->location);
        if (r.is_reachable && r.total_distance < best_dist) {
            best_dist = r.total_distance;
            best = h;
        }
    }
    return best;
}

/* ------------------------------------------------------------------ */
/* Menu action 1: log a new emergency call                             */
/* ------------------------------------------------------------------ */
static void action_new_call(void)
{
    char desc[128];

    printf("\n--- New Emergency Call ---\n");
    for (int i = 0; i < g_city->num_nodes; i++) {
        printf("  %2d : %s\n", i, g_city->landmarks[i].name);
    }

    int node = read_int("Caller location (node ID): ");
    if (node < 0 || node >= g_city->num_nodes) {
        printf("Invalid node.\n");
        return;
    }

    int sev = read_int("Severity (1=Low 2=Medium 3=High 4=Critical): ");
    if (sev < SEVERITY_LOW || sev > SEVERITY_CRITICAL) {
        printf("Invalid severity.\n");
        return;
    }

    read_line("Description: ", desc, sizeof(desc));

    if (enqueue_call(g_queue, g_next_call_id, node, (PriorityLevel)sev, desc)) {
        printf("Call #%d logged. Pending calls: %d\n",
               g_next_call_id, g_queue->count);
        g_next_call_id++;
    } else {
        printf("Could not log call (out of memory).\n");
    }
}

/* ------------------------------------------------------------------ */
/* Menu action 2: dispatch the oldest pending call                     */
/* ------------------------------------------------------------------ */
static void action_dispatch(void)
{
    if (is_queue_empty(g_queue)) {
        printf("\nNo pending calls.\n");
        return;
    }
    if (g_active_count >= MAX_ACTIVE) {
        printf("\nToo many active missions. Complete one first.\n");
        return;
    }

    /* Peek at the oldest call; only dequeue once dispatch is certain */
    EmergencyCall *call = g_queue->front;

    /* Step 1-3: Dijkstra from every free ambulance -> min-heap */
    MinHeap *heap = create_min_heap(MAX_NODES);
    if (!heap) {
        printf("Out of memory.\n");
        return;
    }

    for (AmbulanceNode *a = g_fleet.head; a; a = a->next) {
        if (a->status != STATUS_AVAILABLE) continue;

        RouteResult r = find_shortest_path(g_city, a->location, call->caller_node);
        if (r.is_reachable) {
            heap_insert(heap, a->id, r.total_distance, a->location);
        }
    }

    /* Step 4: closest ambulance = heap minimum */
    HeapNode best;
    bool found = heap_extract_min(heap, &best);
    free_min_heap(heap);

    if (!found) {
        printf("\nNo available ambulance can reach call #%d. "
               "It stays in the queue.\n", call->call_id);
        return;
    }

    /* Step 5: choose hospital (ICU if severity is High or Critical) */
    int need_icu = (call->severity >= SEVERITY_HIGH);
    HospitalNode *hosp = pick_hospital(call->caller_node, need_icu);
    if (!hosp && need_icu) {
        printf("No ICU bed free; falling back to any hospital.\n");
        hosp = pick_hospital(call->caller_node, 0);
    }
    if (!hosp) {
        printf("\nNo hospital with a free bed is reachable. "
               "Call #%d stays in the queue.\n", call->call_id);
        return;
    }

    /* Step 6: full routes */
    RouteResult to_patient  = find_shortest_path(g_city, best.current_node,
                                                 call->caller_node);
    RouteResult to_hospital = find_shortest_path(g_city, call->caller_node,
                                                 hosp->location);

    /* Commit: take the call out of the queue and update state */
    call = dequeue_call(g_queue);
    call->assigned_ambulance_id = best.ambulance_id;
    call->assigned_hospital_id  = hosp->id;
    call->total_trip_distance   = to_patient.total_distance +
                                  to_hospital.total_distance;

    fleet_update_status(&g_fleet, best.ambulance_id, STATUS_EN_ROUTE);
    hospital_admit(&g_hospitals, hosp->id);
    g_active[g_active_count++] = call;

    printf("\n=== DISPATCH: Call #%d ===\n", call->call_id);
    printf("Ambulance %d  ->  Patient at node %d  ->  %s\n",
           best.ambulance_id, call->caller_node, hosp->name);
    printf("\nRoute to patient (%d units):\n", to_patient.total_distance);
    print_route(g_city, &to_patient);
    printf("\nRoute to hospital (%d units):\n", to_hospital.total_distance);
    print_route(g_city, &to_hospital);
    printf("\nTotal trip distance: %d units\n", call->total_trip_distance);
}

/* ------------------------------------------------------------------ */
/* Finish a mission: ambulance has returned and is available again     */
/* ------------------------------------------------------------------ */
static void complete_mission(int ambulance_id)
{
    for (int i = 0; i < g_active_count; i++) {
        if (g_active[i]->assigned_ambulance_id != ambulance_id) continue;

        EmergencyCall *call = g_active[i];
        call->is_resolved = true;

        /* Assumes push_history copies the record (it takes a const pointer) */
        push_history(g_history, call);
        hospital_discharge(&g_hospitals, call->assigned_hospital_id);

        /* Ambulance is now parked at the hospital it delivered to */
        HospitalNode *h = hospital_find(&g_hospitals, call->assigned_hospital_id);
        AmbulanceNode *a = fleet_find(&g_fleet, ambulance_id);
        if (h && a) a->location = h->location;

        printf("Call #%d resolved and moved to history.\n", call->call_id);

        free(call);
        g_active[i] = g_active[--g_active_count];
        return;
    }
}

/* ------------------------------------------------------------------ */
/* Menu action 3: advance an ambulance through its 4-phase lifecycle   */
/* ------------------------------------------------------------------ */
static void action_advance_status(void)
{
    fleet_print(&g_fleet);
    int id = read_int("Ambulance ID to advance: ");

    AmbulanceNode *a = fleet_find(&g_fleet, id);
    if (!a) {
        printf("Ambulance not found.\n");
        return;
    }
    if (a->status == STATUS_AVAILABLE) {
        printf("Ambulance %d is idle. Dispatch a call first.\n", id);
        return;
    }

    AmbulanceStatus old_status = a->status;
    AmbulanceStatus next = (AmbulanceStatus)((old_status + 1) % 4);

    fleet_update_status(&g_fleet, id, next);
    printf("Ambulance %d: %s -> %s\n", id,
           get_status_string(old_status), get_status_string(next));

    if (next == STATUS_AVAILABLE) {
        complete_mission(id);
    }
}

/* ------------------------------------------------------------------ */
/* Save and cleanup                                                    */
/* ------------------------------------------------------------------ */
static void save_all(void)
{
    if (fleet_save(&g_fleet, FLEET_FILE) != FH_OK)
        printf("Warning: could not save %s\n", FLEET_FILE);
    if (hospital_save(&g_hospitals, HOSPITAL_FILE) != FH_OK)
        printf("Warning: could not save %s\n", HOSPITAL_FILE);
}

static void cleanup(void)
{
    for (int i = 0; i < g_active_count; i++) free(g_active[i]);
    g_active_count = 0;

    /* Pending calls still in the queue */
    EmergencyCall *c;
    while (g_queue && (c = dequeue_call(g_queue)) != NULL) free(c);

    free_call_queue(g_queue);
    free_call_stack(g_history);
    fleet_free(&g_fleet);
    hospital_list_free(&g_hospitals);
    free_graph(g_city);
}

/* ------------------------------------------------------------------ */
/* Menu                                                                */
/* ------------------------------------------------------------------ */
static void print_menu(void)
{
    printf("\n=========================================\n");
    printf("  EMERGENCY AMBULANCE DISPATCH SYSTEM\n");
    printf("=========================================\n");
    printf("  1. Log new emergency call\n");
    printf("  2. Dispatch next pending call\n");
    printf("  3. Advance ambulance status\n");
    printf("  4. View fleet\n");
    printf("  5. View hospitals\n");
    printf("  6. View pending calls\n");
    printf("  7. View incident history\n");
    printf("  8. View city map\n");
    printf("  9. Save and exit\n");
}

int main(void)
{
    /* ---- Initialise modules ---- */
    g_city = create_graph(NUM_CITY_NODES);
    if (!g_city || !load_map_from_file(g_city, MAP_FILE)) {
        printf("Error: could not load %s\n", MAP_FILE);
        free_graph(g_city);
        return 1;
    }

    fleet_init(&g_fleet);
    hospital_list_init(&g_hospitals);
    fleet_load(&g_fleet, FLEET_FILE);
    hospital_load(&g_hospitals, HOSPITAL_FILE);

    g_queue   = create_call_queue();
    g_history = create_call_stack();
    if (!g_queue || !g_history) {
        printf("Error: out of memory.\n");
        cleanup();
        return 1;
    }

    seed_defaults();

    /* ---- Main loop ---- */
    int running = 1;
    while (running) {
        print_menu();
        int choice = read_int("Choice: ");

        switch (choice) {
            case 1: action_new_call();                     break;
            case 2: action_dispatch();                     break;
            case 3: action_advance_status();               break;
            case 4: fleet_print(&g_fleet);                 break;
            case 5: hospital_print(&g_hospitals);          break;
            case 6: print_pending_calls(g_queue);          break;
            case 7: print_recent_history(g_history, 10);   break;
            case 8: print_graph(g_city);                   break;
            case 9: running = 0;                           break;
            default: printf("Invalid choice.\n");          break;
        }
    }

    save_all();
    cleanup();
    printf("Data saved. Goodbye.\n");
    return 0;
}
