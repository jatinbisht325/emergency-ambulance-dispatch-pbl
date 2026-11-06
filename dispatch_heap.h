#ifndef DISPATCH_HEAP_H
#define DISPATCH_HEAP_H

#include "common.h"

/*
 * ============================================================================
 * MODULE: DISPATCH ENGINE & PRIORITY QUEUE
 * Author: Kartik
 * Purpose: Select closest available ambulance in O(1) lookup, O(log N) extract
 * ============================================================================
 */

/* Candidate vehicle entry stored inside the binary min-heap */
typedef struct {
    int ambulance_id;        // Unique vehicle ID
    int distance_to_patient; // Computed road distance from Jatin's Dijkstra engine
    int current_node;        // Current intersection where ambulance is parked
} HeapNode;

/* Binary Min-Heap Priority Queue */
typedef struct {
    HeapNode elements[MAX_NODES]; // Array-backed binary min-heap tree
    int size;                     // Current count of candidate vehicles in heap
    int capacity;                 // Max capacity
} MinHeap;

/*
 * Heap Lifecycle & Operations
 */
MinHeap* create_min_heap(int capacity);
void free_min_heap(MinHeap* heap);

/* Core Heap Mechanics (Built from scratch) */
void heapify_up(MinHeap* heap, int index);
void heapify_down(MinHeap* heap, int index);
bool heap_insert(MinHeap* heap, int ambulance_id, int distance, int current_node);
bool heap_extract_min(MinHeap* heap, HeapNode* out_min);
bool heap_is_empty(const MinHeap* heap);

/*
 * Ambulance State Transition Machine
 * Available (0) -> En Route (1) -> At Scene (2) -> Returning (3) -> Available (0)
 */
const char* get_status_string(AmbulanceStatus status);
bool transition_ambulance_status(int ambulance_id, AmbulanceStatus new_status);

#endif /* DISPATCH_HEAP_H */
