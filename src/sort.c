#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "bf.h"
#include "hp_file.h"
#include "record.h"
#include "sort.h"
#include "merge.h"
#include "chunk.h"

void external_Sort(char* input_FileName, int numBlocksInChunk, int bway) {
    int file_desc;
    if (HP_OpenFile(input_FileName, &file_desc) != 0) return;

    // --- PASS 0: Initial Sorting ---
    // Sort the original file in-place in chunks of size numBlocksInChunk
    sort_FileInChunks(file_desc, numBlocksInChunk);
    HP_CloseFile(file_desc);

    int current_ChunkSize = numBlocksInChunk;
    int pass_count = 1;
    char current_Input[256];
    char current_Output[256];
    
    strcpy(current_Input, input_FileName);

    while (true) {
        // Open current input file to check how many blocks it has
        int in_fd;
        HP_OpenFile(current_Input, &in_fd);
        int last_block = HP_GetIdOfLastBlock(in_fd);
        
        // If the entire file fits into a single "chunk", sorting is complete
        if (current_ChunkSize >= last_block) {
            HP_CloseFile(in_fd);
            printf("Sorting complete after %d passes.\n", pass_count - 1);
            break;
        }

        // --- MERGE PASSES ---
        // Prepare names for intermediate files (e.g., temp_pass_1.db)
        sprintf(current_Output, "temp_pass_%d.db", pass_count);
        HP_CreateFile(current_Output);
        
        int out_fd;
        HP_OpenFile(current_Output, &out_fd);

        // Merge 'bway' chunks of current_ChunkSize into the output file
        merge(in_fd, current_ChunkSize, bway, out_fd);

        HP_CloseFile(in_fd);
        HP_CloseFile(out_fd);

        // Prepare for the next pass
        strcpy(current_Input, current_Output);
        current_ChunkSize *= bway; // Each new chunk is now bway times larger 
        pass_count++;
    }
}

/* * Comparison function used by qsort. 
 * Sorts records in ascending order based on Name, then Surname.
 */
int compareRecords(const void* a, const void* b) {
    Record* rec1 = (Record*)a;
    Record* rec2 = (Record*)b;

    int name_cmp = strcmp(rec1->name, rec2->name);
    if (name_cmp != 0) {
        return name_cmp;
    }
    return strcmp(rec1->surname, rec2->surname);
}

bool shouldSwap(Record* rec1, Record* rec2) {
    // Returns true if rec1 > rec2, indicating they are out of order
    return compareRecords(rec1, rec2) > 0;
}

void sort_Chunk(CHUNK* chunk) {
    int total_records = chunk->recordsInChunk;
    if (total_records <= 1) return;

    // Allocate memory to hold all records of the chunk for in-memory sorting
    Record* records = malloc(total_records * sizeof(Record));
    if (records == NULL) {
        perror("Failed to allocate memory for sorting chunk");
        return;
    }

    // 1. Read all records from the chunk into the array
    for (int i = 0; i < total_records; i++) {
        if (CHUNK_GetIthRecordInChunk(chunk, i, &records[i]) != 0) {
            fprintf(stderr, "Error reading record %d from chunk\n", i);
            free(records);
            return;
        }
    }

    // 2. Sort the array in memory
    qsort(records, total_records, sizeof(Record), compareRecords);

    // 3. Write the sorted records back to the chunk (in-place)
    for (int i = 0; i < total_records; i++) {
        if (CHUNK_UpdateIthRecord(chunk, i, records[i]) != 0) {
            fprintf(stderr, "Error updating record %d in chunk\n", i);
            free(records);
            return;
        }
    }

    free(records);
}

void sort_FileInChunks(int file_desc, int numBlocksInChunk) {
    // Initialize an iterator to traverse the file chunk by chunk
    CHUNK_Iterator iterator = CHUNK_CreateIterator(file_desc, numBlocksInChunk);
    CHUNK current_chunk;

    // Iterate through every chunk in the heap file and sort it
    while (CHUNK_GetNext(&iterator, &current_chunk) == 0) {
        sort_Chunk(&current_chunk);
    }
}