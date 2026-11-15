#include <stdio.h>
#include <stdlib.h>
#include "dispatch_heap.h"

/*
 * ============================================================================
 * MODULE: BINARY MIN-HEAP DISPATCH ENGINE (IMPLEMENTATION)
 * Author: Kartik (Refactored by Jatin for C99 Compliance & System Integration)
 *
 * Mechanics:
 * - Array-based complete binary tree
 * - Parent index: (i - 1) / 2
 * - Left child:   2 * i + 1
 * - Right child:  2 * i + 2
 * - Heap Invariant: elements[parent].distance <= elements[child].distance
 * ============================================================================
 */

/* Helper function to swap two HeapNode records in array */
static void swap_nodes(HeapNode* a, HeapNode* b) {
    HeapNode temp = *a;
    *a = *b;
    *b = temp;
}

/* ----------------------------------------------------------------------------
 * 1. LIFECYCLE MANAGEMENT
 * ---------------------------------------------------------------------------- */

/* Allocates and initializes an empty Min-Heap priority queue */
MinHeap* create_min_heap(int capacity) {
    if (capacity <= 0 || capacity > MAX_NODES) {
        capacity = MAX_NODES;
    }

    MinHeap* heap = (MinHeap*)malloc(sizeof(MinHeap));
    if (!heap) {
        fprintf(stderr, "[FATAL] Memory allocation failed for MinHeap.\n");
        return NULL;
    }

    heap->size = 0;
    heap->capacity = capacity;
    return heap;
}

/* Deallocates heap memory */
void free_min_heap(MinHeap* heap) {
    if (heap) {
        free(heap);
    }
}

/* Returns true if the heap currently holds zero elements */
bool heap_is_empty(const MinHeap* heap) {
    return (!heap || heap->size == 0);
}

/* ----------------------------------------------------------------------------
 * 2. CORE HEAP RESTORATION MECHANICS (Sift-Up and Sift-Down)
 * ---------------------------------------------------------------------------- */

/*
 * Restores the min-heap property by bubbling the element at 'index' upward.
 * Time Complexity: O(log N)
 */
void heapify_up(MinHeap* heap, int index) {
    if (!heap || index <= 0 || index >= heap->size) {
        return;
    }

    int current = index;
    while (current > 0) {
        int parent = (current - 1) / 2;

        /* If parent's distance is greater than current's distance, swap them */
        if (heap->elements[parent].distance_to_patient > heap->elements[current].distance_to_patient) {
            swap_nodes(&heap->elements[parent], &heap->elements[current]);
            current = parent;
        } else {
            /* Min-heap invariant is satisfied */
            break;
        }
    }
}

/*
 * Restores the min-heap property by sinking the element at 'index' downward.
 * Time Complexity: O(log N)
 */
void heapify_down(MinHeap* heap, int index) {
    if (!heap || index < 0 || index >= heap->size) {
        return;
    }

    int current = index;
    while (true) {
        int left = 2 * current + 1;
        int right = 2 * current + 2;
        int smallest = current;

        /* Check if left child has smaller distance than current smallest */
        if (left < heap->size &&
            heap->elements[left].distance_to_patient < heap->elements[smallest].distance_to_patient) {
            smallest = left;
        }

        /* Check if right child has even smaller distance */
        if (right < heap->size &&
            heap->elements[right].distance_to_patient < heap->elements[smallest].distance_to_patient) {
            smallest = right;
        }

        /* If smallest is not current, swap and continue sifting down */
        if (smallest != current) {
            swap_nodes(&heap->elements[current], &heap->elements[smallest]);
            current = smallest;
        } else {
            break;
        }
    }
}

/* ----------------------------------------------------------------------------
 * 3. PRIORITY QUEUE OPERATIONS
 * ---------------------------------------------------------------------------- */

/*
 * Inserts a candidate ambulance into the priority queue.
 * Appends at index 'size' and calls heapify_up().
 * Time Complexity: O(log N)
 */
bool heap_insert(MinHeap* heap, int ambulance_id, int distance, int current_node) {
    if (!heap) {
        fprintf(stderr, "[ERROR] heap_insert: Heap pointer is NULL.\n");
        return false;
    }

    if (heap->size >= heap->capacity) {
        fprintf(stderr, "[WARNING] MinHeap is at full capacity (%d). Cannot insert vehicle %d.\n",
                heap->capacity, ambulance_id);
        return false;
    }

    /* Place at the next available leaf position */
    int idx = heap->size;
    heap->elements[idx].ambulance_id = ambulance_id;
    heap->elements[idx].distance_to_patient = distance;
    heap->elements[idx].current_node = current_node;

    heap->size++;

    /* Restore min-heap property */
    heapify_up(heap, idx);
    return true;
}

/*
 * Extracts the ambulance closest to the patient (the root of the min-heap).
 * Replaces root with the last leaf and calls heapify_down().
 * Time Complexity: O(log N) extraction, O(1) minimum lookup.
 */
bool heap_extract_min(MinHeap* heap, HeapNode* out_min) {
    if (!heap || heap->size == 0 || !out_min) {
        return false;
    }

    /* Copy root element */
    *out_min = heap->elements[0];

    /* Move last element to the root position */
    heap->elements[0] = heap->elements[heap->size - 1];
    heap->size--;

    /* Sift root down if heap still has elements */
    if (heap->size > 0) {
        heapify_down(heap, 0);
    }

    return true;
}

/* ----------------------------------------------------------------------------
 * 4. AMBULANCE STATE TRANSITION HELPER
 * ---------------------------------------------------------------------------- */

/* Converts enum status into human-readable string */
const char* get_status_string(AmbulanceStatus status) {
    switch (status) {
        case STATUS_AVAILABLE: return "AVAILABLE (Base / Idle)";
        case STATUS_EN_ROUTE:   return "EN ROUTE (Driving to Scene)";
        case STATUS_AT_SCENE:   return "AT SCENE (Providing First Aid)";
        case STATUS_RETURNING:  return "RETURNING (Transporting to Hospital)";
        default:                return "UNKNOWN STATUS";
    }
}

/* Logs and transitions the state of an ambulance */
bool transition_ambulance_status(int ambulance_id, AmbulanceStatus new_status) {
    printf("[STATE MACHINE] Ambulance ID %d status transitioned -> [%s]\n",
           ambulance_id, get_status_string(new_status));
    return true;
}
