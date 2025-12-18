#ifndef RECORD_H
#define RECORD_H


typedef struct Record {
    int id;           // Move this to the top
    char name[15]; // alignment fix
    char surname[20];
    char city[20];
    // char delimiter[2];, /* Removed 'delimiter' as it's usually handled internally by the HP layer *
} Record;

Record randomRecord();

void printRecord(Record record);

#endif
