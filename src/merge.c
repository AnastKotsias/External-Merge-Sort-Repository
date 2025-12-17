#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "merge.h"
#include "chunk.h"
#include "hp_file.h"
#include "record.h"
#include "sort.h"

/**
 * Helper function to find the index of the record with the minimum value 
 * among the current records of all active iterators.
 */
static int findMinimumRecord(Record* records, int count, bool* active) {
    int minIndex = -1;
    for (int i = 0; i < count; i++) {
        if (!active[i]) continue;
        if (minIndex == -1 || shouldSwap(&records[minIndex], &records[i])) {
            minIndex = i;
        }
    }
    return minIndex;
}

void merge(int input_FileDesc, int chunkSize, int bway, int output_FileDesc) {
    CHUNK_Iterator inputIterator = CHUNK_CreateIterator(input_FileDesc, chunkSize);
    
    // Process groups of 'bway' chunks until the input file is exhausted
    while (true) {
        CHUNK* chunksToMerge = malloc(sizeof(CHUNK) * bway);
        CHUNK_RecordIterator* recordIterators = malloc(sizeof(CHUNK_RecordIterator) * bway);
        Record* currentRecords = malloc(sizeof(Record) * bway);
        bool* active = malloc(sizeof(bool) * bway);
        
        int actualChunks = 0;
        // 1. Initialize iterators for up to 'bway' chunks
        for (int i = 0; i < bway; i++) {
            if (CHUNK_GetNext(&inputIterator, &chunksToMerge[i]) == 0) {
                recordIterators[i] = CHUNK_CreateRecordIterator(&chunksToMerge[i]);
                if (CHUNK_GetNextRecord(&recordIterators[i], &currentRecords[i]) == 0) {
                    active[i] = true;
                    actualChunks++;
                } else {
                    active[i] = false;
                }
            } else {
                active[i] = false;
            }
        }

        // If no chunks were found in this iteration of the outer loop, we've processed the file
        if (actualChunks == 0) {
            free(chunksToMerge);
            free(recordIterators);
            free(currentRecords);
            free(active);
            break;
        }

        // 2. Perform the B-way merge for the current group of chunks
        while (true) {
            int minIdx = findMinimumRecord(currentRecords, bway, active);
            
            // If no active records remain, this specific merge group is finished
            if (minIdx == -1) break; 

            // Insert the smallest record into the output heap file
            HP_InsertEntry(output_FileDesc, currentRecords[minIdx]);

            // Advance the iterator that provided the minimum record
            if (CHUNK_GetNextRecord(&recordIterators[minIdx], &currentRecords[minIdx]) != 0) {
                active[minIdx] = false;
            }
        }

        // Clean up memory for the current merge group
        free(chunksToMerge);
        free(recordIterators);
        free(currentRecords);
        free(active);
    }
}