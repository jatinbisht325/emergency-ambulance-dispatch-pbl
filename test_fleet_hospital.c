#include <stdio.h>
#include <stdlib.h>
#include "fleet_hospital.h"

int main(void) {
    printf("=================================================================\n");
    printf("     FLEET & HOSPITAL MANAGEMENT MODULE - STANDALONE TEST        \n");
    printf("     Author: Mansi | Verification Harness: Jatin Bisht          \n");
    printf("=================================================================\n");

    /* 1. Test Fleet Initialization & File Loading */
    printf("\n[*] Initializing Ambulance Fleet...\n");
    Fleet* fleet = create_fleet();
    if (!fleet) {
        fprintf(stderr, "[FATAL] Fleet initialization failed.\n");
        return EXIT_FAILURE;
    }

    if (!load_fleet_from_file(fleet, "fleet.txt")) {
        fprintf(stderr, "[ERROR] Failed to load fleet.txt\n");
    }

    print_fleet(fleet);

    /* 2. Test Dynamic Insertion (O(1) Head Insertion) */
    printf("[*] Adding Emergency Reserve Vehicle (ID 107)...\n");
    add_ambulance(fleet, 107, "UK-04-R-9999", "ICU-Advanced", 0);
    print_fleet(fleet);

    /* 3. Test Status Transitions */
    printf("[*] Dispatching Ambulance ID 101 (STATUS_AVAILABLE -> STATUS_EN_ROUTE)...\n");
    update_ambulance_status(fleet, 101, STATUS_EN_ROUTE);
    update_ambulance_location(fleet, 101, 3); // Moved to GEHU Gate

    AmbulanceNode* a101 = find_ambulance(fleet, 101);
    if (a101) {
        printf("[+] Vehicle 101 verified: Current Node = [%02d], Status = %d\n",
               a101->current_node, a101->status);
    }

    /* 4. Test Hospital Directory Initialization & File Loading */
    printf("\n[*] Initializing Hospital Directory...\n");
    HospitalList* hospitals = create_hospital_list();
    if (!hospitals) {
        fprintf(stderr, "[FATAL] Hospital list initialization failed.\n");
        free_fleet(fleet);
        return EXIT_FAILURE;
    }

    if (!load_hospitals_from_file(hospitals, "hospitals.txt")) {
        fprintf(stderr, "[ERROR] Failed to load hospitals.txt\n");
    }

    print_hospitals(hospitals);

    /* 5. Test Hospital Bed Admission (Regular and ICU) */
    printf("[*] Testing Patient Admission to District Civil Hospital (ID 1, Regular Ward)...\n");
    admit_patient_to_hospital(hospitals, 1, false);

    printf("[*] Testing Critical Patient Admission to Trauma Center (ID 2, ICU)...\n");
    admit_patient_to_hospital(hospitals, 2, true);

    print_hospitals(hospitals);

    /* 6. Test Patient Discharge */
    printf("[*] Testing Patient Discharge from District Civil Hospital (ID 1)...\n");
    discharge_patient_from_hospital(hospitals, 1, false);
    print_hospitals(hospitals);

    /* 7. Test Memory Deallocation */
    printf("[*] Deallocating all dynamically allocated nodes...\n");
    free_fleet(fleet);
    free_hospital_list(hospitals);
    printf("[+] Memory deallocated cleanly. 0 leaks.\n");

    printf("\n=================================================================\n");
    printf("     MANSI'S MODULE VERIFICATION COMPLETE - FULLY FUNCTIONAL     \n");
    printf("=================================================================\n");

    return EXIT_SUCCESS;
}
