#ifndef CALL_SYSTEM_H
#define CALL_SYSTEM_H

#include "common.h"

/*
 * ============================================================================
 * MODULE: CALL LOGGING & QUEUE/STACK ENGINE
 * Author: Mahima
 * Purpose: Manage incoming 112/999 calls (FIFO) & historical incident review (LIFO)
 * ============================================================================
 */

/* Emergency Call Record */
typedef struct EmergencyCall {
    int call_id;                 // Incremental incident ID
    int caller_node;             // Intersection ID where emergency occurred
    PriorityLevel severity;      // Critical, High, Medium, Low
    char description[128];       // Incident description (e.g. "Vehicular collision")
    int assigned_ambulance_id;   // Dispatched unit ID (-1 if waiting in queue)
    int assigned_hospital_id;    // Assigned receiving medical center
    int total_trip_distance;     // Total road distance calculated by Jatin's Dijkstra
    bool is_resolved;            // Resolution state flag
    struct EmergencyCall* next;  // Pointer for linked queue / stack
} EmergencyCall;

/* FIFO Queue for Incoming Calls (Strict chronological order) */
typedef struct {
    EmergencyCall* front;        // Head of queue (oldest pending call)
    EmergencyCall* rear;         // Tail of queue (newest incoming call)
    int count;                   // Number of pending calls
} CallQueue;

/* LIFO Stack for Incident History (Most recently resolved incident on top) */
typedef struct {
    EmergencyCall* top;          // Top of stack
    int count;                   // Total resolved incidents
} CallHistoryStack;

/*
 * Lifecycle Management
 */
CallQueue* create_call_queue(void);
void free_call_queue(CallQueue* q);

CallHistoryStack* create_call_stack(void);
void free_call_stack(CallHistoryStack* s);

/* FIFO Queue Operations */
bool enqueue_call(CallQueue* q, int call_id, int caller_node, PriorityLevel severity, const char* desc);
EmergencyCall* dequeue_call(CallQueue* q);
bool is_queue_empty(const CallQueue* q);
void print_pending_calls(const CallQueue* q);

/* LIFO Stack Operations */
bool push_history(CallHistoryStack* s, const EmergencyCall* resolved_call);
EmergencyCall* pop_history(CallHistoryStack* s);
void print_recent_history(const CallHistoryStack* s, int limit);

#endif /* CALL_SYSTEM_H */
