/*
 * Boat Management System
 *
 * This program manages a set of boats at the marina, including:
 *  - Boats in slips   ($12.50/foot/month)
 *  - Boats on land    ($14.00/foot/month)
 *  - Boats on trailers ($25.00/foot/month)
 *  - Boats in storage ($11.20/foot/month)
 *
 * The system loads boat information from a CSV file, allows the user to perform
 * various operations, and saves the final data back upon exit.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_VESSELS          120
#define MAX_VESSEL_NAME_LEN  128
#define MAX_LEN_FEET         100
#define MAX_SLIP_NUM         85
#define MAX_STORAGE_LOC      50

/* Monthly billing rates (dollars per foot) */
#define RATE_SLIP      12.50
#define RATE_LAND      14.00
#define RATE_TRAILER   25.00
#define RATE_STORAGE   11.20

/* Enum for location categories */
typedef enum {
    SLIP,
    LAND,
    TRAILOR,
    STORAGE
} LocationCategory;

/* Union storing location-specific data */
typedef union {
    int  slipNo;                /* For SLIP (1–85)       */
    char bayLabel;              /* For LAND (A–Z)        */
    char trailerTag[10];        /* For TRAILOR           */
    int  storageSpot;           /* For STORAGE (1–50)    */
} LocDetails;

/* Struct for each vessel’s information */
typedef struct {
    char            vesselName[MAX_VESSEL_NAME_LEN];
    float           lengthFt;
    LocationCategory locationCat;
    LocDetails      locationInfo;
    float           outstandingFees;
} Vessel;
void  printWelcome();
void  printFarewell();
void  showMenu();
void  loadData(const char* fileName, Vessel** fleet, int* totalCount);
void  saveData(const char* fileName, Vessel** fleet, int totalCount);
void  listAllVessels(Vessel** fleet, int totalCount);
void  insertVessel(Vessel** fleet, int* totalCount, const char* csvLine);
void  removeVessel(Vessel** fleet, int* totalCount);
void  recordPayment(Vessel** fleet, int totalCount);
void  applyMonthlyFees(Vessel** fleet, int totalCount);
char* locationCategoryToStr(LocationCategory lc);
int   locateVesselByName(Vessel** fleet, int totalCount, const char* searchName);
int   compareVessels(const void* a, const void* b);
void  freeVesselMemory(Vessel** fleet, int totalCount);

int main(int argc, char* argv[])
{
    Vessel* fleet[MAX_VESSELS] = { NULL };
    int     totalVessels       = 0;
    char    userChoice;
    char    inputBuffer[256];
    if (argc != 2) {
        printf("Usage: %s <boatdata.csv>\n", argv[0]);
        return 1;
    }

    /* data from CSV */
    loadData(argv[1], fleet, &totalVessels);

    /* Welcome message */
    printWelcome();

    /* Main loop for user interaction */
    do {
        showMenu();
        if (fgets(inputBuffer, sizeof(inputBuffer), stdin) != NULL) {
            userChoice = toupper(inputBuffer[0]);

            switch (userChoice) {
                case 'I':
                    listAllVessels(fleet, totalVessels);
                    break;
                case 'A':
                    printf("Please enter the boat data in CSV format                 : ");
                    if (fgets(inputBuffer, sizeof(inputBuffer), stdin) != NULL) {
                        inputBuffer[strcspn(inputBuffer, "\n")] = '\0';
                        insertVessel(fleet, &totalVessels, inputBuffer);
                    }
                    break;
                case 'R':
                    removeVessel(fleet, &totalVessels);
                    break;
                case 'P':
                    recordPayment(fleet, totalVessels);
                    break;
                case 'M':
                    applyMonthlyFees(fleet, totalVessels);
                    break;
                case 'X':
                    break;
                default:
                    printf("Invalid option %c\n\n", userChoice);
                    break;
            }
        }
    } while (userChoice != 'X');

    saveData(argv[1], fleet, totalVessels);

    printFarewell();

    /* Free memory */
    freeVesselMemory(fleet, totalVessels);

    return 0;
}
void printWelcome()
{
    printf("\nWelcome to Alans' Boat Management\n");
    printf("--------------------------------------------\n\n");
}

void printFarewell()
{
    printf("\nExiting Boat Management System\n");
}

void showMenu()
{
    printf("(I)nventory, (A)dd, (R)emove, (P)ayment, (M)onth, e(X)it : ");
}

void loadData(const char* fileName, Vessel** fleet, int* totalCount)
{
    FILE* fp = fopen(fileName, "r");
    if (!fp) {
        printf("Warning: Could not open %s for reading.\n", fileName);
        return;
    }

    *totalCount = 0;
    char line[256];

    while (fgets(line, sizeof(line), fp) != NULL && *totalCount < MAX_VESSELS) {
        line[strcspn(line, "\n")] = '\0';  /* Remove trailing newline */

        Vessel* newBoat = (Vessel*)malloc(sizeof(Vessel));
        if (!newBoat) {
            printf("Error: memory allocation failed.\n");
            continue;
        }

        char* segment;
        char* remainder = line;

        segment = strtok_r(remainder, ",", &remainder);
        if (!segment) {
            free(newBoat);
            continue;
        }
        strncpy(newBoat->vesselName, segment, MAX_VESSEL_NAME_LEN - 1);
        newBoat->vesselName[MAX_VESSEL_NAME_LEN - 1] = '\0';

        /* Length of vessel */
        segment = strtok_r(remainder, ",", &remainder);
        if (!segment) {
            free(newBoat);
            continue;
        }
        newBoat->lengthFt = (float)atof(segment);

        /* Location category (slip land, trailor, storage) */
        segment = strtok_r(remainder, ",", &remainder);
        if (!segment) {
            free(newBoat);
            continue;
        }
        if (strcmp(segment, "slip") == 0) {
            newBoat->locationCat = SLIP;
            segment = strtok_r(remainder, ",", &remainder);
            if (!segment) {
                free(newBoat);
                continue;
            }
            newBoat->locationInfo.slipNo = atoi(segment);
        } else if (strcmp(segment, "land") == 0) {
            newBoat->locationCat = LAND;
            segment = strtok_r(remainder, ",", &remainder);
            if (!segment || strlen(segment) == 0) {
                free(newBoat);
                continue;
            }
            newBoat->locationInfo.bayLabel = segment[0];
        } else if (strcmp(segment, "trailor") == 0) {
            newBoat->locationCat = TRAILOR;
            segment = strtok_r(remainder, ",", &remainder);
            if (!segment) {
                free(newBoat);
                continue;
            }
            strncpy(newBoat->locationInfo.trailerTag, segment, 9);
            newBoat->locationInfo.trailerTag[9] = '\0';
        } else if (strcmp(segment, "storage") == 0) {
            newBoat->locationCat = STORAGE;
            segment = strtok_r(remainder, ",", &remainder);
            if (!segment) {
                free(newBoat);
                continue;
            }
            newBoat->locationInfo.storageSpot = atoi(segment);
        } else {
            free(newBoat);
            continue;
        }

        /* Outstanding fees */
        segment = strtok_r(remainder, ",", &remainder);
        if (!segment) {
            free(newBoat);
            continue;
        }
        newBoat->outstandingFees = (float)atof(segment);

        fleet[*totalCount] = newBoat;
        (*totalCount)++;
    }
    fclose(fp);

    /* Sort vessels by name for consistent ordering */
    qsort(fleet, *totalCount, sizeof(Vessel*), compareVessels);
}

/*
 * Write updated vessel info back to CSV file
 */
void saveData(const char* fileName, Vessel** fleet, int totalCount)
{
    FILE* fp = fopen(fileName, "w");
    if (!fp) {
        printf("Error: Could not open file %s for writing.\n", fileName);
        return;
    }

    for (int i = 0; i < totalCount; i++) {
        Vessel* v = fleet[i];
        fprintf(fp, "%s,%.0f,%s,", 
                v->vesselName,
                v->lengthFt,
                locationCategoryToStr(v->locationCat));

        switch (v->locationCat) {
            case SLIP:
                fprintf(fp, "%d", v->locationInfo.slipNo);
                break;
            case LAND:
                fprintf(fp, "%c", v->locationInfo.bayLabel);
                break;
            case TRAILOR:
                fprintf(fp, "%s", v->locationInfo.trailerTag);
                break;
            case STORAGE:
                fprintf(fp, "%d", v->locationInfo.storageSpot);
                break;
        }
        fprintf(fp, ",%.2f\n", v->outstandingFees);
    }

    fclose(fp);
}

/* List vessels in alphabetical order */
void listAllVessels(Vessel** fleet, int totalCount)
{
    for (int i = 0; i < totalCount; i++) {
        Vessel* v = fleet[i];
        printf("%-20s %3.0f' ", v->vesselName, v->lengthFt);

        switch (v->locationCat) {
            case SLIP:
                printf("%8s   # %2d", "slip", v->locationInfo.slipNo);
                break;
            case LAND:
                printf("%8s      %c", "land", v->locationInfo.bayLabel);
                break;
            case TRAILOR:
                printf("%8s %6s", "trailor", v->locationInfo.trailerTag);
                break;
            case STORAGE:
                printf("%8s   # %2d", "storage", v->locationInfo.storageSpot);
                break;
        }
        printf("   Owes $%7.2f\n", v->outstandingFees);
    }
    printf("\n");
}

/*
 * Insert a new boat from a CSV-style string
 */
void insertVessel(Vessel** fleet, int* totalCount, const char* csvLine)
{
    if (*totalCount >= MAX_VESSELS) {
        printf("Error: Maximum capacity reached.\n\n");
        return;
    }

    Vessel* newBoat = (Vessel*)malloc(sizeof(Vessel));
    if (!newBoat) {
        printf("Error: Memory allocation problem.\n\n");
        return;
    }

    char buffer[256];
    strncpy(buffer, csvLine, sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';

    char* token;
    char* rest = buffer;

    /* Vessel name */
    token = strtok_r(rest, ",", &rest);
    if (!token) {
        free(newBoat);
        printf("Error: Invalid CSV format.\n\n");
        return;
    }
    strncpy(newBoat->vesselName, token, MAX_VESSEL_NAME_LEN - 1);
    newBoat->vesselName[MAX_VESSEL_NAME_LEN - 1] = '\0';

    /* Vessel length */
    token = strtok_r(rest, ",", &rest);
    if (!token) {
        free(newBoat);
        printf("Error: Invalid CSV format.\n\n");
        return;
    }
    newBoat->lengthFt = (float)atof(token);

    /* Location category */
    token = strtok_r(rest, ",", &rest);
    if (!token) {
        free(newBoat);
        printf("Error: Invalid CSV format.\n\n");
        return;
    }
    if (strcasecmp(token, "slip") == 0) {
        newBoat->locationCat = SLIP;
        token = strtok_r(rest, ",", &rest);
        if (!token) {
            free(newBoat);
            printf("Error: Incomplete data.\n\n");
            return;
        }
        newBoat->locationInfo.slipNo = atoi(token);
    } else if (strcasecmp(token, "land") == 0) {
        newBoat->locationCat = LAND;
        token = strtok_r(rest, ",", &rest);
        if (!token || strlen(token) == 0) {
            free(newBoat);
            printf("Error: Incomplete data.\n\n");
            return;
        }
        newBoat->locationInfo.bayLabel = token[0];
    } else if (strcasecmp(token, "trailor") == 0) {
        newBoat->locationCat = TRAILOR;
        token = strtok_r(rest, ",", &rest);
        if (!token) {
            free(newBoat);
            printf("Error: Incomplete data.\n\n");
            return;
        }
        strncpy(newBoat->locationInfo.trailerTag, token, 9);
        newBoat->locationInfo.trailerTag[9] = '\0';
    } else if (strcasecmp(token, "storage") == 0) {
        newBoat->locationCat = STORAGE;
        token = strtok_r(rest, ",", &rest);
        if (!token) {
            free(newBoat);
            printf("Error: Incomplete data.\n\n");
            return;
        }
        newBoat->locationInfo.storageSpot = atoi(token);
    } else {
        free(newBoat);
        printf("Error: Unknown location.\n\n");
        return;
    }

    /* Outstanding fees */
    token = strtok_r(rest, ",", &rest);
    if (!token) {
        free(newBoat);
        printf("Error: Missing fee data.\n\n");
        return;
    }
    newBoat->outstandingFees = (float)atof(token);

    /* Add the new vessel and sort again */
    fleet[*totalCount] = newBoat;
    (*totalCount)++;
    qsort(fleet, *totalCount, sizeof(Vessel*), compareVessels);
}

/*
 * Delete a boat entry by its name
 */
void removeVessel(Vessel** fleet, int* totalCount)
{
    char targetName[MAX_VESSEL_NAME_LEN];
    printf("Please enter the boat name                               : ");
    if (fgets(targetName, sizeof(targetName), stdin) != NULL) {
        targetName[strcspn(targetName, "\n")] = '\0';

        int idx = locateVesselByName(fleet, *totalCount, targetName);
        if (idx == -1) {
            printf("No boat with that name\n\n");
            return;
        }
        free(fleet[idx]);
        for (int i = idx; i < (*totalCount) - 1; i++) {
            fleet[i] = fleet[i + 1];
        }
        (*totalCount)--;
    }
}

/*
 * Accept a payment up to the total owed
 */
void recordPayment(Vessel** fleet, int totalCount)
{
    char  inputName[MAX_VESSEL_NAME_LEN];
    float amount;
    printf("Please enter the boat name: ");
    if (fgets(inputName, sizeof(inputName), stdin) != NULL) {
        inputName[strcspn(inputName, "\n")] = '\0';
        int index = locateVesselByName(fleet, totalCount, inputName);
        if (index == -1) {
            printf("No boat with that name\n\n");
            return;
        }
        printf("Please enter the amount to be paid: ");
        char buf[50];
        if (fgets(buf, sizeof(buf), stdin) != NULL) {
            amount = (float)atof(buf);
            
            if (amount >= fleet[index]->outstandingFees) {
                printf("That is more than the amount owed, $%.2f\n\n",
                       fleet[index]->outstandingFees);
                return;
            }
            fleet[index]->outstandingFees -= amount;
        }
    }
}

/*
 * Add monthly charges for each vessel
 */
void applyMonthlyFees(Vessel** fleet, int totalCount)
{
    for (int i = 0; i < totalCount; i++) {
        Vessel* v = fleet[i];
        float monthlyCharge = 0.0f;
        switch (v->locationCat) {
            case SLIP:
                monthlyCharge = v->lengthFt * RATE_SLIP;
                break;
            case LAND:
                monthlyCharge = v->lengthFt * RATE_LAND;
                break;
            case TRAILOR:
                monthlyCharge = v->lengthFt * RATE_TRAILER;
                break;
            case STORAGE:
                monthlyCharge = v->lengthFt * RATE_STORAGE;
                break;
        }
        v->outstandingFees += monthlyCharge;
    }
    printf("\n");
}

/*
 * Convert the enum to a string
 */
char* locationCategoryToStr(LocationCategory lc)
{
    switch (lc) {
        case SLIP:    return "slip";
        case LAND:    return "land";
        case TRAILOR: return "trailor";
        case STORAGE: return "storage";
        default:      return "unknown";
    }
}

/*
 * Return the index of a boat by name
 */
int locateVesselByName(Vessel** fleet, int totalCount, const char* searchName)
{
    for (int i = 0; i < totalCount; i++) {
        if (strcasecmp(fleet[i]->vesselName, searchName) == 0) {
            return i;
        }
    }
    return -1;
}

/*
 * Used by qsort to compare names
 */
int compareVessels(const void* a, const void* b)
{
    Vessel* va = *(Vessel**)a;
    Vessel* vb = *(Vessel**)b;
    return strcasecmp(va->vesselName, vb->vesselName);
}

/*
 * Free up all dynamically allocated memory
 */
void freeVesselMemory(Vessel** fleet, int totalCount)
{
    for (int i = 0; i < totalCount; i++) {
        free(fleet[i]);
    }
}
