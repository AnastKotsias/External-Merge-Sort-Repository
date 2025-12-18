#include <stdio.h>
#include <stdlib.h>
#include "bf.h"
#include "sort.h"
#include "hp_file.h"

#define TEST_RECORDS 100

void run_experiment(int blocksInChunk, int bWay) {
    char name[32];
    sprintf(name, "exp_%d_%d.db", blocksInChunk, bWay);
    
    // Clean up any leftover files
    remove(name);
    remove("temp_pass_1.db");
    remove("temp_pass_2.db");
    
    // 1. Create and fill file with TEST_RECORDS records
    HP_CreateFile(name);
    int fd;
    HP_OpenFile(name, &fd);
    for(int i=0; i<TEST_RECORDS; i++) {
        HP_InsertEntry(fd, randomRecord());
    }
    HP_CloseFile(fd);

    printf("\n--- Experiment: ChunkSize=%d, %d-way Merge ---\n", blocksInChunk, bWay);
    // 2. Run the sort (ensure external_Sort prints the pass count)
    external_Sort(name, blocksInChunk, bWay);
}

int main() {
    BF_Init(LRU);
    
    // 5-block chunks, 2-way merge
    run_experiment(5, 2);
    
    // 5-block chunks, 10-way merge
    run_experiment(5, 10);

    BF_Close();
    return 0;
}