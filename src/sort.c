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

int compareRecords(const void* a, const void* b) {
    const Record* rec1 = (const Record*)a;
    const Record* rec2 = (const Record*)b;

    int name_cmp = strcmp(rec1->name, rec2->name);
    if (name_cmp != 0) return name_cmp;
    return strcmp(rec1->surname, rec2->surname);
}

bool shouldSwap(Record* rec1, Record* rec2) {
    return compareRecords(rec1, rec2) > 0;
}

void sort_Chunk(CHUNK* chunk) {
    int total_records = chunk->recordsInChunk;
    if (total_records <= 1) return;

    Record* records = malloc(total_records * sizeof(Record));
    if (!records) {
        perror("Memory allocation failed in sort_Chunk");
        return;
    }

    for (int i = 0; i < total_records; i++) {
        if (CHUNK_GetIthRecordInChunk(chunk, i, &records[i]) != 0) {
            fprintf(stderr, "Error reading record %d from chunk in block %d\n", i, chunk->from_BlockId);
            free(records);
            return;
        }
    }

    qsort(records, total_records, sizeof(Record), compareRecords);

    for (int i = 0; i < total_records; i++) {
        if (CHUNK_UpdateIthRecord(chunk, i, records[i]) != 0) {
            fprintf(stderr, "Critical Error: Failed to update record %d in chunk [%d-%d]\n", i, chunk->from_BlockId, chunk->to_BlockId);
            free(records);
            return;
        }
    }

    free(records);
}

void sort_FileInChunks(int file_desc, int numBlocksInChunk) {
    CHUNK_Iterator iterator = CHUNK_CreateIterator(file_desc, numBlocksInChunk);
    CHUNK current_chunk;

    while (CHUNK_GetNext(&iterator, &current_chunk) == 0) {
        sort_Chunk(&current_chunk);
    }
}

void external_Sort(char* input_FileName, int numBlocksInChunk, int bway) {
    int file_desc;
    if (HP_OpenFile(input_FileName, &file_desc) != 0) return;

    // PASS 0: In-place sorting of chunks
    sort_FileInChunks(file_desc, numBlocksInChunk);
    HP_CloseFile(file_desc);

    int current_ChunkSize = numBlocksInChunk;
    int pass_count = 1;
    char current_Input[256];
    char current_Output[256];
    
    strcpy(current_Input, input_FileName);

    while (true) {
        int in_fd;
        if (HP_OpenFile(current_Input, &in_fd) != 0) break;
        
        int last_block = HP_GetIdOfLastBlock(in_fd);
        if (current_ChunkSize >= last_block) {
            HP_CloseFile(in_fd);
            printf("Sorting complete after %d passes.\n", pass_count - 1);
            break;
        }

        sprintf(current_Output, "temp_pass_%d.db", pass_count);
        HP_CreateFile(current_Output);
        
        int out_fd;
        if (HP_OpenFile(current_Output, &out_fd) != 0) {
            HP_CloseFile(in_fd);
            break;
        }

        merge(in_fd, current_ChunkSize, bway, out_fd);

        HP_CloseFile(in_fd);
        HP_CloseFile(out_fd);

        strcpy(current_Input, current_Output);
        current_ChunkSize *= bway; 
        pass_count++;
    }
}