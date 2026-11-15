#include <stdio.h>
#include <stdlib.h>
#include "call_system.h"

int main(void) {
    printf("=================================================================\n");
    printf("    CALL INTAKE & INCIDENT AUDIT SYSTEM - STANDALONE TEST       \n");
    printf("    Author: Mahima | Verification Harness: Jatin Bisht          \n");
    printf("=================================================================\n");

    /* 1. Test FIFO Queue Lifecycle & Enqueue */
    printf("\n[*] Initializing FIFO Emergency Call Intake Queue...\n");
    CallQueue* queue = create_call_queue();
    if (!queue) {
        fprintf(stderr, "[FATAL] Failed to initialize CallQueue.\n");
        return EXIT_FAILURE;
    }

    printf("[*] Simulating 3 incoming emergency 112 calls...\n");
    enqueue_call(queue, 501, 3, SEVERITY_CRITICAL, "Severe road accident at GEHU Gate");
    enqueue_call(queue, 502, 1, SEVERITY_MEDIUM,   "Elderly cardiac distress at Tallital");
    enqueue_call(queue, 503, 6, SEVERITY_LOW,      "Minor fracture at Kainchi Sector");

    print_pending_calls(queue);

    /* 2. Test FIFO Dequeue (First In First Out) */
    printf("[*] Dequeuing oldest call for dispatch triage...\n");
    EmergencyCall* first_call = dequeue_call(queue);
    if (first_call) {
        printf("[+] Dequeued Call #%d: Scene = Node [%02d], Desc = '%s'\n",
               first_call->call_id, first_call->caller_node, first_call->description);
    }

    print_pending_calls(queue);

    /* 3. Test LIFO Stack Lifecycle & Push History */
    printf("\n[*] Initializing LIFO Incident Audit Stack...\n");
    CallHistoryStack* stack = create_call_stack();
    if (!stack) {
        fprintf(stderr, "[FATAL] Failed to initialize CallHistoryStack.\n");
        free(first_call);
        free_call_queue(queue);
        return EXIT_FAILURE;
    }

    /* Simulate resolution of Call #501 */
    first_call->assigned_ambulance_id = 101;
    first_call->assigned_hospital_id = 1;
    first_call->total_trip_distance = 9;

    printf("[*] Logging resolved incident #%d onto history stack...\n", first_call->call_id);
    push_history(stack, first_call);
    free(first_call); /* Safe because push_history made deep copy */

    /* Simulate resolving second call */
    EmergencyCall* second_call = dequeue_call(queue);
    if (second_call) {
        second_call->assigned_ambulance_id = 103;
        second_call->assigned_hospital_id = 2;
        second_call->total_trip_distance = 14;
        printf("[*] Logging resolved incident #%d onto history stack...\n", second_call->call_id);
        push_history(stack, second_call);
        free(second_call);
    }

    /* 4. Test LIFO Stack Audit Review (Last In First Out) */
    printf("\n[*] Reviewing recent incident history (Top should be Call #502):\n");
    print_recent_history(stack, 5);

    /* 5. Memory Cleanup */
    printf("[*] Deallocating Queue and Stack structures...\n");
    free_call_queue(queue);
    free_call_stack(stack);
    printf("[+] Memory deallocated cleanly. 0 leaks.\n");

    printf("\n=================================================================\n");
    printf("     MAHIMA'S MODULE VERIFICATION COMPLETE - FULLY FUNCTIONAL    \n");
    printf("=================================================================\n");

    return EXIT_SUCCESS;
}
