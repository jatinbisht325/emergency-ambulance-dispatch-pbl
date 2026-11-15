#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "call_system.h"

/*
 * ============================================================================
 * MODULE: EMERGENCY CALL INTAKE & INCIDENT AUDIT (IMPLEMENTATION)
 * Author: Mahima (Refactored by Jatin for C99 Compliance & System Integration)
 *
 * Data Structures:
 * 1. FIFO Queue: Strict First-Come First-Served emergency intake triage (O(1) enqueue/dequeue)
 * 2. LIFO Stack: Reverse chronological incident audit history (O(1) push/pop)
 * ============================================================================
 */

/* Helper to convert priority level enum to formatted string */
static const char* get_severity_string(PriorityLevel level) {
    switch (level) {
        case SEVERITY_LOW:      return "LOW (Minor)";
        case SEVERITY_MEDIUM:   return "MEDIUM (Urgent)";
        case SEVERITY_HIGH:     return "HIGH (Severe)";
        case SEVERITY_CRITICAL: return "CRITICAL (Immediate)";
        default:                return "STANDARD";
    }
}

/* ----------------------------------------------------------------------------
 * 1. LIFECYCLE MANAGEMENT
 * ---------------------------------------------------------------------------- */

/* Allocates and initializes an empty FIFO call queue */
CallQueue* create_call_queue(void) {
    CallQueue* q = (CallQueue*)malloc(sizeof(CallQueue));
    if (!q) {
        fprintf(stderr, "[FATAL] Memory allocation failed for CallQueue.\n");
        return NULL;
    }
    q->front = NULL;
    q->rear = NULL;
    q->count = 0;
    return q;
}

/* Deallocates every pending call node and frees the queue structure */
void free_call_queue(CallQueue* q) {
    if (!q) return;

    EmergencyCall* current = q->front;
    while (current != NULL) {
        EmergencyCall* temp = current;
        current = current->next;
        free(temp);
    }
    free(q);
}

/* Allocates and initializes an empty LIFO history stack */
CallHistoryStack* create_call_stack(void) {
    CallHistoryStack* s = (CallHistoryStack*)malloc(sizeof(CallHistoryStack));
    if (!s) {
        fprintf(stderr, "[FATAL] Memory allocation failed for CallHistoryStack.\n");
        return NULL;
    }
    s->top = NULL;
    s->count = 0;
    return s;
}

/* Deallocates every historical call node and frees the stack structure */
void free_call_stack(CallHistoryStack* s) {
    if (!s) return;

    EmergencyCall* current = s->top;
    while (current != NULL) {
        EmergencyCall* temp = current;
        current = current->next;
        free(temp);
    }
    free(s);
}

/* ----------------------------------------------------------------------------
 * 2. FIFO QUEUE OPERATIONS (Incoming 112 / 999 Calls)
 * ---------------------------------------------------------------------------- */

/*
 * Inserts a newly received emergency call at the rear of the FIFO queue.
 * Time Complexity: O(1)
 */
bool enqueue_call(CallQueue* q, int call_id, int caller_node, PriorityLevel severity, const char* desc) {
    if (!q) {
        fprintf(stderr, "[ERROR] enqueue_call: Queue pointer is NULL.\n");
        return false;
    }

    EmergencyCall* new_call = (EmergencyCall*)malloc(sizeof(EmergencyCall));
    if (!new_call) {
        fprintf(stderr, "[FATAL] Memory allocation failed for new EmergencyCall node.\n");
        return false;
    }

    new_call->call_id = call_id;
    new_call->caller_node = caller_node;
    new_call->severity = severity;

    strncpy(new_call->description, desc ? desc : "Emergency Assistance Request", sizeof(new_call->description) - 1);
    new_call->description[sizeof(new_call->description) - 1] = '\0';

    new_call->assigned_ambulance_id = -1;
    new_call->assigned_hospital_id = -1;
    new_call->total_trip_distance = 0;
    new_call->is_resolved = false;
    new_call->next = NULL;

    /* Enqueue at rear */
    if (q->rear == NULL) {
        /* Empty queue: new node is both front and rear */
        q->front = new_call;
        q->rear = new_call;
    } else {
        q->rear->next = new_call;
        q->rear = new_call;
    }

    q->count++;
    return true;
}

/*
 * Removes and returns the oldest call from the front of the FIFO queue.
 * Time Complexity: O(1)
 * Caller takes ownership of the returned node memory.
 */
EmergencyCall* dequeue_call(CallQueue* q) {
    if (!q || q->front == NULL) {
        return NULL;
    }

    EmergencyCall* dequeued = q->front;
    q->front = q->front->next;

    /* If queue is now empty, reset rear pointer as well */
    if (q->front == NULL) {
        q->rear = NULL;
    }

    q->count--;
    dequeued->next = NULL; /* Detach link */
    return dequeued;
}

/* Returns true if no calls are currently waiting in the intake queue */
bool is_queue_empty(const CallQueue* q) {
    return (!q || q->count == 0);
}

/* Displays all pending incoming calls awaiting ambulance dispatch */
void print_pending_calls(const CallQueue* q) {
    if (!q || q->front == NULL) {
        printf("\n[CALL INTAKE QUEUE IS EMPTY - ALL EMERGENCIES SERVICED]\n");
        return;
    }

    printf("\n===============================================================================\n");
    printf("                    PENDING EMERGENCY CALLS (FIFO QUEUE)                       \n");
    printf("===============================================================================\n");
    printf(" %-8s  %-6s  %-20s  %-36s\n",
           "Call ID", "Node", "Severity Level", "Description");
    printf("-------------------------------------------------------------------------------\n");

    EmergencyCall* current = q->front;
    while (current != NULL) {
        printf(" #%-7d  [%02d]    %-20s  %-36s\n",
               current->call_id,
               current->caller_node,
               get_severity_string(current->severity),
               current->description);
        current = current->next;
    }

    printf("-------------------------------------------------------------------------------\n");
    printf(" Total Pending Calls: %d\n", q->count);
    printf("===============================================================================\n\n");
}

/* ----------------------------------------------------------------------------
 * 3. LIFO STACK OPERATIONS (Resolved Incident History Audit)
 * ---------------------------------------------------------------------------- */

/*
 * Pushes a resolved incident record onto the top of the history stack.
 * Performs a deep copy so the caller can safely free its local copy.
 * Time Complexity: O(1)
 */
bool push_history(CallHistoryStack* s, const EmergencyCall* resolved_call) {
    if (!s || !resolved_call) {
        return false;
    }

    EmergencyCall* history_node = (EmergencyCall*)malloc(sizeof(EmergencyCall));
    if (!history_node) {
        fprintf(stderr, "[FATAL] Memory allocation failed for history stack entry.\n");
        return false;
    }

    /* Copy all record fields */
    *history_node = *resolved_call;
    history_node->is_resolved = true;

    /* Top insertion: new node points to current top, becomes new top */
    history_node->next = s->top;
    s->top = history_node;
    s->count++;

    return true;
}

/*
 * Pops the most recently resolved incident off the top of the stack.
 * Time Complexity: O(1)
 * Caller takes ownership of the returned node memory.
 */
EmergencyCall* pop_history(CallHistoryStack* s) {
    if (!s || s->top == NULL) {
        return NULL;
    }

    EmergencyCall* popped = s->top;
    s->top = s->top->next;
    s->count--;
    popped->next = NULL;

    return popped;
}

/*
 * Displays the latest 'limit' resolved incident reports in reverse chronological order.
 */
void print_recent_history(const CallHistoryStack* s, int limit) {
    if (!s || s->top == NULL) {
        printf("\n[NO RESOLVED INCIDENTS IN AUDIT HISTORY]\n");
        return;
    }

    printf("\n===============================================================================\n");
    printf("                 INCIDENT AUDIT LOG (LIFO HISTORY STACK)                       \n");
    printf("===============================================================================\n");
    printf(" %-8s  %-6s  %-10s  %-12s  %-10s  %-20s\n",
           "Call ID", "Scene", "Ambulance", "Hospital ID", "Total Trip", "Description");
    printf("-------------------------------------------------------------------------------\n");

    EmergencyCall* current = s->top;
    int printed = 0;

    while (current != NULL && (limit <= 0 || printed < limit)) {
        printf(" #%-7d  [%02d]    Amb #%-5d  Hosp #%-6d  %-3d km      %-20s\n",
               current->call_id,
               current->caller_node,
               current->assigned_ambulance_id,
               current->assigned_hospital_id,
               current->total_trip_distance,
               current->description);

        current = current->next;
        printed++;
    }

    printf("-------------------------------------------------------------------------------\n");
    printf(" Showing %d of %d total completed emergency incidents.\n", printed, s->count);
    printf("===============================================================================\n\n");
}
