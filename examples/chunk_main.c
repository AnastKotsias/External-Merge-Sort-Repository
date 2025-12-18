#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bf.h"
#include "hp_file.h"
#include "chunk.h"

#define FILE_NAME "test_chunks.db"
#define RECORDS_NUM 53 // Using a prime number to ensure uneven chunks
#define BLOCKS_IN_CHUNK 3

int main() {
    // Initialize Buffer Manager
    BF_Init(LRU);

    // 1. Cleanup and Setup
    remove(FILE_NAME);
    if (HP_CreateFile(FILE_NAME) != 0) {
        fprintf(stderr, "Error creating heap file\n");
        return -1;
    }

    int file_desc;
    if (HP_OpenFile(FILE_NAME, &file_desc) != 0) {
        fprintf(stderr, "Error opening heap file\n");
        return -1;
    }

    // 2. Populate file with known data for verification
    printf("--- Populating Heap File with %d records ---\n", RECORDS_NUM);
    Record record;
    for (int i = 0; i < RECORDS_NUM; i++) {
        record.id = i;
        sprintf(record.name, "Name%d", i);
        sprintf(record.surname, "Surname%d", i);
        sprintf(record.city, "City%d", i);
        HP_InsertEntry(file_desc, record);
    }

    printf("\n--- Starting CHUNK Library Verification ---\n");

    // 3. Verify CHUNK_Iterator and CHUNK_RecordIterator
    CHUNK_Iterator chunk_it = CHUNK_CreateIterator(file_desc, BLOCKS_IN_CHUNK);
    CHUNK chunk;
    int chunk_count = 0;
    int total_records_found = 0;
    int error_count = 0;

    while (CHUNK_GetNext(&chunk_it, &chunk) == 0) {
        printf("[Chunk %d] Range: Blocks %d to %d | Records in Chunk: %d\n", 
                chunk_count++, chunk.from_BlockId, chunk.to_BlockId, chunk.recordsInChunk);

        CHUNK_RecordIterator rec_it = CHUNK_CreateRecordIterator(&chunk);
        Record rec;
        int rec_in_chunk_idx = 0;
        
        while (CHUNK_GetNextRecord(&rec_it, &rec) == 0) {
            // Programmatic Validation: Check if the iterator order is sequential
            if (rec.id != total_records_found) {
                printf("  [ERROR] Sequential mismatch: Expected ID %d, found %d\n", total_records_found, rec.id);
                error_count++;
            }

            // Cross-validation with CHUNK_GetIthRecordInChunk
            Record ith_rec;
            if (CHUNK_GetIthRecordInChunk(&chunk, rec_in_chunk_idx, &ith_rec) == 0) {
                if (ith_rec.id != rec.id) {
                    printf("  [ERROR] Random access mismatch at index %d\n", rec_in_chunk_idx);
                    error_count++;
                }
            } else {
                printf("  [ERROR] Failed to retrieve Ith record at index %d\n", rec_in_chunk_idx);
                error_count++;
            }
            
            rec_in_chunk_idx++;
            total_records_found++;
        }
    }

    // 4. Verification Summary
    if (total_records_found == RECORDS_NUM && error_count == 0) {
        printf("\n[SUCCESS] All %d records verified across all chunks.\n", total_records_found);
    } else {
        printf("\n[FAILURE] Found %d errors. Total records retrieved: %d/%d\n", error_count, total_records_found, RECORDS_NUM);
    }

    // 5. Verify CHUNK_UpdateIthRecord 
    printf("\nTesting Update Logic in the last Chunk...\n");
    Record update_rec = {999, "UpdatedName", "UpdatedSurname", "Athens"};
    if (CHUNK_UpdateIthRecord(&chunk, 0, update_rec) == 0) {
        Record verify_rec;
        CHUNK_GetIthRecordInChunk(&chunk, 0, &verify_rec);
        if (strcmp(verify_rec.name, "UpdatedName") == 0) {
            printf("Update Successful: Record 0 of last chunk is now %s (ID: %d)\n", verify_rec.name, verify_rec.id);
        } else {
            printf("Update Failed: Data mismatch.\n");
        }
    }

    // 6. Cleanup
    HP_CloseFile(file_desc);
    BF_Close();
    printf("\n--- End of Test ---\n");

    return 0;
}