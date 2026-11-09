#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fleet_hospital.h"

/*
 * ============================================================================
 * MODULE: FLEET, HOSPITAL MANAGEMENT & PERSISTENCE (IMPLEMENTATION)
 * Author: Mansi
 * ============================================================================
 */

/* 
 * 1. LIFECYCLE MANAGEMENT 
 */

FleetManager* create_fleet_manager(void) {
    FleetManager* fm = (FleetManager*)malloc(sizeof(FleetManager));
    if (!fm) {
        printf("Error: Memory allocation failed for FleetManager.\n");
        return NULL;
    }
    fm->head = NULL;
    fm->total_ambulances = 0;
    fm->total_hospitals = 0;
    return fm;
}

void free_fleet_manager(FleetManager* fm) {
    if (!fm) return;

    // Free the singly linked list of ambulances
    Ambulance* current = fm->head;
    while (current != NULL) {
        Ambulance* temp = current;
        current = current->next;
        free(temp);
    }

    // Free the manager structure itself
    free(fm);
}


/* 
 * 2. DYNAMIC SINGLY LINKED LIST OPERATIONS (Ambulance Fleet) 
 */

bool add_ambulance(FleetManager* fm, int id, const char* plate, const char* type, int start_node) {
    if (!fm) return false;

    // Check if ambulance ID already exists
    if (find_ambulance(fm, id) != NULL) {
        printf("Error: Ambulance with ID %d already exists.\n", id);
        return false;
    }

    Ambulance* new_ambulance = (Ambulance*)malloc(sizeof(Ambulance));
    if (!new_ambulance) {
        printf("Error: Memory allocation failed for new ambulance.\n");
        return false;
    }

    new_ambulance->id = id;
    strncpy(new_ambulance->vehicle_plate, plate, sizeof(new_ambulance->vehicle_plate) - 1);
    new_ambulance->vehicle_plate[sizeof(new_ambulance->vehicle_plate) - 1] = '\0';

    strncpy(new_ambulance->type, type, sizeof(new_ambulance->type) - 1);
    new_ambulance->type[sizeof(new_ambulance->type) - 1] = '\0';

    new_ambulance->current_node = start_node;
    new_ambulance->status = AMBULANCE_AVAILABLE; // Assuming AMBULANCE_AVAILABLE is defined in common.h
    new_ambulance->total_trips_completed = 0;

    // Insert at the head of the linked list (O(1) insertion)
    new_ambulance->next = fm->head;
    fm->head = new_ambulance;
    fm->total_ambulances++;

    return true;
}

Ambulance* find_ambulance(FleetManager* fm, int id) {
    if (!fm) return NULL;

    Ambulance* current = fm->head;
    while (current != NULL) {
        if (current->id == id) {
            return current;
        }
        current = current->next;
    }
    return NULL; // Safe NULL return if not found
}

bool update_ambulance_location(FleetManager* fm, int id, int new_node, AmbulanceStatus new_status) {
    Ambulance* amb = find_ambulance(fm, id);
    if (!amb) {
        printf("Error: Ambulance ID %d not found for update.\n", id);
        return false;
    }

    amb->current_node = new_node;
    amb->status = new_status;
    if (new_status == AMBULANCE_AVAILABLE) {
        amb->total_trips_completed++;
    }
    return true;
}

void print_fleet(const FleetManager* fm) {
    if (!fm || !fm->head) {
        printf("\n[Fleet is Empty]\n");
        return;
    }

    printf("\n=================== ACTIVE AMBULANCE FLEET ===================\n");
    printf("ID\tPlate Number\tType\tNode\tStatus\t\tTrips\n");
    printf("--------------------------------------------------------------\n");
    
    Ambulance* current = fm->head;
    while (current != NULL) {
        printf("%d\t%s\t%s\t%d\t%d\t\t%d\n", 
            current->id, 
            current->vehicle_plate, 
            current->type, 
            current->current_node, 
            current->status, 
            current->total_trips_completed);
        current = current->next;
    }
    printf("Total Ambulances: %d\n", fm->total_ambulances);
    printf("==============================================================\n");
}


/* 
 * 3. HOSPITAL CAPACITY OPERATIONS 
 */

Hospital* find_hospital_by_node(FleetManager* fm, int node_id) {
    if (!fm) return NULL;

    for (int i = 0; i < fm->total_hospitals; i++) {
        if (fm->hospitals[i].node_id == node_id) {
            return &fm->hospitals[i];
        }
    }
    return NULL;
}

bool admit_patient_to_hospital(FleetManager* fm, int hospital_id, bool requires_icu) {
    if (!fm) return false;

    Hospital* target_hospital = NULL;
    for (int i = 0; i < fm->total_hospitals; i++) {
        if (fm->hospitals[i].id == hospital_id) {
            target_hospital = &fm->hospitals[i];
            break;
        }
    }

    if (!target_hospital) {
        printf("Error: Hospital ID %d not found.\n", hospital_id);
        return false;
    }

    if (requires_icu) {
        if (target_hospital->icu_beds_available > 0) {
            target_hospital->icu_beds_available--;
            target_hospital->available_beds--; // ICU beds usually contribute to total beds
            return true;
        } else {
            printf("Notice: No ICU beds available at hospital %s.\n", target_hospital->name);
            return false;
        }
    } else {
        if (target_hospital->available_beds > 0) {
            target_hospital->available_beds--;
            return true;
        } else {
            printf("Notice: No general beds available at hospital %s.\n", target_hospital->name);
            return false;
        }
    }
}

void print_hospitals(const FleetManager* fm) {
    if (!fm || fm->total_hospitals == 0) {
        printf("\n[No Hospitals Registered]\n");
        return;
    }

    printf("\n======================= REGISTERED HOSPITALS =======================\n");
    printf("ID\tNode\tName\t\t\tBeds (Avail/Total)\tICU (Avail/Total)\tTrauma\n");
    printf("--------------------------------------------------------------------\n");

    for (int i = 0; i < fm->total_hospitals; i++) {
        Hospital h = fm->hospitals[i];
        printf("%d\t%d\t%-15s\t%d/%d\t\t\t%d/%d\t\t\t%s\n",
            h.id, h.node_id, h.name, 
            h.available_beds, h.total_beds, 
            h.icu_beds_available, h.icu_beds_total,
            h.has_trauma_center ? "Yes" : "No");
    }
    printf("====================================================================\n");
}


/* 
 * 4. FLAT-FILE PERSISTENCE ENGINE (Disk I/O) 
 */

bool load_fleet_from_file(FleetManager* fm, const char* filename) {
    if (!fm) return false;

    FILE* file = fopen(filename, "r");
    if (!file) {
        printf("Warning: Could not open fleet file %s for reading.\n", filename);
        return false;
    }

    int id, node, status, trips;
    char plate[32], type[16];

    // Assuming format per line: id plate type current_node status trips
    while (fscanf(file, "%d %31s %15s %d %d %d", &id, plate, type, &node, &status, &trips) == 6) {
        add_ambulance(fm, id, plate, type, node);
        // Fix status and trips if added with defaults
        Ambulance* amb = find_ambulance(fm, id);
        if (amb) {
            amb->status = (AmbulanceStatus)status;
            amb->total_trips_completed = trips;
        }
    }

    fclose(file);
    return true;
}

bool save_fleet_to_file(const FleetManager* fm, const char* filename) {
    if (!fm) return false;

    FILE* file = fopen(filename, "w");
    if (!file) {
        printf("Error: Could not open fleet file %s for writing.\n", filename);
        return false;
    }

    Ambulance* current = fm->head;
    while (current != NULL) {
        fprintf(file, "%d %s %s %d %d %d\n",
            current->id,
            current->vehicle_plate,
            current->type,
            current->current_node,
            (int)current->status,
            current->total_trips_completed);
        current = current->next;
    }

    fclose(file);
    return true;
}

bool load_hospitals_from_file(FleetManager* fm, const char* filename) {
    if (!fm) return false;

    FILE* file = fopen(filename, "r");
    if (!file) {
        printf("Warning: Could not open hospital file %s for reading.\n", filename);
        return false;
    }

    fm->total_hospitals = 0;
    Hospital h;
    int trauma_flag;

    // Format per line: id node_id name total_beds available_beds icu_total icu_avail trauma_flag
    while (fscanf(file, "%d %d %255s %d %d %d %d %d", 
        &h.id, &h.node_id, h.name, 
        &h.total_beds, &h.available_beds, 
        &h.icu_beds_total, &h.icu_beds_available, 
        &trauma_flag) == 8) {
        
        h.has_trauma_center = (bool)trauma_flag;
        if (fm->total_hospitals < MAX_NODES) {
            fm->hospitals[fm->total_hospitals++] = h;
        }
    }

    fclose(file);
    return true;
}

bool save_hospitals_to_file(const FleetManager* fm, const char* filename) {
    if (!fm) return false;

    FILE* file = fopen(filename, "w");
    if (!file) {
        printf("Error: Could not open hospital file %s for writing.\n", filename);
        return false;
    }

    for (int i = 0; i < fm->total_hospitals; i++) {
        Hospital h = fm->hospitals[i];
        fprintf(file, "%d %d %s %d %d %d %d %d\n",
            h.id, h.node_id, h.name,
            h.total_beds, h.available_beds,
            h.icu_beds_total, h.icu_beds_available,
            h.has_trauma_center ? 1 : 0);
    }

    fclose(file);
    return true;
}
