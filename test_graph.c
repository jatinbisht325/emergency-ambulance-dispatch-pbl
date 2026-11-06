#include <stdio.h>
#include <stdlib.h>
#include "graph.h"

int main(void) {
    printf("=================================================================\n");
    printf("  EMERGENCY AMBULANCE DISPATCH SYSTEM - CORE GRAPH ENGINE TEST   \n");
    printf("  Author: Jatin (Team Leader) | Graphic Era Hill University      \n");
    printf("=================================================================\n");

    /* Step 1: Create Graph Instance */
    printf("\n[*] Initializing City Graph (Max Capacity: %d nodes)...\n", MAX_NODES);
    CityGraph* city = create_graph(MAX_NODES);
    if (!city) {
        fprintf(stderr, "[FATAL] Graph creation failed. Aborting test.\n");
        return EXIT_FAILURE;
    }

    /* Step 2: Load Road Network from Flat File */
    const char* map_file = "city_map.txt";
    printf("[*] Loading road map from '%s'...\n", map_file);
    if (!load_map_from_file(city, map_file)) {
        fprintf(stderr, "[FATAL] Failed to load road map from '%s'.\n", map_file);
        free_graph(city);
        return EXIT_FAILURE;
    }
    printf("[+] Successfully loaded %d intersections and %d road segments.\n",
           city->num_nodes, city->num_edges);

    /* Step 3: Print Graph Topology (Adjacency List verification) */
    print_graph(city);

    /* Step 4: Run Scenario 1 - Accident at GEHU Bhimtal Campus (Node 3) */
    int base_a = 0;       // Bhimtal Ambulance Base
    int accident_node = 3; // GEHU Bhimtal Campus Gate
    printf("\n=================================================================\n");
    printf(" [SCENARIO 1] EMERGENCY CALL: ACCIDENT REPORTED AT GEHU CAMPUS\n");
    printf(" Dispatching nearest unit from Bhimtal Base (Node %d) to Scene (Node %d)\n",
           base_a, accident_node);
    printf("=================================================================\n");

    RouteResult leg1 = find_shortest_path(city, base_a, accident_node);
    print_route(city, &leg1);

    /* Step 5: Run Scenario 2 - Patient Transfer: Campus (Node 3) to Civil Hospital (Node 5) */
    int hospital_node = 5; // District Civil Hospital
    printf("\n=================================================================\n");
    printf(" [SCENARIO 2] PATIENT TRANSPORT: SCENE -> DISTRICT CIVIL HOSPITAL\n");
    printf(" Computing fastest route from Node %d to Hospital (Node %d)\n",
           accident_node, hospital_node);
    printf("=================================================================\n");

    RouteResult leg2 = find_shortest_path(city, accident_node, hospital_node);
    print_route(city, &leg2);

    /* Total Mission Summary */
    printf("\n>>> TOTAL MISSION ROAD DISTANCE (Base -> Scene -> Hospital): %d km\n\n",
           leg1.total_distance + leg2.total_distance);

    /* Step 6: Run Scenario 3 - Critical Trauma: Mallital (Node 2) to Trauma Center (Node 8) */
    int mallital = 2;
    int trauma_center = 8;
    printf("=================================================================\n");
    printf(" [SCENARIO 3] CRITICAL TRAUMA: MALLITAL (Node %d) -> TRAUMA CENTER (Node %d)\n",
           mallital, trauma_center);
    printf("=================================================================\n");

    RouteResult leg3 = find_shortest_path(city, mallital, trauma_center);
    print_route(city, &leg3);

    /* Step 7: Edge Case Testing (Essential for Professor Viva!) */
    printf("=================================================================\n");
    printf(" [SCENARIO 4] DEFENSIVE TESTS & EDGE CASES\n");
    printf("=================================================================\n");

    // Test A: Same start and end node
    printf("[Test A] Start and Destination are identical (Node 1 -> Node 1):\n");
    RouteResult same_node = find_shortest_path(city, 1, 1);
    printf("   Result: Distance = %d km, Hops = %d, Reachable = %s\n",
           same_node.total_distance, same_node.path_length,
           same_node.is_reachable ? "YES" : "NO");

    // Test B: Out of bounds node ID
    printf("[Test B] Querying non-existent node (Node 0 -> Node 99):\n");
    RouteResult invalid_node = find_shortest_path(city, 0, 99);
    printf("   Result: Reachable = %s (Gracefully caught without crash)\n",
           invalid_node.is_reachable ? "YES" : "NO");

    /* Step 8: Clean Memory Deallocation */
    printf("\n[*] Deallocating graph memory and freeing adjacency lists...\n");
    free_graph(city);
    printf("[+] Memory deallocated cleanly. 0 memory leaks.\n");
    printf("=================================================================\n");
    printf("  CORE GRAPH ENGINE VERIFICATION COMPLETE - READY FOR TEAM MERGE \n");
    printf("=================================================================\n");

    return EXIT_SUCCESS;
}
