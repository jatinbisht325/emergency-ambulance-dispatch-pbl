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

/* Clear screen helper for cross-platform aesthetic */
static void clear_screen(void) {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

/* Pause helper */
static void pause_prompt(void) {
    printf("\nPress [Enter] to return to the Main Control Center...");
    while (getchar() != '\n');
    getchar();
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
    printf("    Condition: %s | Severity Level: %d\n",
           call->description, call->severity);

    /* Step 2: Filter available ambulances from Mansi's Singly Linked List */
    int available_count = count_available_ambulances(fleet);
    if (available_count == 0) {
        printf("\n[DISPATCH WARNING] All ambulances are currently BUSY or deployed!\n");
        printf("Re-enqueuing Call #%d at front of queue for immediate servicing once a vehicle frees up.\n", call->call_id);
        
        /* Re-insert call at the front of the queue by re-enqueuing */
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
                printf("    ├── Candidate Amb #%-5d at Node [%02d] -> Distance: %d km\n",
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
        /* Check bed qualifications */
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

        /* Calculate road distance from accident scene to this hospital */
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

    /* Admit patient to hospital (decrements regular and ICU beds) */
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

    printf("\n[SUCCESS] Emergency incident resolved and permanently recorded in Supervisor Audit Stack.\n");
    printf("===============================================================================\n");
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

    printf("\n[+] All Core Data Structures Successfully Initialized & Memory Mapped.\n");
    pause_prompt();

    int next_call_id = 101;
    int choice = -1;

    while (choice != 0) {
        clear_screen();
        printf("===============================================================================\n");
        printf("                EMERGENCY DISPATCH OPERATIONS DASHBOARD                        \n");
        printf("===============================================================================\n");
        printf(" 1. View City Road Network Topology (Adjacency List - Jatin)\n");
        printf(" 2. View Active Ambulance Fleet Registry (Singly Linked List - Mansi)\n");
        printf(" 3. View Hospital Bed & ICU Capacities (Capacity Tracker - Mansi)\n");
        printf(" 4. Intake New Emergency Call (FIFO Enqueue - Mahima)\n");
        printf(" 5. View Pending Emergency Calls Queue (FIFO Inspection - Mahima)\n");
        printf(" 6. Run Smart Dispatch for Next Call (Dijkstra + Min-Heap - Jatin & Kartik)\n");
        printf(" 7. View Incident Audit History (LIFO Stack - Mahima)\n");
        printf(" 8. Persist System State to Disk (fleet.txt & hospitals.txt - Mansi)\n");
        printf(" 9. Run Automated Professor Demo Simulation (End-to-End Walkthrough)\n");
        printf(" 0. Save State & Exit System\n");
        printf("===============================================================================\n");
        printf(" Enter your command selection [0-9]: ");

        if (scanf("%d", &choice) != 1) {
            while (getchar() != '\n');
            continue;
        }

        switch (choice) {
            case 1:
                clear_screen();
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
                    printf("  Node [%02d]: %s\n", i, city->landmarks[i].name);
                }

                printf("\nEnter Intersection Node of Emergency [0 to %d]: ", city->num_nodes - 1);
                if (scanf("%d", &node_id) != 1 || node_id < 0 || node_id >= city->num_nodes) {
                    printf("[ERROR] Invalid intersection node.\n");
                    pause_prompt();
                    break;
                }

                printf("Select Severity Level (1=LOW, 2=MEDIUM, 3=HIGH, 4=CRITICAL): ");
                if (scanf("%d", &sev_input) != 1 || sev_input < 1 || sev_input > 4) {
                    sev_input = 2; // Default to medium
                }

                while (getchar() != '\n'); // Clear input buffer
                printf("Enter Brief Incident Description: ");
                if (fgets(desc, sizeof(desc), stdin)) {
                    desc[strcspn(desc, "\r\n")] = '\0';
                }

                enqueue_call(queue, next_call_id++, node_id, (PriorityLevel)sev_input, desc);
                printf("\n[+] Emergency Call #%d registered and queued into FIFO buffer.\n", next_call_id - 1);
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
                print_recent_history(history, 10);
                pause_prompt();
                break;

            case 8:
                clear_screen();
                printf("[*] Saving system state to disk...\n");
                save_fleet_to_file(fleet, "fleet.txt");
                save_hospitals_to_file(hospitals, "hospitals.txt");
                printf("[+] System state successfully synchronized to persistent files.\n");
                pause_prompt();
                break;

            case 9:
                clear_screen();
                run_automated_demo(city, fleet, hospitals, queue, history);
                pause_prompt();
                break;

            case 0:
                printf("\n[*] Auto-saving fleet and hospital state before shutdown...\n");
                save_fleet_to_file(fleet, "fleet.txt");
                save_hospitals_to_file(hospitals, "hospitals.txt");
                printf("[+] Shutdown complete. Goodbye!\n");
                break;

            default:
                printf("\n[ERROR] Invalid option selected. Please choose between 0 and 9.\n");
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
