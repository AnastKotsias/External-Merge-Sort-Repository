#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/hp_file.h"
#include "../include/chunk.h"

#define FILE_NAME "test_chunks.db"
#define RECORDS_NUM 50
#define BLOCKS_IN_CHUNK 3

int main() {
    // 1. Προετοιμασία: Δημιουργία και άνοιγμα αρχείου σωρού [cite: 24, 27]
    HP_CreateFile(FILE_NAME);
    int file_desc;
    HP_OpenFile(FILE_NAME, &file_desc);

    // 2. Εισαγωγή τυχαίων εγγραφών για δοκιμή [cite: 34]
    Record record;
    for (int i = 0; i < RECORDS_NUM; i++) {
        record.id = i;
        sprintf(record.name, "Name%d", i);
        sprintf(record.surname, "Surname%d", i);
        sprintf(record.city, "City%d", i);
        HP_InsertEntry(file_desc, record);
    }

    printf("--- Έναρξη Ελέγχου CHUNK Library ---\n");

    // 3. Έλεγχος CHUNK_Iterator: Διάσχιση του αρχείου ανά Chunks [cite: 94, 95]
    CHUNK_Iterator chunk_it = CHUNK_CreateIterator(file_desc, BLOCKS_IN_CHUNK);
    CHUNK chunk;
    int chunk_count = 0;

    while (CHUNK_GetNext(&chunk_it, &chunk) == 0) {
        printf("\n[Chunk %d] Blocks: %d έως %d, Records: %d\n", 
                chunk_count++, chunk.from_BlockId, chunk.to_BlockId, chunk.recordsInChunk);

        // 4. Έλεγχος CHUNK_RecordIterator: Διάσχιση εγγραφών μέσα στο Chunk [cite: 110, 112]
        CHUNK_RecordIterator rec_it = CHUNK_CreateRecordIterator(&chunk);
        Record rec;
        int rec_in_chunk_idx = 0;
        
        while (CHUNK_GetNextRecord(&rec_it, &rec) == 0) {
            // Έλεγχος GetIthRecordInChunk [cite: 98]
            Record ith_rec;
            if (CHUNK_GetIthRecordInChunk(&chunk, rec_in_chunk_idx, &ith_rec) == 0) {
                if (ith_rec.id != rec.id) {
                    printf("ΣΦΑΛΜΑ: Η εγγραφή %d δεν ταιριάζει με την GetIth!\n", rec_in_chunk_idx);
                }
            }
            
            if (rec_in_chunk_idx == 0) {
                printf("  Πρώτη εγγραφή Chunk: %s %s (ID: %d)\n", rec.name, rec.surname, rec.id);
            }
            rec_in_chunk_idx++;
        }
    }

    // 5. Έλεγχος CHUNK_UpdateIthRecord 
    printf("\nΈλεγχος Update στην 1η εγγραφή του τελευταίου Chunk...\n");
    Record update_rec = {999, "Updated", "User", "Athens"};
    CHUNK_UpdateIthRecord(&chunk, 0, update_rec);
    
    Record verify_rec;
    CHUNK_GetIthRecordInChunk(&chunk, 0, &verify_rec);
    printf("Νέα τιμή: %s %s (ID: %d)\n", verify_rec.name, verify_rec.surname, verify_rec.id);

    // Κλείσιμο αρχείου [cite: 30]
    HP_CloseFile(file_desc);
    printf("\n--- Τέλος Ελέγχου ---\n");

    return 0;
}