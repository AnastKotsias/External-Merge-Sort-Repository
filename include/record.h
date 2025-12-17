#ifndef RECORD_H
#define RECORD_H


typedef struct Record {
  	char name[20];			// increased the size a little bit
	char surname[20];
	char city[20];
	int id;	
	char delimiter[2];
} Record;

Record randomRecord();

void printRecord(Record record);

#endif
