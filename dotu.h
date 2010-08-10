/*
 This file provides the means to read dot-underscore
 (._) files that are formed when copying a file
 from an OSX machine onto a *nix.
 
 Author: Kelli Ireland
 Summer 2010
 As part of the Google Summer of Code
*/


#ifndef DOTU_H
#define DOTU_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>




struct DotUHeader {
	uint32_t magic;
	uint32_t versionNum;
	char homeFileSystem[16];
	uint16_t numEntries;
};

struct ExtAttrHeader{
	uint32_t headerMagic;
	uint32_t debugTag;
	uint32_t size;
	uint32_t attrDataOffset;
	uint32_t attrDataLength;
	char attrReserved[12];
	char attrFlags[2];
	uint16_t numAttrs;
};

struct ExtAttr{
	uint32_t valueOffset;
	uint32_t valueLength;
	char flags[2];
	uint8_t nameLength;
	/* The name is stored in the dot-u file as a null-terminated string, 128 bytes max */
	char * name; 
	char * value;
	
};


struct FinderEntry {
	/* If entry id == 9, it's the Finder entry. */
	char finderHeader[32];
	/* two bytes padding */
	char padding[2];
	struct ExtAttrHeader xattrHdr;
	struct ExtAttr * attr;
};


struct ResourceEntry {
	char * data;
};

union entryData {
	struct ResourceEntry resource;
	struct FinderEntry finder;
};


struct DotUEntry {
	uint32_t id;
	uint32_t offset;
	uint32_t length;
	union entryData data;
};


struct DotU {
	struct DotUHeader header;
	struct DotUEntry entry[2];
};

void printChar(char thisChar);

void bufWrite(char* buf,uint32_t startIndex,char* data,uint32_t length);

uint32_t toBigEndian(char* charArray, uint32_t numBytes);

char* toSmallEndian(char* num, uint32_t numBytes);

/* Returns the maximum byte number needed for
   the dot-underscore file. */
uint32_t sizeNeeded(struct DotU dotU);

uint32_t roundup4096(uint32_t size);

uint32_t attrHdrSize(uint32_t nameLength);

struct DotU readDotUFile(const char *fileName);

int createDotUFile(struct DotU dotU, const char * parentFileName, const char * suffix);

int setOffsets(struct DotU * dotU);

int addAttr(struct DotU * dotU, const char * name, const char * value);

/* Remove an extended attribute.  Return 0 if good, -1 if fail */
int rmAttr(struct DotU * dotU, const char * name);

char* getAttrValue(struct DotU dotU, const char * name);

int getAttrIndex(struct DotU dotU, const char * name);

int getFinderInfoEntry(struct DotU dotU);

void listAttrs(struct DotU dotU);

void printDotUDetail(struct DotU dotU);

struct DotU iniDotU(const char * parentFileName);

#endif