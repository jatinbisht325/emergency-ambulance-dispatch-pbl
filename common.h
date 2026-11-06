#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

/* Global System Limits */
#define MAX_NODES 50
#define MAX_STR_LEN 64
#define INF 9999999  // Safe infinity value that will not overflow during edge relaxation

/*
 * ============================================================================
 * AMBULANCE OPERATIONAL STATES (Kartik & Mansi)
 * 4-Phase Lifecycle: Available -> En Route -> At Scene -> Returning -> Available
 * ============================================================================
 */
typedef enum {
    STATUS_AVAILABLE = 0,   // Stationed at base / idle, ready for dispatch
    STATUS_EN_ROUTE   = 1,   // Dispatched and driving towards the patient
    STATUS_AT_SCENE   = 2,   // Arrived at accident location, administering aid
    STATUS_RETURNING  = 3    // Transporting patient to hospital or returning to base
} AmbulanceStatus;

/*
 * ============================================================================
 * EMERGENCY CALL SEVERITY (Mahima)
 * Used to prioritize dispatch when multiple calls arrive
 * ============================================================================
 */
typedef enum {
    SEVERITY_LOW      = 1,   // Non-critical, minor injury
    SEVERITY_MEDIUM   = 2,   // Moderate urgency
    SEVERITY_HIGH     = 3,   // Severe condition, rapid response required
    SEVERITY_CRITICAL = 4    // Life threatening, immediate priority dispatch
} PriorityLevel;

/*
 * ============================================================================
 * SHORTEST PATH ROUTING RESULT (Jatin - Graph & Routing Engine)
 * Returned by Dijkstra's algorithm and shared across all modules
 * ============================================================================
 */
typedef struct {
    int path[MAX_NODES];     // Turn-by-turn sequence of node IDs (e.g., [0, 2, 5, 8])
    int path_length;         // Number of intersections along this path
    int total_distance;      // Cumulative road distance (in km or travel units)
    bool is_reachable;       // True if a valid road path exists, false if disconnected
} RouteResult;

#endif /* COMMON_H */
