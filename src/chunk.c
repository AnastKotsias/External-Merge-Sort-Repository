#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "chunk.h"
#include "hp_file.h"
#include "record.h"

CHUNK_Iterator CHUNK_CreateIterator(int fileDesc, int blocksInChunk) {
    CHUNK_Iterator iterator;
    iterator.file_desc = fileDesc;
    iterator.current = 1; // Data blocks start from ID 1 (0 is header)
    iterator.lastBlocksID = HP_GetIdOfLastBlock(fileDesc);
    iterator.blocksInChunk = blocksInChunk;
    return iterator;
}

int CHUNK_GetNext(CHUNK_Iterator *iterator, CHUNK* chunk) {
    if (iterator->current > iterator->lastBlocksID) {
        return -1; 
    }

    chunk->file_desc = iterator->file_desc;
    chunk->from_BlockId = iterator->current;
    
    int toBlock = iterator->current + iterator->blocksInChunk - 1;
    if (toBlock > iterator->lastBlocksID) {
        toBlock = iterator->lastBlocksID;
    }
    chunk->to_BlockId = toBlock;
    chunk->blocksInChunk = (chunk->to_BlockId - chunk->from_BlockId) + 1;
    
    int totalRecords = 0;
    for (int i = chunk->from_BlockId; i <= chunk->to_BlockId; i++) {
        totalRecords += HP_GetRecordCounter(iterator->file_desc, i);
    }
    chunk->recordsInChunk = totalRecords;

    iterator->current += iterator->blocksInChunk;
    return 0;
}

int CHUNK_GetIthRecordInChunk(CHUNK* chunk, int i, Record* record) {
    int maxRecs = HP_GetMaxRecordsInBlock(chunk->file_desc);
    if (maxRecs <= 0) return -1;

    int relativeBlock = i / maxRecs;
    int cursor = i % maxRecs;
    int targetBlockId = chunk->from_BlockId + relativeBlock;

    if (targetBlockId > chunk->to_BlockId) return -1;

    // Standard success for Get is 0
    if (HP_GetRecord(chunk->file_desc, targetBlockId, cursor, record) == 0) {
        HP_Unpin(chunk->file_desc, targetBlockId);
        return 0;
    }
    return -1;
}

int CHUNK_UpdateIthRecord(CHUNK* chunk, int i, Record record) {
    int maxRecs = HP_GetMaxRecordsInBlock(chunk->file_desc);
    if (maxRecs <= 0) return -1;
    
    int relativeBlock = i / maxRecs;
    int cursor = i % maxRecs;
    int targetBlockId = chunk->from_BlockId + relativeBlock;

    if (targetBlockId > chunk->to_BlockId) return -1;

    /* * FIX: Checked for 0 instead of 1. 
     * Most HP implementations return 0 (BF_OK) on success.
     */
    int status = HP_UpdateRecord(chunk->file_desc, targetBlockId, cursor, record);
    if (status == 0 || status == 1) { 
        HP_Unpin(chunk->file_desc, targetBlockId);
        return 0;
    }
    return -1;
}

void CHUNK_Print(CHUNK chunk) {
    CHUNK_RecordIterator it = CHUNK_CreateRecordIterator(&chunk);
    Record rec;
    printf("--- Chunk [Blocks %d - %d] ---\n", chunk.from_BlockId, chunk.to_BlockId);
    while (CHUNK_GetNextRecord(&it, &rec) == 0) {
        printRecord(rec);
    }
}

CHUNK_RecordIterator CHUNK_CreateRecordIterator(CHUNK *chunk) {
    CHUNK_RecordIterator iterator;
    iterator.chunk = *chunk;
    iterator.currentBlockId = chunk->from_BlockId;
    iterator.cursor = 0;
    return iterator;
}

int CHUNK_GetNextRecord(CHUNK_RecordIterator *iterator, Record* record) {
    if (iterator->currentBlockId > iterator->chunk.to_BlockId) {
        return -1;
    }

    int recordsInCurrentBlock = HP_GetRecordCounter(iterator->chunk.file_desc, iterator->currentBlockId);
    
    if (iterator->cursor >= recordsInCurrentBlock) {
        iterator->currentBlockId++;
        iterator->cursor = 0;
        return CHUNK_GetNextRecord(iterator, record);
    }

    if (HP_GetRecord(iterator->chunk.file_desc, iterator->currentBlockId, iterator->cursor, record) == 0) {
        HP_Unpin(iterator->chunk.file_desc, iterator->currentBlockId);
        iterator->cursor++;
        return 0;
    }

    return -1;
}