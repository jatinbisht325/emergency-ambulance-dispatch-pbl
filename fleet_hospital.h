#ifndef FLEET_HOSPITAL_H
#define FLEET_HOSPITAL_H

#include "common.h"

/*
 * ============================================================================
 * MODULE: AMBULANCE FLEET & HOSPITAL CAPACITY MANAGEMENT
 * Author: Mansi (Refactored by Jatin for C99 Compliance & System Integration)
 *
 * Data Structures Used:
 * 1. Singly Linked List for Ambulance Fleet (Dynamic vehicle additions, O(1) head insert)
 * 2. Singly Linked List for Hospital Directory & Real-time Bed/ICU Tracker
 * 3. Flat-File Persistence Engine for Disk I/O (fleet.txt & hospitals.txt)
 * ============================================================================
 */

/* ----------------------------------------------------------------------------
 * 1. AMBULANCE FLEET DATA STRUCTURES
 * ---------------------------------------------------------------------------- */

/* Singly linked list node representing one active ambulance vehicle */
typedef struct AmbulanceNode {
    int id;                         /* Unique Ambulance ID (e.g. 101, 102) */
    char plate[MAX_STR_LEN];        /* License plate string (e.g. "UK-04-A-1234") */
    char type[MAX_STR_LEN];         /* Vehicle tier: "Basic Life Support" or "Advanced ICU" */
    int current_node;               /* Current intersection where vehicle is parked */
    AmbulanceStatus status;         /* AVAILABLE, EN_ROUTE, AT_SCENE, RETURNING */
    int total_trips;                /* Counter for missions completed */
    struct AmbulanceNode* next;     /* Pointer to next vehicle node in the linked list */
} AmbulanceNode;

/* Master head wrapper for the fleet linked list */
typedef struct {
    AmbulanceNode* head;            /* Head pointer to first ambulance node */
    int count;                      /* Total number of ambulances in fleet */
} Fleet;

/* ----------------------------------------------------------------------------
 * 2. HOSPITAL MANAGEMENT DATA STRUCTURES
 * ---------------------------------------------------------------------------- */

/* Singly linked list node representing a city hospital */
typedef struct HospitalNode {
    int id;                         /* Unique Hospital ID */
    int node_id;                    /* Intersection ID on city map where hospital is located */
    char name[MAX_STR_LEN];         /* Hospital Name (e.g. "District Civil Hospital") */
    int total_beds;                 /* Total regular bed capacity */
    int available_beds;             /* Currently available regular beds */
    int icu_beds_total;             /* Total ICU bed capacity */
    int icu_beds_available;         /* Currently available ICU beds */
    bool has_trauma_center;         /* True if hospital has advanced emergency trauma unit */
    struct HospitalNode* next;      /* Pointer to next hospital node */
} HospitalNode;

/* Master head wrapper for the hospitals linked list */
typedef struct {
    HospitalNode* head;             /* Head pointer to first hospital node */
    int count;                      /* Total registered hospitals */
} HospitalList;

/* ----------------------------------------------------------------------------
 * 3. FLEET OPERATIONS (Singly Linked List)
 * ---------------------------------------------------------------------------- */
Fleet* create_fleet(void);
void free_fleet(Fleet* fleet);

/* O(1) Insertion at the head of the list */
bool add_ambulance(Fleet* fleet, int id, const char* plate, const char* type, int start_node);

/* O(N) Defensive linear search */
AmbulanceNode* find_ambulance(const Fleet* fleet, int id);

/* Status & Location Updates */
bool update_ambulance_status(Fleet* fleet, int id, AmbulanceStatus new_status);
bool update_ambulance_location(Fleet* fleet, int id, int new_node);

/* Visual Display */
void print_fleet(const Fleet* fleet);
int count_available_ambulances(const Fleet* fleet);

/* ----------------------------------------------------------------------------
 * 4. HOSPITAL OPERATIONS
 * ---------------------------------------------------------------------------- */
HospitalList* create_hospital_list(void);
void free_hospital_list(HospitalList* list);

bool add_hospital(HospitalList* list, int id, int node_id, const char* name,
                 int total_beds, int available_beds,
                 int icu_total, int icu_avail, bool has_trauma);

HospitalNode* find_hospital_by_id(const HospitalList* list, int id);
HospitalNode* find_hospital_by_node(const HospitalList* list, int node_id);
HospitalNode* find_suitable_hospital(const HospitalList* list, bool need_icu, bool need_trauma);

/* Capacity Allocation & Discharge */
bool admit_patient_to_hospital(HospitalList* list, int hospital_id, bool requires_icu);
bool discharge_patient_from_hospital(HospitalList* list, int hospital_id, bool had_icu);

void print_hospitals(const HospitalList* list);

/* ----------------------------------------------------------------------------
 * 5. FLAT-FILE DISK PERSISTENCE ENGINE
 * ---------------------------------------------------------------------------- */
bool load_fleet_from_file(Fleet* fleet, const char* filename);
bool save_fleet_to_file(const Fleet* fleet, const char* filename);

bool load_hospitals_from_file(HospitalList* list, const char* filename);
bool save_hospitals_to_file(const HospitalList* list, const char* filename);

#endif /* FLEET_HOSPITAL_H */
