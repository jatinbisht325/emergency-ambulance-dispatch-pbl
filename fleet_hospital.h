#ifndef FLEET_HOSPITAL_H
#define FLEET_HOSPITAL_H

#include "common.h"

/*
 * ============================================================================
 * MODULE: FLEET, HOSPITAL MANAGEMENT & PERSISTENCE
 * Author: Mansi
 * Purpose: Manage dynamic ambulance records, hospital capacities, & disk I/O
 * ============================================================================
 */

/* Individual Ambulance Record */
typedef struct Ambulance {
    int id;                      // Unique vehicle ID (e.g. 101, 102)
    char vehicle_plate[32];      // Registration plate (e.g. "UK-04-AB-1234")
    char type[16];               // "ALS" (Advanced Life Support) or "BLS" (Basic)
    int current_node;            // Current intersection node ID
    AmbulanceStatus status;      // Current state (Available, En Route, etc.)
    int total_trips_completed;   // Operational metric
    struct Ambulance* next;      // Singly linked list pointer
} Ambulance;

/* Hospital Record */
typedef struct {
    int id;                      // Hospital ID
    int node_id;                 // Intersection ID on city road graph
    char name[MAX_STR_LEN];      // Hospital Name
    int total_beds;              // Overall capacity
    int available_beds;          // Current open beds
    int icu_beds_total;          // ICU capacity
    int icu_beds_available;      // Open ICU beds
    bool has_trauma_center;      // Trauma capability flag
} Hospital;

/* Fleet Manager (Linked List Head + Hospital Catalog) */
typedef struct {
    Ambulance* head;             // Head of dynamic singly linked list
    int total_ambulances;        // Active fleet count
    Hospital hospitals[MAX_NODES]; // Fixed catalog of regional medical centers
    int total_hospitals;         // Hospital count
} FleetManager;

/*
 * Lifecycle & Management
 */
FleetManager* create_fleet_manager(void);
void free_fleet_manager(FleetManager* fm);

/* Dynamic Singly Linked List Operations */
bool add_ambulance(FleetManager* fm, int id, const char* plate, const char* type, int start_node);
Ambulance* find_ambulance(FleetManager* fm, int id); // Returns safe NULL if not found
bool update_ambulance_location(FleetManager* fm, int id, int new_node, AmbulanceStatus new_status);
void print_fleet(const FleetManager* fm);

/* Hospital Capacity Operations */
Hospital* find_hospital_by_node(FleetManager* fm, int node_id);
bool admit_patient_to_hospital(FleetManager* fm, int hospital_id, bool requires_icu);
void print_hospitals(const FleetManager* fm);

/* Flat-File Persistence Engine (Disk I/O) */
bool load_fleet_from_file(FleetManager* fm, const char* filename);
bool save_fleet_to_file(const FleetManager* fm, const char* filename);
bool load_hospitals_from_file(FleetManager* fm, const char* filename);
bool save_hospitals_to_file(const FleetManager* fm, const char* filename);

#endif /* FLEET_HOSPITAL_H */
