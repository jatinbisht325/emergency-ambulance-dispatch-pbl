#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "common.h"
#include "graph.h"
#include "dispatch_heap.h"
#include "fleet_hospital.h"
#include "call_system.h"

/*
 * ============================================================================
 * EMERGENCY AMBULANCE DISPATCH & FLEET OPTIMIZATION SYSTEM
 * Academic Project (PBL) - Data Structures in C
 * Graphic Era Hill University (GEHU), Bhimtal
 *
 * Team Members & Contributions:
 * - Jatin Bisht (Leader) : Graph Road Network, Sparse Adjacency List, Dijkstra Routing
 * - Kartik              : Binary Min-Heap Priority Queue, Vehicle State Transitions
 * - Mansi               : Ambulance Fleet (Linked List), Hospital Tracker, File I/O
 * - Mahima              : Emergency Call Intake (FIFO Queue), Incident History (LIFO Stack)
 * ============================================================================
 */

/* Clear screen helper */
static void clear_screen(void) {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

/* Pause helper */
static void pause_prompt(void) {
    printf("\nPress [Enter] to return to the Control Dashboard...");
    while (getchar() != '\n');
    getchar();
}

/*
 * ============================================================================
 * VISUAL 2D CITY ROAD NETWORK SCHEMATIC
 * Renders a visual structural layout of the Bhimtal emergency network
 * ============================================================================
 */
static void print_visual_map(void) {
    printf("===============================================================================\n");
    printf("               BHIMTAL REGIONAL EMERGENCY ROAD NETWORK SCHEMATIC               \n");
    printf("===============================================================================\n\n");
    printf("   [00] Bhimtal Base [B] --------- 4 km --------- [01] Tallital Junction\n");
    printf("          |                                              |         |\n");
    printf("         2 km                                           5 km      6 km\n");
    printf("          |                                              |         |\n");
    printf("   [03] GEHU Campus Gate --------- 3 km --------- [04] Bhowali     |\n");
    printf("          |                                         Crossroad      |\n");
    printf("         7 km                                            |         |\n");
    printf("          |                                             3 km       |\n");
    printf("   [06] Kainchi Sector Post                              |         |\n");
    printf("          |      \\                                [05] District    |\n");
    printf("          |       \\------ 4 km ---------\\          Civil Hosp [H]  |\n");
    printf("         6 km                            \\               |         |\n");
    printf("          |                               \\             5 km       |\n");
    printf("          |                                \\             |         |\n");
    printf("   [08] City Multi-Specialty Trauma Center [H+B] --------+-- 8 km -+-- [09] Jeolikote\n");
    printf("          |                                                        |\n");
    printf("         3 km                                                     4 km\n");
    printf("          |                                                        |\n");
    printf("   [07] Kathgodam Base [B] ----------------------------------------+\n\n");
    printf("   -------------------------------------------------------------------------\n");
    printf("   LEGEND:  [B] = Ambulance Station   |   [H] = Hospital with Bed/ICU Ward   \n");
    printf("   -------------------------------------------------------------------------\n");
}

/*
 * Helper to count total available beds across all hospitals
 */
static int get_total_free_beds(const HospitalList* list) {
    if (!list) return 0;
    int sum = 0;
    HospitalNode* curr = list->head;
    while (curr) {
        sum += curr->available_beds;
        curr = curr->next;
    }
    return sum;
}

/*
 * Helper to count total bed capacity across all hospitals
 */
static int get_total_bed_capacity(const HospitalList* list) {
    if (!list) return 0;
    int sum = 0;
    HospitalNode* curr = list->head;
    while (curr) {
        sum += curr->total_beds;
        curr = curr->next;
    }
    return sum;
}

/*
 * Prints a top-level telemetry summary bar
 */
static void print_telemetry_bar(const Fleet* fleet, const HospitalList* hospitals,
                               const CallQueue* queue, const CallHistoryStack* history) {
    int free_amb = count_available_ambulances(fleet);
    int total_amb = fleet ? fleet->count : 0;
    int pending_calls = queue ? queue->count : 0;
    int free_beds = get_total_free_beds(hospitals);
    int total_beds = get_total_bed_capacity(hospitals);
    int resolved_calls = history ? history->count : 0;

    printf("+-----------------------------------------------------------------------------+\n");
    printf("| [TELEMETRY] Ambulances: %d/%d Ready | Calls: %d Pending | Beds: %d/%d Free | Audited: %d |\n",
           free_amb, total_amb, pending_calls, free_beds, total_beds, resolved_calls);
    printf("+-----------------------------------------------------------------------------+\n");
}

/*
 * ============================================================================
 * CORE DISPATCH ENGINE: Ties all 4 Data Structures Together
 * ============================================================================
 */
static void dispatch_next_emergency(CityGraph* city, Fleet* fleet, HospitalList* hospitals,
                                   CallQueue* queue, CallHistoryStack* history) {
    if (is_queue_empty(queue)) {
        printf("\n[DISPATCH ALERT] No emergency calls currently waiting in the queue.\n");
        return;
    }

    printf("\n===============================================================================\n");
    printf("                  AUTOMATED 112 / 999 DISPATCH ENGINE                         \n");
    printf("===============================================================================\n");

    /* Step 1: Dequeue oldest emergency call from Mahima's FIFO Queue */
    EmergencyCall* call = dequeue_call(queue);
    printf("[*] Triaging Call #%d at Scene: Node [%02d] (%s)\n",
           call->call_id, call->caller_node, city->landmarks[call->caller_node].name);
    printf("    Incident Details: \"%s\" | Severity Level: %d\n",
           call->description, call->severity);

    /* Step 2: Filter available ambulances from Mansi's Singly Linked List */
    int available_count = count_available_ambulances(fleet);
    if (available_count == 0) {
        printf("\n[DISPATCH WARNING] All ambulances are currently BUSY or deployed!\n");
        printf("Re-enqueuing Call #%d at front of queue for immediate servicing once a vehicle frees up.\n", call->call_id);
        
        enqueue_call(queue, call->call_id, call->caller_node, call->severity, call->description);
        free(call);
        return;
    }

    printf("\n[*] Found %d available ambulances in fleet. Computing road distances...\n", available_count);

    /* Step 3: Run Jatin's Dijkstra from each available vehicle to the accident scene */
    MinHeap* heap = create_min_heap(MAX_NODES);
    AmbulanceNode* curr_amb = fleet->head;

    while (curr_amb != NULL) {
        if (curr_amb->status == STATUS_AVAILABLE) {
            RouteResult route = find_shortest_path(city, curr_amb->current_node, call->caller_node);
            if (route.is_reachable) {
                /* Step 4: Insert (ambulance_id, distance) into Kartik's Min-Heap */
                heap_insert(heap, curr_amb->id, route.total_distance, curr_amb->current_node);
                printf("    |-- Candidate Amb #%-5d at Node [%02d] -> Distance: %d km\n",
                       curr_amb->id, curr_amb->current_node, route.total_distance);
            }
        }
        curr_amb = curr_amb->next;
    }

    if (heap_is_empty(heap)) {
        printf("\n[ERROR] No available ambulances can reach Node [%02d] via road network.\n", call->caller_node);
        free_min_heap(heap);
        free(call);
        return;
    }

    /* Step 5: Extract closest vehicle in O(1) from Kartik's Min-Heap */
    HeapNode closest;
    heap_extract_min(heap, &closest);
    free_min_heap(heap);

    printf("\n[+] OPTIMAL VEHICLE SELECTED BY MIN-HEAP: Ambulance #%d (Proximity: %d km)\n",
           closest.ambulance_id, closest.distance_to_patient);

    /* Step 6: Select best receiving hospital (Capacity + Shortest Distance from Scene) */
    bool need_icu = (call->severity >= SEVERITY_HIGH);
    bool need_trauma = (call->severity == SEVERITY_CRITICAL);

    HospitalNode* best_hosp = NULL;
    int min_hosp_dist = INF;
    RouteResult best_hosp_route;
    best_hosp_route.is_reachable = false;

    HospitalNode* h_curr = hospitals->head;
    while (h_curr != NULL) {
        if (need_trauma && !h_curr->has_trauma_center) {
            h_curr = h_curr->next;
            continue;
        }
        if (need_icu && h_curr->icu_beds_available <= 0) {
            h_curr = h_curr->next;
            continue;
        }
        if (h_curr->available_beds <= 0) {
            h_curr = h_curr->next;
            continue;
        }

        RouteResult r = find_shortest_path(city, call->caller_node, h_curr->node_id);
        if (r.is_reachable && r.total_distance < min_hosp_dist) {
            min_hosp_dist = r.total_distance;
            best_hosp = h_curr;
            best_hosp_route = r;
        }

        h_curr = h_curr->next;
    }

    if (!best_hosp) {
        printf("\n[HOSPITAL ALERT] No hospitals currently have free beds matching patient requirements!\n");
        free(call);
        return;
    }

    printf("[+] RECEIVING HOSPITAL IDENTIFIED: '%s' (Node [%02d], Distance from Scene: %d km)\n",
           best_hosp->name, best_hosp->node_id, min_hosp_dist);

    /* Step 7: Tracing Turn-by-Turn Driving Routes */
    RouteResult leg1 = find_shortest_path(city, closest.current_node, call->caller_node);
    printf("\n>>> DISPATCH LEG 1: Base to Scene (%s -> %s)\n",
           city->landmarks[closest.current_node].name, city->landmarks[call->caller_node].name);
    print_route(city, &leg1);

    printf(">>> DISPATCH LEG 2: Patient Transport (%s -> %s)\n",
           city->landmarks[call->caller_node].name, best_hosp->name);
    print_route(city, &best_hosp_route);

    int total_mission_km = leg1.total_distance + best_hosp_route.total_distance;
    printf(">>> TOTAL MISSION DISTANCE: %d km\n", total_mission_km);

    /* Step 8: Update States and Capacities */
    printf("\n[*] Executing Live Status Transitions:\n");
    transition_ambulance_status(closest.ambulance_id, STATUS_EN_ROUTE);
    transition_ambulance_status(closest.ambulance_id, STATUS_AT_SCENE);
    transition_ambulance_status(closest.ambulance_id, STATUS_RETURNING);

    /* Admit patient to hospital */
    admit_patient_to_hospital(hospitals, best_hosp->id, need_icu);

    /* Ambulance completes mission at the hospital base */
    update_ambulance_location(fleet, closest.ambulance_id, best_hosp->node_id);
    update_ambulance_status(fleet, closest.ambulance_id, STATUS_AVAILABLE);
    transition_ambulance_status(closest.ambulance_id, STATUS_AVAILABLE);

    /* Step 9: Push completed incident to Mahima's LIFO CallHistoryStack */
    call->assigned_ambulance_id = closest.ambulance_id;
    call->assigned_hospital_id = best_hosp->id;
    call->total_trip_distance = total_mission_km;
    call->is_resolved = true;

    push_history(history, call);
    free(call);

    printf("\n[SUCCESS] Emergency incident resolved and logged in Supervisor Audit Stack.\n");
    printf("===============================================================================\n");
}

/*
 * ============================================================================
 * DIRECT ROUTE EXPLORER: Demonstrates Dijkstra on-demand for any two nodes
 * ============================================================================
 */
static void explore_route(const CityGraph* city) {
    printf("===============================================================================\n");
    printf("                   DIRECT ROUTE EXPLORER (DIJKSTRA ENGINE)                     \n");
    printf("===============================================================================\n");
    printf(" Available City Intersections:\n");
    for (int i = 0; i < city->num_nodes; i++) {
        printf("   [%02d] %-28s %s\n",
               i, city->landmarks[i].name,
               city->landmarks[i].is_hospital ? "[HOSPITAL]" : (city->landmarks[i].is_base ? "[BASE]" : ""));
    }

    int src, dest;
    printf("\nEnter Starting Intersection Node [0-%d]: ", city->num_nodes - 1);
    if (scanf("%d", &src) != 1 || src < 0 || src >= city->num_nodes) {
        printf("[ERROR] Invalid start node.\n");
        return;
    }

    printf("Enter Destination Intersection Node [0-%d]: ", city->num_nodes - 1);
    if (scanf("%d", &dest) != 1 || dest < 0 || dest >= city->num_nodes) {
        printf("[ERROR] Invalid destination node.\n");
        return;
    }

    printf("\n[*] Running Dijkstra Shortest Path Algorithm from Node [%02d] to Node [%02d]...\n", src, dest);
    RouteResult res = find_shortest_path(city, src, dest);
    print_route(city, &res);
}

/*
 * ============================================================================
 * PATIENT DISCHARGE / HOSPITAL BED DEALLOCATION
 * ============================================================================
 */
static void discharge_patient_interface(HospitalList* hospitals) {
    printf("===============================================================================\n");
    printf("                      PATIENT DISCHARGE & BED RELEASE                          \n");
    printf("===============================================================================\n");
    print_hospitals(hospitals);

    int hosp_id, is_icu;
    printf("Enter Hospital ID for patient discharge: ");
    if (scanf("%d", &hosp_id) != 1) return;

    printf("Was this patient occupying an ICU bed? (1 = YES, 0 = NO): ");
    if (scanf("%d", &is_icu) != 1) is_icu = 0;

    if (discharge_patient_from_hospital(hospitals, hosp_id, is_icu != 0)) {
        printf("\n[SUCCESS] Patient successfully discharged. Hospital bed restored.\n");
    } else {
        printf("\n[NOTICE] Hospital ID %d already at maximum bed capacity or not found.\n", hosp_id);
    }
}

/*
 * ============================================================================
 * AUTOMATED DEMO SIMULATION (For Professor & External Viva Presentation)
 * ============================================================================
 */
static void run_automated_demo(CityGraph* city, Fleet* fleet, HospitalList* hospitals,
                              CallQueue* queue, CallHistoryStack* history) {
    printf("\n===============================================================================\n");
    printf("           RUNNING FULL END-TO-END DEMONSTRATION SIMULATION                    \n");
    printf("===============================================================================\n");
    printf(" Simulating a realistic city emergency scenario with 3 simultaneous calls...\n\n");

    /* 1. Register 3 incoming calls */
    printf("[1/3] Enqueuing Call #1001: Critical Collision at GEHU Campus Gate (Node 3)\n");
    enqueue_call(queue, 1001, 3, SEVERITY_CRITICAL, "Severe vehicle crash, trauma support required");

    printf("[2/3] Enqueuing Call #1002: Cardiac Distress at Mallital Market (Node 2)\n");
    enqueue_call(queue, 1002, 2, SEVERITY_HIGH, "Elderly patient cardiac arrest");

    printf("[3/3] Enqueuing Call #1003: Sports Injury at Kainchi Sector Post (Node 6)\n");
    enqueue_call(queue, 1003, 6, SEVERITY_LOW, "Minor compound fracture");

    print_pending_calls(queue);

    /* 2. Dispatch Call 1 */
    printf("\n[*] Processing Dispatch for First Call (#1001)...\n");
    dispatch_next_emergency(city, fleet, hospitals, queue, history);

    /* 3. Dispatch Call 2 */
    printf("\n[*] Processing Dispatch for Second Call (#1002)...\n");
    dispatch_next_emergency(city, fleet, hospitals, queue, history);

    /* 4. Display Updated State */
    printf("\n[*] Current Active Fleet after Dispatches:\n");
    print_fleet(fleet);

    printf("[*] Current Hospital Capacities after Admissions:\n");
    print_hospitals(hospitals);

    printf("[*] Supervisor Audit Trail (LIFO Stack):\n");
    print_recent_history(history, 5);

    printf("\n[+] AUTOMATED DEMONSTRATION COMPLETED SUCCESSFULLY.\n");
}

/*
 * ============================================================================
 * MAIN APPLICATION ENTRY POINT & INTERACTIVE MENU
 * ============================================================================
 */
int main(void) {
    clear_screen();
    printf("===============================================================================\n");
    printf("      EMERGENCY AMBULANCE DISPATCH & FLEET OPTIMIZATION SYSTEM (112/999)       \n");
    printf("             Academic Project: Data Structures & Algorithms in C               \n");
    printf("             Graphic Era Hill University (GEHU), Bhimtal                       \n");
    printf("===============================================================================\n");

    /* 1. Initialize City Graph & Load Map */
    printf("[*] Initializing City Road Network from 'city_map.txt'...\n");
    CityGraph* city = create_graph(MAX_NODES);
    if (!city || !load_map_from_file(city, "city_map.txt")) {
        fprintf(stderr, "[FATAL] Failed to load city_map.txt. Ensure file exists in working dir.\n");
        return EXIT_FAILURE;
    }

    /* 2. Initialize Ambulance Fleet & Load from File */
    printf("[*] Initializing Dynamic Ambulance Fleet from 'fleet.txt'...\n");
    Fleet* fleet = create_fleet();
    if (!fleet) {
        fprintf(stderr, "[FATAL] Failed to create fleet.\n");
        free_graph(city);
        return EXIT_FAILURE;
    }
    load_fleet_from_file(fleet, "fleet.txt");

    /* 3. Initialize Hospital Directory & Load from File */
    printf("[*] Initializing Hospital Directory from 'hospitals.txt'...\n");
    HospitalList* hospitals = create_hospital_list();
    if (!hospitals) {
        fprintf(stderr, "[FATAL] Failed to create hospital list.\n");
        free_fleet(fleet);
        free_graph(city);
        return EXIT_FAILURE;
    }
    load_hospitals_from_file(hospitals, "hospitals.txt");

    /* 4. Initialize Call Queue & History Stack */
    CallQueue* queue = create_call_queue();
    CallHistoryStack* history = create_call_stack();
    if (!queue || !history) {
        fprintf(stderr, "[FATAL] Failed to create call queue or history stack.\n");
        return EXIT_FAILURE;
    }

    printf("\n[+] All 4 Core Data Structures Initialized (Graph, Fleet, Min-Heap, Queue/Stack).\n");
    pause_prompt();

    int next_call_id = 101;
    int choice = -1;

    while (choice != 0) {
        clear_screen();
        printf("===============================================================================\n");
        printf("                EMERGENCY DISPATCH OPERATIONS DASHBOARD                        \n");
        printf("===============================================================================\n");
        print_telemetry_bar(fleet, hospitals, queue, history);
        printf("\n");
        printf("  [1] View Visual 2D City Road Network & Graph (Jatin - Adjacency List)\n");
        printf("  [2] View Active Ambulance Fleet Registry     (Mansi - Singly Linked List)\n");
        printf("  [3] View Hospital Capacities & ICU Monitor    (Mansi - Bed Tracker)\n");
        printf("  [4] Intake New 112/999 Emergency Call         (Mahima - FIFO Queue)\n");
        printf("  [5] Inspect Pending Emergency Calls Queue    (Mahima - FIFO Queue)\n");
        printf("  [6] Auto-Dispatch Nearest Ambulance          (Jatin & Kartik - Dijkstra + Heap)\n");
        printf("  [7] Direct Route Explorer (Any 2 Nodes)      (Jatin - Dijkstra Engine)\n");
        printf("  [8] Discharge Patient / Free Hospital Bed    (Mansi - Capacity Release)\n");
        printf("  [9] View Incident Audit History              (Mahima - LIFO Stack)\n");
        printf(" [10] Persist System State to Disk (.txt)      (Mansi - File I/O)\n");
        printf(" [11] Run Automated Professor Demo Simulation  (End-to-End Walkthrough)\n");
        printf("  [0] Save State & Exit System\n");
        printf("===============================================================================\n");
        printf(" Select an option [0-11]: ");

        if (scanf("%d", &choice) != 1) {
            while (getchar() != '\n');
            continue;
        }

        switch (choice) {
            case 1:
                clear_screen();
                print_visual_map();
                printf("\n");
                print_graph(city);
                pause_prompt();
                break;

            case 2:
                clear_screen();
                print_fleet(fleet);
                pause_prompt();
                break;

            case 3:
                clear_screen();
                print_hospitals(hospitals);
                pause_prompt();
                break;

            case 4: {
                clear_screen();
                printf("===============================================================================\n");
                printf("                       INCOMING 112 / 999 EMERGENCY CALL                       \n");
                printf("===============================================================================\n");
                int node_id, sev_input;
                char desc[128];

                printf("Available City Intersections:\n");
                for (int i = 0; i < city->num_nodes; i++) {
                    printf("  [%02d] %s\n", i, city->landmarks[i].name);
                }

                printf("\nEnter Intersection Node of Emergency [0 to %d]: ", city->num_nodes - 1);
                if (scanf("%d", &node_id) != 1 || node_id < 0 || node_id >= city->num_nodes) {
                    printf("[ERROR] Invalid intersection node.\n");
                    pause_prompt();
                    break;
                }

                printf("Select Severity (1=LOW, 2=MEDIUM, 3=HIGH, 4=CRITICAL): ");
                if (scanf("%d", &sev_input) != 1 || sev_input < 1 || sev_input > 4) {
                    sev_input = 2;
                }

                while (getchar() != '\n');
                printf("Enter Brief Incident Description: ");
                if (fgets(desc, sizeof(desc), stdin)) {
                    desc[strcspn(desc, "\r\n")] = '\0';
                }

                enqueue_call(queue, next_call_id++, node_id, (PriorityLevel)sev_input, desc);
                printf("\n[+] Emergency Call #%d registered into FIFO queue successfully.\n", next_call_id - 1);
                pause_prompt();
                break;
            }

            case 5:
                clear_screen();
                print_pending_calls(queue);
                pause_prompt();
                break;

            case 6:
                clear_screen();
                dispatch_next_emergency(city, fleet, hospitals, queue, history);
                pause_prompt();
                break;

            case 7:
                clear_screen();
                explore_route(city);
                pause_prompt();
                break;

            case 8:
                clear_screen();
                discharge_patient_interface(hospitals);
                pause_prompt();
                break;

            case 9:
                clear_screen();
                print_recent_history(history, 10);
                pause_prompt();
                break;

            case 10:
                clear_screen();
                printf("[*] Saving system state to persistent disk files...\n");
                save_fleet_to_file(fleet, "fleet.txt");
                save_hospitals_to_file(hospitals, "hospitals.txt");
                printf("[+] System state successfully synchronized to 'fleet.txt' and 'hospitals.txt'.\n");
                pause_prompt();
                break;

            case 11:
                clear_screen();
                run_automated_demo(city, fleet, hospitals, queue, history);
                pause_prompt();
                break;

            case 0:
                printf("\n[*] Synchronizing fleet and hospital state to disk before exit...\n");
                save_fleet_to_file(fleet, "fleet.txt");
                save_hospitals_to_file(hospitals, "hospitals.txt");
                printf("[+] Shutdown clean. All records saved.\n");
                break;

            default:
                printf("\n[ERROR] Invalid option selected. Please choose between 0 and 11.\n");
                pause_prompt();
                break;
        }
    }

    /* Clean Memory Deallocation */
    free_graph(city);
    free_fleet(fleet);
    free_hospital_list(hospitals);
    free_call_queue(queue);
    free_call_stack(history);

    return EXIT_SUCCESS;
}
