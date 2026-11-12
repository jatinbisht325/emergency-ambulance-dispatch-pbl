#include "call_system.h"

/* Create a new empty Call Queue */
CallQueue* create_call_queue(void)
{
    CallQueue* q = (CallQueue*)malloc(sizeof(CallQueue));

    if (q == NULL)
        return NULL;

    q->front = NULL;
    q->rear = NULL;
    q->count = 0;

    return q;
}

/* Create a new empty History Stack */
CallHistoryStack* create_call_stack(void)
{
    CallHistoryStack* s =
        (CallHistoryStack*)malloc(sizeof(CallHistoryStack));

    if (s == NULL)
        return NULL;

    s->top = NULL;
    s->count = 0;

    return s;
}

/* Check whether Queue is empty */
bool is_queue_empty(const CallQueue* q)
{
    return (q == NULL || q->front == NULL);
}

/* Insert a new emergency call into Queue */
bool enqueue_call(CallQueue* q, int call_id,
                  int caller_node, PriorityLevel severity,
                  const char* desc)
{
    if (q == NULL || desc == NULL)
        return false;

    EmergencyCall* newCall =
        (EmergencyCall*)malloc(sizeof(EmergencyCall));

    if (newCall == NULL)
        return false;

    newCall->call_id = call_id;
    newCall->caller_node = caller_node;
    newCall->severity = severity;

    strncpy(newCall->description, desc,
            sizeof(newCall->description) - 1);

    newCall->description[sizeof(newCall->description) - 1] = '\0';

    newCall->assigned_ambulance_id = -1;
    newCall->assigned_hospital_id = -1;
    newCall->total_trip_distance = 0;
    newCall->is_resolved = false;
    newCall->next = NULL;

    /* If Queue is empty */
    if (q->rear == NULL)
    {
        q->front = newCall;
        q->rear = newCall;
    }
    else
    {
        q->rear->next = newCall;
        q->rear = newCall;
    }

    q->count++;

    printf("\nEmergency Call Added Successfully!\n");

    return true;
}

/* Remove the oldest emergency call */
EmergencyCall* dequeue_call(CallQueue* q)
{
    if (is_queue_empty(q))
    {
        printf("\nNo Pending Emergency Calls!\n");
        return NULL;
    }

    EmergencyCall* temp = q->front;

    q->front = q->front->next;

    if (q->front == NULL)
        q->rear = NULL;

    temp->next = NULL;
    q->count--;

    return temp;
}

/* Display all pending emergency calls */
void print_pending_calls(const CallQueue* q)
{
    if (is_queue_empty(q))
    {
        printf("\nNo Pending Calls!\n");
        return;
    }

    EmergencyCall* temp = q->front;

    printf("\n========== PENDING CALLS ==========\n");

    while (temp != NULL)
    {
        printf("\nCall ID: %d", temp->call_id);
        printf("\nCaller Location: %d", temp->caller_node);
        printf("\nSeverity: %d", temp->severity);
        printf("\nDescription: %s", temp->description);

        printf("\n-----------------------------------");

        temp = temp->next;
    }

    printf("\nTotal Pending Calls: %d\n", q->count);
}

/* Push completed call into History Stack */
bool push_history(CallHistoryStack* s,
                  const EmergencyCall* resolved_call)
{
    if (s == NULL || resolved_call == NULL)
        return false;

    EmergencyCall* newCall =
        (EmergencyCall*)malloc(sizeof(EmergencyCall));

    if (newCall == NULL)
        return false;

    *newCall = *resolved_call;

    newCall->is_resolved = true;

    newCall->next = s->top;

    s->top = newCall;

    s->count++;

    printf("\nCall Added To History Successfully!\n");

    return true;
}

/* Remove the most recent call from History Stack */
EmergencyCall* pop_history(CallHistoryStack* s)
{
    if (s == NULL || s->top == NULL)
    {
        printf("\nHistory Stack Is Empty!\n");
        return NULL;
    }

    EmergencyCall* temp = s->top;

    s->top = s->top->next;

    temp->next = NULL;

    s->count--;

    return temp;
}

/* Display recent resolved calls */
void print_recent_history(const CallHistoryStack* s, int limit)
{
    if (s == NULL || s->top == NULL || limit <= 0)
    {
        printf("\nNo Call History Available!\n");
        return;
    }

    EmergencyCall* temp = s->top;
    int displayed = 0;

    printf("\n========== CALL HISTORY ==========\n");

    while (temp != NULL && displayed < limit)
    {
        printf("\nCall ID: %d", temp->call_id);
        printf("\nLocation: %d", temp->caller_node);
        printf("\nDescription: %s", temp->description);
        printf("\nResolved: %s",
               temp->is_resolved ? "Yes" : "No");

        printf("\n-----------------------------------");

        temp = temp->next;
        displayed++;
    }
}

/* Free all memory used by Queue */
void free_call_queue(CallQueue* q)
{
    if (q == NULL)
        return;

    EmergencyCall* temp;

    while (q->front != NULL)
    {
        temp = q->front;
        q->front = q->front->next;
        free(temp);
    }

    free(q);
}

/* Free all memory used by History Stack */
void free_call_stack(CallHistoryStack* s)
{
    if (s == NULL)
        return;

    EmergencyCall* temp;

    while (s->top != NULL)
    {
        temp = s->top;
        s->top = s->top->next;
        free(temp);
    }

    free(s);
}
