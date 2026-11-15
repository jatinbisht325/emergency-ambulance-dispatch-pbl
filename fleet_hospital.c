#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fleet_hospital.h"

/*
 * ============================================================================
 * MODULE: AMBULANCE FLEET & HOSPITAL PERSISTENCE (IMPLEMENTATION)
 * Author: Mansi (Refactored by Jatin for C99 Compliance & System Integration)
 *
 * Implements:
 * 1. Dynamic Singly Linked List for Ambulance fleet (O(1) head insertion)
 * 2. Hospital Bed & ICU tracker (atomic increment/decrement with capacity bounds)
 * 3. Flat-file disk serialization/deserialization for persistence
 * ============================================================================
 */

/* ----------------------------------------------------------------------------
 * 1. FLEET LIFECYCLE & LINKED LIST OPERATIONS
 * ---------------------------------------------------------------------------- */

/* Allocates and initializes an empty fleet manager */
Fleet* create_fleet(void) {
    Fleet* fleet = (Fleet*)malloc(sizeof(Fleet));
    if (!fleet) {
        fprintf(stderr, "[ERROR] Memory allocation failed for Fleet.\n");
        return NULL;
    }
    fleet->head = NULL;
    fleet->count = 0;
    return fleet;
}

/* Deallocates every ambulance node in the linked list and frees the fleet */
void free_fleet(Fleet* fleet) {
    if (!fleet) return;

    AmbulanceNode* current = fleet->head;
    while (current != NULL) {
        AmbulanceNode* temp = current;
        current = current->next;
        free(temp);
    }

    free(fleet);
}

/* Adds a new ambulance at the head of the singly linked list in O(1) time */
bool add_ambulance(Fleet* fleet, int id, const char* plate, const char* type, int start_node) {
    if (!fleet) {
        fprintf(stderr, "[ERROR] add_ambulance: Fleet pointer is NULL.\n");
        return false;
    }

    /* Prevent duplicate vehicle IDs */
    if (find_ambulance(fleet, id) != NULL) {
        fprintf(stderr, "[WARNING] Ambulance ID %d already exists. Skipping insertion.\n", id);
        return false;
    }

    AmbulanceNode* new_node = (AmbulanceNode*)malloc(sizeof(AmbulanceNode));
    if (!new_node) {
        fprintf(stderr, "[FATAL] Memory allocation failed for new AmbulanceNode.\n");
        return false;
    }

    new_node->id = id;
    strncpy(new_node->plate, plate ? plate : "N/A", MAX_STR_LEN - 1);
    new_node->plate[MAX_STR_LEN - 1] = '\0';

    strncpy(new_node->type, type ? type : "Basic", MAX_STR_LEN - 1);
    new_node->type[MAX_STR_LEN - 1] = '\0';

    new_node->current_node = start_node;
    new_node->status = STATUS_AVAILABLE;
    new_node->total_trips = 0;

    /* Head insertion: new_node points to old head, head becomes new_node */
    new_node->next = fleet->head;
    fleet->head = new_node;
    fleet->count++;

    return true;
}

/* Traverses the linked list defensively to find an ambulance by ID in O(N) time */
AmbulanceNode* find_ambulance(const Fleet* fleet, int id) {
    if (!fleet) return NULL;

    AmbulanceNode* current = fleet->head;
    while (current != NULL) {
        if (current->id == id) {
            return current;
        }
        current = current->next;
    }

    return NULL; /* Safe NULL return prevents segmentation faults */
}

/* Updates the operational status of an ambulance */
bool update_ambulance_status(Fleet* fleet, int id, AmbulanceStatus new_status) {
    AmbulanceNode* amb = find_ambulance(fleet, id);
    if (!amb) {
        fprintf(stderr, "[ERROR] Ambulance ID %d not found for status update.\n", id);
        return false;
    }

    /* If the vehicle is returning to AVAILABLE, increment its completed mission count */
    if (new_status == STATUS_AVAILABLE && amb->status != STATUS_AVAILABLE) {
        amb->total_trips++;
    }

    amb->status = new_status;
    return true;
}

/* Updates the physical location node of an ambulance on the map */
bool update_ambulance_location(Fleet* fleet, int id, int new_node) {
    AmbulanceNode* amb = find_ambulance(fleet, id);
    if (!amb) {
        fprintf(stderr, "[ERROR] Ambulance ID %d not found for location update.\n", id);
        return false;
    }

    amb->current_node = new_node;
    return true;
}

/* Returns the total number of ambulances currently ready for dispatch */
int count_available_ambulances(const Fleet* fleet) {
    if (!fleet) return 0;

    int ready_count = 0;
    AmbulanceNode* current = fleet->head;
    while (current != NULL) {
        if (current->status == STATUS_AVAILABLE) {
            ready_count++;
        }
        current = current->next;
    }
    return ready_count;
}

/* Prints a structured table of all registered fleet vehicles */
void print_fleet(const Fleet* fleet) {
    if (!fleet || !fleet->head) {
        printf("\n[FLEET REGISTRY IS CURRENTLY EMPTY]\n");
        return;
    }

    printf("\n===============================================================================\n");
    printf("                           ACTIVE AMBULANCE FLEET                              \n");
    printf("===============================================================================\n");
    printf(" %-5s  %-15s  %-16s  %-6s  %-15s  %-6s\n",
           "ID", "Plate Number", "Category", "Node", "Status", "Trips");
    printf("-------------------------------------------------------------------------------\n");

    const char* status_names[] = {
        "AVAILABLE",
        "EN ROUTE",
        "AT SCENE",
        "RETURNING"
    };

    AmbulanceNode* current = fleet->head;
    while (current != NULL) {
        const char* status_str = "UNKNOWN";
        if (current->status >= STATUS_AVAILABLE && current->status <= STATUS_RETURNING) {
            status_str = status_names[current->status];
        }

        printf(" %-5d  %-15s  %-16s  [%02d]    %-15s  %-6d\n",
               current->id,
               current->plate,
               current->type,
               current->current_node,
               status_str,
               current->total_trips);

        current = current->next;
    }

    printf("-------------------------------------------------------------------------------\n");
    printf(" Total Fleet Size: %d | Available for Dispatch: %d\n",
           fleet->count, count_available_ambulances(fleet));
    printf("===============================================================================\n\n");
}

/* ----------------------------------------------------------------------------
 * 2. HOSPITAL DIRECTORY & CAPACITY TRACKER
 * ---------------------------------------------------------------------------- */

/* Allocates and initializes an empty hospital list */
HospitalList* create_hospital_list(void) {
    HospitalList* list = (HospitalList*)malloc(sizeof(HospitalList));
    if (!list) {
        fprintf(stderr, "[ERROR] Memory allocation failed for HospitalList.\n");
        return NULL;
    }
    list->head = NULL;
    list->count = 0;
    return list;
}

/* Deallocates every hospital node and frees the list */
void free_hospital_list(HospitalList* list) {
    if (!list) return;

    HospitalNode* current = list->head;
    while (current != NULL) {
        HospitalNode* temp = current;
        current = current->next;
        free(temp);
    }

    free(list);
}

/* Adds a hospital record to the linked list */
bool add_hospital(HospitalList* list, int id, int node_id, const char* name,
                 int total_beds, int available_beds,
                 int icu_total, int icu_avail, bool has_trauma) {
    if (!list) return false;

    if (find_hospital_by_id(list, id) != NULL) {
        fprintf(stderr, "[WARNING] Hospital ID %d already exists. Skipping.\n", id);
        return false;
    }

    HospitalNode* new_node = (HospitalNode*)malloc(sizeof(HospitalNode));
    if (!new_node) {
        fprintf(stderr, "[FATAL] Memory allocation failed for HospitalNode.\n");
        return false;
    }

    new_node->id = id;
    new_node->node_id = node_id;
    strncpy(new_node->name, name ? name : "City Medical Center", MAX_STR_LEN - 1);
    new_node->name[MAX_STR_LEN - 1] = '\0';

    new_node->total_beds = total_beds;
    new_node->available_beds = available_beds;
    new_node->icu_beds_total = icu_total;
    new_node->icu_beds_available = icu_avail;
    new_node->has_trauma_center = has_trauma;

    /* Head insertion */
    new_node->next = list->head;
    list->head = new_node;
    list->count++;

    return true;
}

/* Finds a hospital by unique ID */
HospitalNode* find_hospital_by_id(const HospitalList* list, int id) {
    if (!list) return NULL;

    HospitalNode* current = list->head;
    while (current != NULL) {
        if (current->id == id) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

/* Finds a hospital situated at a specific intersection node */
HospitalNode* find_hospital_by_node(const HospitalList* list, int node_id) {
    if (!list) return NULL;

    HospitalNode* current = list->head;
    while (current != NULL) {
        if (current->node_id == node_id) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

/* Finds the first hospital matching specific medical treatment needs */
HospitalNode* find_suitable_hospital(const HospitalList* list, bool need_icu, bool need_trauma) {
    if (!list) return NULL;

    HospitalNode* current = list->head;
    while (current != NULL) {
        /* Check trauma requirement */
        if (need_trauma && !current->has_trauma_center) {
            current = current->next;
            continue;
        }

        /* Check ICU bed requirement */
        if (need_icu && current->icu_beds_available <= 0) {
            current = current->next;
            continue;
        }

        /* Check general bed availability */
        if (current->available_beds > 0) {
            return current;
        }

        current = current->next;
    }

    return NULL; /* No hospital currently has capacity matching criteria */
}

/* Reserves a bed (regular or ICU) at the target hospital */
bool admit_patient_to_hospital(HospitalList* list, int hospital_id, bool requires_icu) {
    HospitalNode* hosp = find_hospital_by_id(list, hospital_id);
    if (!hosp) {
        printf("[HOSPITAL ERROR] Hospital ID %d not found.\n", hospital_id);
        return false;
    }

    if (requires_icu) {
        if (hosp->icu_beds_available <= 0) {
            printf("[ADMISSION REJECTED] Hospital '%s' has 0 ICU beds available!\n", hosp->name);
            return false;
        }
        if (hosp->available_beds <= 0) {
            printf("[ADMISSION REJECTED] Hospital '%s' has 0 total beds available!\n", hosp->name);
            return false;
        }
        hosp->icu_beds_available--;
        hosp->available_beds--;
        printf("[HOSPITAL ADMIT] Patient admitted to ICU at '%s'. (Remaining ICU: %d, General: %d)\n",
               hosp->name, hosp->icu_beds_available, hosp->available_beds);
        return true;
    } else {
        if (hosp->available_beds <= 0) {
            printf("[ADMISSION REJECTED] Hospital '%s' has no regular beds available!\n", hosp->name);
            return false;
        }
        hosp->available_beds--;
        printf("[HOSPITAL ADMIT] Patient admitted to General Ward at '%s'. (Remaining Beds: %d)\n",
               hosp->name, hosp->available_beds);
        return true;
    }
}

/* Discharges a patient, releasing a regular or ICU bed back into availability */
bool discharge_patient_from_hospital(HospitalList* list, int hospital_id, bool had_icu) {
    HospitalNode* hosp = find_hospital_by_id(list, hospital_id);
    if (!hosp) return false;

    if (had_icu) {
        if (hosp->icu_beds_available < hosp->icu_beds_total) {
            hosp->icu_beds_available++;
        }
    }

    if (hosp->available_beds < hosp->total_beds) {
        hosp->available_beds++;
        return true;
    }

    return false;
}

/* Prints a formatted status table of all registered hospitals */
void print_hospitals(const HospitalList* list) {
    if (!list || !list->head) {
        printf("\n[NO HOSPITALS REGISTERED]\n");
        return;
    }

    printf("\n===============================================================================\n");
    printf("                         CITY HOSPITAL CAPACITY MONITOR                        \n");
    printf("===============================================================================\n");
    printf(" %-4s  %-4s  %-30s  %-12s  %-10s  %-8s\n",
           "ID", "Node", "Hospital Name", "General Beds", "ICU Beds", "Trauma");
    printf("-------------------------------------------------------------------------------\n");

    HospitalNode* curr = list->head;
    while (curr != NULL) {
        char gen_str[32], icu_str[32];
        snprintf(gen_str, sizeof(gen_str), "%d / %d", curr->available_beds, curr->total_beds);
        snprintf(icu_str, sizeof(icu_str), "%d / %d", curr->icu_beds_available, curr->icu_beds_total);

        printf(" %-4d  [%02d]  %-30s  %-12s  %-10s  %-8s\n",
               curr->id,
               curr->node_id,
               curr->name,
               gen_str,
               icu_str,
               curr->has_trauma_center ? "YES" : "NO");

        curr = curr->next;
    }

    printf("===============================================================================\n\n");
}

/* ----------------------------------------------------------------------------
 * 3. FLAT-FILE DISK PERSISTENCE ENGINE (I/O)
 * ---------------------------------------------------------------------------- */

/* Loads ambulances from flat text file */
bool load_fleet_from_file(Fleet* fleet, const char* filename) {
    if (!fleet || !filename) return false;

    FILE* fp = fopen(filename, "r");
    if (!fp) {
        printf("[NOTICE] Fleet file '%s' not found. Starting with default/empty fleet.\n", filename);
        return false;
    }

    char line[256];
    int loaded = 0;

    while (fgets(line, sizeof(line), fp)) {
        /* Strip leading whitespace */
        char* p = line;
        while (*p == ' ' || *p == '\t') p++;

        /* Skip comments and empty lines */
        if (*p == '#' || *p == '\0' || *p == '\n' || *p == '\r') {
            continue;
        }

        int id = 0, node = 0, status_val = 0, trips = 0;
        char plate[MAX_STR_LEN] = {0};
        char type[MAX_STR_LEN] = {0};

        /* Format: ID PLATE TYPE NODE STATUS TRIPS */
        int count = sscanf(p, "%d %63s %63s %d %d %d",
                           &id, plate, type, &node, &status_val, &trips);
        if (count >= 5) {
            add_ambulance(fleet, id, plate, type, node);
            AmbulanceNode* amb = find_ambulance(fleet, id);
            if (amb) {
                amb->status = (AmbulanceStatus)status_val;
                if (count == 6) amb->total_trips = trips;
            }
            loaded++;
        }
    }

    fclose(fp);
    printf("[+] Successfully loaded %d ambulances from '%s'.\n", loaded, filename);
    return true;
}

/* Saves the current fleet to flat text file */
bool save_fleet_to_file(const Fleet* fleet, const char* filename) {
    if (!fleet || !filename) return false;

    FILE* fp = fopen(filename, "w");
    if (!fp) {
        fprintf(stderr, "[ERROR] Could not open '%s' for saving fleet.\n", filename);
        return false;
    }

    fprintf(fp, "# ====================================================================\n");
    fprintf(fp, "# AMBULANCE FLEET PERSISTENCE DATA\n");
    fprintf(fp, "# Format: ID PLATE TYPE NODE STATUS TRIPS\n");
    fprintf(fp, "# ====================================================================\n");

    AmbulanceNode* curr = fleet->head;
    while (curr != NULL) {
        fprintf(fp, "%d %s %s %d %d %d\n",
                curr->id,
                curr->plate,
                curr->type,
                curr->current_node,
                (int)curr->status,
                curr->total_trips);
        curr = curr->next;
    }

    fclose(fp);
    printf("[+] Fleet data successfully saved to '%s'.\n", filename);
    return true;
}

/* Loads hospitals from flat text file */
bool load_hospitals_from_file(HospitalList* list, const char* filename) {
    if (!list || !filename) return false;

    FILE* fp = fopen(filename, "r");
    if (!fp) {
        printf("[NOTICE] Hospital file '%s' not found.\n", filename);
        return false;
    }

    char line[256];
    int loaded = 0;

    while (fgets(line, sizeof(line), fp)) {
        char* p = line;
        while (*p == ' ' || *p == '\t') p++;

        if (*p == '#' || *p == '\0' || *p == '\n' || *p == '\r') {
            continue;
        }

        int id = 0, node_id = 0, total_b = 0, avail_b = 0, icu_tot = 0, icu_av = 0, trauma = 0;
        char name[MAX_STR_LEN] = {0};

        /* Format: ID NODE_ID TOTAL_BEDS AVAIL_BEDS ICU_TOTAL ICU_AVAIL TRAUMA NAME */
        int count = sscanf(p, "%d %d %d %d %d %d %d %63[^\r\n]",
                           &id, &node_id, &total_b, &avail_b, &icu_tot, &icu_av, &trauma, name);
        if (count == 8) {
            /* Trim trailing whitespace from name */
            int len = strlen(name);
            while (len > 0 && (name[len - 1] == ' ' || name[len - 1] == '\t' || name[len - 1] == '\r')) {
                name[--len] = '\0';
            }

            add_hospital(list, id, node_id, name, total_b, avail_b, icu_tot, icu_av, trauma != 0);
            loaded++;
        }
    }

    fclose(fp);
    printf("[+] Successfully loaded %d hospitals from '%s'.\n", loaded, filename);
    return true;
}

/* Saves hospitals to flat text file */
bool save_hospitals_to_file(const HospitalList* list, const char* filename) {
    if (!list || !filename) return false;

    FILE* fp = fopen(filename, "w");
    if (!fp) {
        fprintf(stderr, "[ERROR] Could not open '%s' for saving hospitals.\n", filename);
        return false;
    }

    fprintf(fp, "# ====================================================================\n");
    fprintf(fp, "# HOSPITAL DIRECTORY PERSISTENCE DATA\n");
    fprintf(fp, "# Format: ID NODE_ID TOTAL_BEDS AVAIL_BEDS ICU_TOTAL ICU_AVAIL TRAUMA NAME\n");
    fprintf(fp, "# ====================================================================\n");

    HospitalNode* curr = list->head;
    while (curr != NULL) {
        fprintf(fp, "%d %d %d %d %d %d %d %s\n",
                curr->id,
                curr->node_id,
                curr->total_beds,
                curr->available_beds,
                curr->icu_beds_total,
                curr->icu_beds_available,
                curr->has_trauma_center ? 1 : 0,
                curr->name);
        curr = curr->next;
    }

    fclose(fp);
    printf("[+] Hospital directory successfully saved to '%s'.\n", filename);
    return true;
}
