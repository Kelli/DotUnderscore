#ifndef DOTU_H
#define DOTU_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>


void printChar(char thisChar){
	if((thisChar>=48 && thisChar<=125)) printf("%c",thisChar);
	else printf("%X",thisChar);
}


long toBigEndian(char* charArray, int numBytes){
	int i,total;
	total=0;
	for(i=0;i<numBytes;i++){
		total = total*256 + (unsigned char) charArray[i];
	}
	return total;
}

// Returns the header size
// TODO: I think there's a bug in here...
long attrHdrSize(long nameLength){
	// 4 bytes each for value offset, value length, and name length
	// Plus the number of bytes in the name
	// Then round up to the nearest word boundary because
	// of course it's faster that way.
	return ((12*sizeof(char)+(long)nameLength+2) / 4) * 4;
}


void dp(char* thisString){
	printf("%s",thisString);
}


struct DotUHeader {
	uint32_t magicNum;
	uint32_t versionNum;
	char homeFileSystem[16];
	uint16_t numEntries;
};

struct ExtAttrHeader{
	uint32_t headerMagicNum;
	uint32_t debugTag;
	uint32_t size;
	uint32_t attrDataOffset;
	uint32_t attrDataLength;
	char attrReserved[12];
	char attrFlags[2];
	uint16_t numAttrs;
};

struct ExtAttr{
	char * name;
	char * data;
};


struct FinderEntry {
	// If entry id == 9, it's the Finder entry.
	char finderHeader[32];
	// two bytes padding
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

struct DotU createDotU(const char *fileName){
	struct stat statBuffer;
	int i;
	
	// Note: Using fstat() to obtain size based on advice from
	// https://www.securecoding.cert.org/confluence/display/seccode/FIO19-C.+Do+not+use+fseek%28%29+and+ftell%28%29+to+compute+the+size+of+a+file
	
	dp("Opening File\n");
	int fileDescriptor = open(fileName,O_RDONLY);
	if(fileDescriptor==-1){
		// TODO: handle error
		printf("Error locating dot underscore file.\n");
	}
		
	FILE *dotUFile = fdopen(fileDescriptor, "rb");
	if(dotUFile==NULL){
		// TODO: handle error
		printf("Error opening dot underscore file.\n");
	}
	
	if(fstat(fileDescriptor, &statBuffer)==-1){
		// TODO: handle error
		printf("Error getting dot underscore file stat.\n");
	}
	
	long fileLength = statBuffer.st_size;
	char *dotUBuffer = (char*)malloc(fileLength);
	if(dotUBuffer==NULL){
		// TODO: handle error
		printf("Error allocating dot underscore file buffer.\n");
	}
	
	dp("Reading File\n");
	char readChar;
	for (i = 0; (readChar = getc(dotUFile)) != EOF && i < fileLength; dotUBuffer[i++] = readChar); //printChar(readChar);

	fclose(dotUFile);
	
	
	// Fill dotU struct
	struct DotU dotU;
	dp("Setting up header\n");
	// dotU header
	dotU.header.magicNum = (uint32_t) toBigEndian(&dotUBuffer[0],4);
	dotU.header.versionNum = (uint32_t) toBigEndian(&(dotUBuffer[4]),4);
	for(i=0;i<16;i++) dotU.header.homeFileSystem[i] = (char) dotUBuffer[8+i];
	dotU.header.numEntries = (uint16_t) toBigEndian(&dotUBuffer[24],2);
	
	
	
	// The dotU file has various entries.
	// Extended attributes are usually in the Finder Info.
	dp("Setting up dotu entries\n");
	int entryCount,dotUOffset;
	dotUOffset=26;
	for(entryCount=0;entryCount<dotU.header.numEntries;entryCount++){
		dotU.entry[entryCount].id = (uint32_t) toBigEndian(&dotUBuffer[dotUOffset],4);
		dotU.entry[entryCount].offset = (uint32_t) toBigEndian(&dotUBuffer[dotUOffset+4],4);
		dotU.entry[entryCount].length = (uint32_t) toBigEndian(&dotUBuffer[dotUOffset+8],4);
		
		// Data set up according to needs of entry type
		switch(dotU.entry[entryCount].id){
			case 2:{
				dp("Setting up resource fork\n");
				char *data=(char*)malloc(dotU.entry[entryCount].length);
				for(i=0;i<dotU.entry[entryCount].length;i++) data[i]=(char)dotUBuffer[dotU.entry[entryCount].offset+i];
				dotU.entry[entryCount].data.resource.data=data;
			}break;
			case 9:{
				dp("Setting up finder info\n");
				for(i=0;i<32;i++){
					dotU.entry[entryCount].data.finder.finderHeader[i]          = dotUBuffer[dotU.entry[entryCount].offset+i];
				}
				for(i=0;i<2;i++){
					dotU.entry[entryCount].data.finder.padding[i]               = dotUBuffer[dotU.entry[entryCount].offset+32+i];
				}
				dotU.entry[entryCount].data.finder.xattrHdr.headerMagicNum    = (uint32_t) toBigEndian(&dotUBuffer[dotU.entry[entryCount].offset+34],4);
				dotU.entry[entryCount].data.finder.xattrHdr.debugTag          = (uint32_t) toBigEndian(&dotUBuffer[dotU.entry[entryCount].offset+38],4);
				dotU.entry[entryCount].data.finder.xattrHdr.size              = (uint32_t) toBigEndian(&dotUBuffer[dotU.entry[entryCount].offset+42],4);
				dotU.entry[entryCount].data.finder.xattrHdr.attrDataOffset    = (uint32_t) toBigEndian(&dotUBuffer[dotU.entry[entryCount].offset+46],4);
				dotU.entry[entryCount].data.finder.xattrHdr.attrDataLength    = (uint32_t) toBigEndian(&dotUBuffer[dotU.entry[entryCount].offset+50],4);
				for(i=0;i<12;i++){
					dotU.entry[entryCount].data.finder.xattrHdr.attrReserved[i] = dotUBuffer[dotU.entry[entryCount].offset+54+i];
				}
				for(i=0;i<2;i++){
					dotU.entry[entryCount].data.finder.xattrHdr.attrFlags[i]    = dotUBuffer[dotU.entry[entryCount].offset+66+i];
				}
				dotU.entry[entryCount].data.finder.xattrHdr.numAttrs          = (uint16_t) toBigEndian(&dotUBuffer[dotU.entry[entryCount].offset+68],2);
				
				dp("Setting up xattrs\n");
				struct ExtAttr *attrs=(struct ExtAttr*)malloc(sizeof(struct ExtAttr) * dotU.entry[entryCount].data.finder.xattrHdr.numAttrs);
				
				char *entryName;
				char *entryValue;
				long charNum;
				long entryNameLength, entryValueLength;
				long entryHeaderOffset=dotU.entry[entryCount].offset+70;
				long entryValueOffset;
				for(i=0;i<dotU.entry[entryCount].data.finder.xattrHdr.numAttrs;i++){
					// Debug printing
					printf("Setting up xattr %i :\n",i);

					entryValueOffset = toBigEndian(&dotUBuffer[entryHeaderOffset],4);
					entryValueLength = toBigEndian(&dotUBuffer[entryHeaderOffset+4],4);
					entryNameLength  = dotUBuffer[entryHeaderOffset+10];
					
					entryName=malloc(sizeof(char)*entryNameLength+1);
					entryValue=malloc(sizeof(char)*entryValueLength+1);
					
					for(charNum=0;charNum<=entryNameLength;charNum++){
						entryName[charNum]=dotUBuffer[entryHeaderOffset+11+charNum];
					}
					entryName[entryNameLength]='\0';
					for(charNum=0;charNum<=entryValueLength;charNum++){
						entryValue[charNum]=dotUBuffer[entryValueOffset+charNum];
					}
					entryValue[entryValueLength]='\0';
					
					attrs[i].name=entryName;
					attrs[i].data=entryValue;

					// Debug printing
					printf("\tNameOffset:  %li\tNameLength:  %li\t Name: %s\n",entryHeaderOffset+11,entryNameLength,entryName);
					printf("\tValueOffset: %li\tValueLength: %li\t Data: %s\n",entryValueOffset,entryValueLength,entryValue);
					
					entryHeaderOffset+=attrHdrSize(entryNameLength);
				}
				
				dotU.entry[entryCount].data.finder.attr=attrs;
				
			}break;
			default:{
				printf("\nError.  Unknown Dot-Underscore Entry ID type.");
			}break;
		}
		dotUOffset+=12;
	}
	return dotU;
} 


#endif