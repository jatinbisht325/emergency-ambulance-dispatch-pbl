/*
 * fleet_hospital.h
 * Module owner : Mansi
 * Purpose      : Dynamic fleet (singly linked list), hospital bed tracker,
 *                and flat-file persistence (fleet.txt, hospitals.txt).
 *
 * NOTE: Match type names with common.h. If common.h already defines
 *       AmbulanceStatus, remove the enum below and use that one.
 */

#ifndef FLEET_HOSPITAL_H
#define FLEET_HOSPITAL_H

#include <stdio.h>

/* ------------------------------------------------------------------ */
/* Constants                                                           */
/* ------------------------------------------------------------------ */
#define MAX_NAME_LEN     50
#define FLEET_FILE       "fleet.txt"
#define HOSPITAL_FILE    "hospitals.txt"

/* Return codes (used by functions that can fail) */
#define FH_OK              0
#define FH_ERR_NULL       -1   /* NULL pointer passed in            */
#define FH_ERR_MEMORY     -2   /* malloc failed                     */
#define FH_ERR_DUPLICATE  -3   /* ID already exists                 */
#define FH_ERR_NOT_FOUND  -4   /* ID not found                      */
#define FH_ERR_INVALID    -5   /* invalid value (status, beds, ...) */
#define FH_ERR_FILE       -6   /* file could not be opened/written  */
#define FH_ERR_NO_BEDS    -7   /* hospital has no free beds         */

/* ------------------------------------------------------------------ */
/* Ambulance status (4-phase cycle owned by Kartik's state machine)    */
/* ------------------------------------------------------------------ */
typedef enum {
    STATUS_AVAILABLE = 0,
    STATUS_EN_ROUTE  = 1,
    STATUS_AT_SCENE  = 2,
    STATUS_RETURNING = 3
} FleetStatus;

/* ------------------------------------------------------------------ */
/* Fleet: singly linked list of ambulances                             */
/* ------------------------------------------------------------------ */
typedef struct AmbulanceNode {
    int                   id;          /* unique ambulance ID           */
    int                   location;    /* graph node where it is parked */
    FleetStatus           status;      /* current phase                 */
    int                   type;        /* 0 = basic, 1 = ICU/advanced   */
    struct AmbulanceNode *next;        /* link to next ambulance        */
} AmbulanceNode;

typedef struct {
    AmbulanceNode *head;               /* first ambulance (NULL if none)*/
    int            count;              /* number of ambulances          */
} Fleet;

/* ------------------------------------------------------------------ */
/* Hospitals: singly linked list of hospital records                   */
/* ------------------------------------------------------------------ */
typedef struct HospitalNode {
    int                  id;
    char                 name[MAX_NAME_LEN];
    int                  location;       /* graph node of the hospital   */
    int                  total_beds;
    int                  available_beds;
    int                  icu_ready;      /* 1 = has ICU, 0 = no          */
    int                  trauma_center;  /* 1 = trauma center, 0 = no    */
    struct HospitalNode *next;
} HospitalNode;

typedef struct {
    HospitalNode *head;
    int           count;
} HospitalList;

/* ================================================================== */
/* FLEET FUNCTIONS                                                     */
/* ================================================================== */

/* Create an empty fleet. */
void fleet_init(Fleet *fleet);

/* Add an ambulance at the head of the list (O(1)).
 * Returns FH_OK, FH_ERR_NULL, FH_ERR_DUPLICATE, FH_ERR_INVALID or
 * FH_ERR_MEMORY. */
int fleet_add(Fleet *fleet, int id, int location, FleetStatus status, int type);

/* Remove the ambulance with this ID and free its node (O(n)).
 * Returns FH_OK or FH_ERR_NOT_FOUND. */
int fleet_remove(Fleet *fleet, int id);

/* Find an ambulance by ID. Returns pointer, or NULL if not found. */
AmbulanceNode *fleet_find(const Fleet *fleet, int id);

/* Change an ambulance's status. Returns FH_OK, FH_ERR_NOT_FOUND or
 * FH_ERR_INVALID. */
int fleet_update_status(Fleet *fleet, int id, FleetStatus new_status);

/* Count ambulances with a given status. */
int fleet_count_by_status(const Fleet *fleet, FleetStatus status);

/* Print every ambulance in a readable table. */
void fleet_print(const Fleet *fleet);

/* Free every node and reset the fleet to empty. */
void fleet_free(Fleet *fleet);

/* ================================================================== */
/* HOSPITAL FUNCTIONS                                                  */
/* ================================================================== */

void hospital_list_init(HospitalList *list);

/* Add a hospital. Returns FH_OK, FH_ERR_NULL, FH_ERR_DUPLICATE,
 * FH_ERR_INVALID (beds < 0 or available > total) or FH_ERR_MEMORY. */
int hospital_add(HospitalList *list, int id, const char *name, int location,
                 int total_beds, int available_beds,
                 int icu_ready, int trauma_center);

/* Find a hospital by ID. Returns pointer, or NULL if not found. */
HospitalNode *hospital_find(const HospitalList *list, int id);

/* Pick a hospital for a patient.
 *   need_icu    : 1 if the patient needs ICU, else 0
 *   need_trauma : 1 if the patient needs a trauma center, else 0
 * Only hospitals with available_beds > 0 and matching needs qualify.
 * Returns the matching hospital, or NULL if none qualifies. */
HospitalNode *hospital_find_suitable(const HospitalList *list,
                                     int need_icu, int need_trauma);

/* Reserve one bed. Returns FH_OK, FH_ERR_NOT_FOUND or FH_ERR_NO_BEDS. */
int hospital_admit(HospitalList *list, int id);

/* Free one bed (never above total_beds). Returns FH_OK or
 * FH_ERR_NOT_FOUND. */
int hospital_discharge(HospitalList *list, int id);

/* Print every hospital in a readable table. */
void hospital_print(const HospitalList *list);

/* Free every node and reset the list to empty. */
void hospital_list_free(HospitalList *list);

/* ================================================================== */
/* FILE PERSISTENCE (flat .txt files, no database)                     */
/* ================================================================== */

/* Save the whole fleet to fleet.txt (one ambulance per line).
 * Returns FH_OK or FH_ERR_FILE. */
int fleet_save(const Fleet *fleet, const char *filename);

/* Load the fleet from fleet.txt. If the file does not exist, the fleet
 * stays empty and FH_OK is returned. Bad lines are skipped with a
 * warning. Returns FH_OK or FH_ERR_FILE. */
int fleet_load(Fleet *fleet, const char *filename);

/* Same pair of functions for hospitals.txt. */
int hospital_save(const HospitalList *list, const char *filename);
int hospital_load(HospitalList *list, const char *filename);

#endif /* FLEET_HOSPITAL_H */
