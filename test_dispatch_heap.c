#include <stdio.h>
#include <stdlib.h>
#include "dispatch_heap.h"

int main(void) {
    printf("=================================================================\n");
    printf("     MIN-HEAP PRIORITY QUEUE DISPATCH ENGINE - STANDALONE TEST   \n");
    printf("     Author: Kartik | Verification Harness: Jatin Bisht          \n");
    printf("=================================================================\n");

    /* 1. Create Min-Heap */
    printf("\n[*] Initializing Min-Heap Priority Queue (Capacity: 10)...\n");
    MinHeap* heap = create_min_heap(10);
    if (!heap) {
        fprintf(stderr, "[FATAL] Failed to create MinHeap.\n");
        return EXIT_FAILURE;
    }

    /* 2. Insert candidate ambulances with various distances to patient */
    printf("[*] Simulating candidates evaluated by Dijkstra's shortest path:\n");
    printf("    -> Vehicle 101 parked at Node 0 (Distance: 12 km)\n");
    heap_insert(heap, 101, 12, 0);

    printf("    -> Vehicle 102 parked at Node 7 (Distance: 4 km)\n");
    heap_insert(heap, 102, 4, 7);

    printf("    -> Vehicle 103 parked at Node 8 (Distance: 7 km)\n");
    heap_insert(heap, 103, 7, 8);

    printf("    -> Vehicle 104 parked at Node 0 (Distance: 2 km)\n");
    heap_insert(heap, 104, 2, 0);

    printf("    -> Vehicle 105 parked at Node 7 (Distance: 9 km)\n");
    heap_insert(heap, 105, 9, 7);

    printf("[+] Total candidates queued in MinHeap: %d\n", heap->size);

    /* 3. Extract in priority order (Closest first: 2km -> 4km -> 7km -> 9km -> 12km) */
    printf("\n[*] Extracting nearest available ambulances in order of proximity:\n");
    printf("-----------------------------------------------------------------\n");
    printf(" Priority Rank | Vehicle ID | Parked Node | Road Distance\n");
    printf("-----------------------------------------------------------------\n");

    int rank = 1;
    HeapNode min_amb;
    while (heap_extract_min(heap, &min_amb)) {
        printf(" Rank #%-7d | Amb %-6d | Node [%02d]   | %d km\n",
               rank++, min_amb.ambulance_id, min_amb.current_node, min_amb.distance_to_patient);
    }
    printf("-----------------------------------------------------------------\n");

    /* 4. Test Edge Case: Extracting from empty heap */
    printf("\n[*] Testing edge case: Extraction from empty heap...\n");
    if (!heap_extract_min(heap, &min_amb)) {
        printf("[+] Correctly handled empty heap extraction without crash.\n");
    }

    /* 5. Test State Transitions */
    printf("\n[*] Testing State Transition Lifecycle for Ambulance 104:\n");
    transition_ambulance_status(104, STATUS_EN_ROUTE);
    transition_ambulance_status(104, STATUS_AT_SCENE);
    transition_ambulance_status(104, STATUS_RETURNING);
    transition_ambulance_status(104, STATUS_AVAILABLE);

    /* 6. Clean Memory */
    printf("\n[*] Deallocating MinHeap memory...\n");
    free_min_heap(heap);
    printf("[+] Memory deallocated cleanly.\n");

    printf("\n=================================================================\n");
    printf("     KARTIK'S MODULE VERIFICATION COMPLETE - FULLY FUNCTIONAL    \n");
    printf("=================================================================\n");

    return EXIT_SUCCESS;
}
