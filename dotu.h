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
	char magic[4];
	uint32_t versionNum;
	char homeFileSystem[16];
	uint16_t numEntries;
};

struct ExtAttrHeader{
	char headerMagic[4];
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



void printChar(char thisChar){
	if((thisChar>=48 && thisChar<=125)) printf("%c",thisChar);
	else printf("%.2X",thisChar);
}

/* Writes char array to buffer */
void bufWrite(char* buf,long startIndex,char* data,long length){
	long i;
	for(i=0;i<length;i++){
		buf[startIndex+i] = data[i];
		printf("%c",data[i]);
	}
	return;
}

/* Small endian to big */
long toBigEndian(char* charArray, int numBytes){
	int i,total;
	total=0;
	for(i=0;i<numBytes;i++){
		total = total*256 + (unsigned char) charArray[i];
	}
	return total;
}

/* Big endian to small */
char* toSmallEndian(char* num, int numBytes){
	int i;
	char * returnArray=(char *)malloc(sizeof(char)*numBytes);
	for(i=0;i<numBytes;i++){
		returnArray[i]=num[numBytes-i-1];
	}
	return returnArray;
	
}

/* Returns the maximum byte number needed for
   the dot-underscore file. */
long sizeNeeded(struct DotU dotU){
	long max=0;
	int i;
	for(i=0;i<dotU.header.numEntries;i++){
		if(dotU.entry[i].offset+dotU.entry[i].length>max){
			max = dotU.entry[i].offset+dotU.entry[i].length;
		}
	}
	
	return max;
}


/* Returns the header size */
/* TODO: I think there's a bug in here... */
long attrHdrSize(long nameLength){
	/* 4 bytes each for value offset, value length, 
	   2 bytes for flags, 1 bytes for name length
	 Plus the number of bytes in the name
	 Plus 1 for being null-terminated
	 Then round up to the nearest word boundary because
	 of course mem reads are faster that way... */
	long bitFilter = 3L;
	return ((sizeof(char)*(11+(long)nameLength+1) + bitFilter) & ~bitFilter);
}






struct DotU readDotUFile(const char *fileName){
	struct stat statBuffer;
	int i;
	int entryCount,dotUOffset;
	int fileDescriptor;
	char *data;
	FILE *dotUFile;
	char *dotUBuffer;
	char readChar;
	long fileLength;
	
	struct DotU dotU;
	struct ExtAttr *attrs;
	char *entryName;
	char *entryValue;
	long charNum;
	uint8_t entryNameLength;
	long entryValueLength;
	long entryHeaderOffset;
	long entryValueOffset;
	char attrFlags[2];

	/* Note: Using fstat() to obtain size based on advice from
	 https://www.securecoding.cert.org/confluence/display/seccode/FIO19-C.+Do+not+use+fseek%28%29+and+ftell%28%29+to+compute+the+size+of+a+file
	*/
	
	printf("Opening File\n"); /* DEBUG PRINT */
	fileDescriptor = open(fileName,O_RDONLY);
	if(fileDescriptor==-1){
		/* TODO: handle error */
		printf("Error locating dot underscore file.\n");
	}
		
	dotUFile = fdopen(fileDescriptor, "rb");
	if(dotUFile==NULL){
		/* TODO: handle error */
		printf("Error opening dot underscore file.\n");
	}
	
	if(fstat(fileDescriptor, &statBuffer)==-1){
		/* TODO: handle error */
		printf("Error getting dot underscore file stat.\n");
	}
	
	fileLength = statBuffer.st_size;
	dotUBuffer = (char*)malloc(fileLength);
	if(dotUBuffer==NULL){
		/* TODO: handle error */
		printf("Error allocating dot underscore file buffer.\n");
	}
	
	printf("Reading File\n"); /* DEBUG PRINT */

	for (i = 0; (readChar = getc(dotUFile)) != EOF && i < fileLength; dotUBuffer[i++] = readChar) printChar(readChar);
	fclose(dotUFile);
	
	/* Fill dotU struct */
	printf("Setting up header\n"); /* DEBUG PRINT */
	/* dotU header */
	for(i=0;i<16;i++) dotU.header.magic[i] = (char) dotUBuffer[i];
	dotU.header.versionNum = (uint32_t) toBigEndian(&(dotUBuffer[4]),4);
	for(i=0;i<16;i++) dotU.header.homeFileSystem[i] = (char) dotUBuffer[8+i];
	dotU.header.numEntries = (uint16_t) toBigEndian(&dotUBuffer[24],2);
	
	
	
	/* The dotU file has various entries.
	/ Extended attributes are usually in the Finder Info.*/
	printf("Setting up dotu entries\n"); /* DEBUG PRINT */

	dotUOffset=26;
	for(entryCount=0;entryCount<dotU.header.numEntries;entryCount++){
		dotU.entry[entryCount].id = (uint32_t) toBigEndian(&dotUBuffer[dotUOffset],4);
		dotU.entry[entryCount].offset = (uint32_t) toBigEndian(&dotUBuffer[dotUOffset+4],4);
		dotU.entry[entryCount].length = (uint32_t) toBigEndian(&dotUBuffer[dotUOffset+8],4);
		
		/* Data set up according to needs of entry type */
		switch(dotU.entry[entryCount].id){
			case 2:{
				printf("Setting up resource fork\n"); /* DEBUG PRINT */
				data=(char*)malloc(dotU.entry[entryCount].length);
				for(i=0;i<dotU.entry[entryCount].length;i++) data[i]=(char)dotUBuffer[dotU.entry[entryCount].offset+i];
				dotU.entry[entryCount].data.resource.data=data;
			}break;
			case 9:{
				printf("Setting up finder info\n");/* DEBUG PRINT */
				for(i=0;i<32;i++){
					dotU.entry[entryCount].data.finder.finderHeader[i]          = dotUBuffer[dotU.entry[entryCount].offset+i];
				}
				for(i=0;i<2;i++){
					dotU.entry[entryCount].data.finder.padding[i]               = dotUBuffer[dotU.entry[entryCount].offset+32+i];
				}
				for(i=0;i<4;i++){
				dotU.entry[entryCount].data.finder.xattrHdr.headerMagic[i]    = dotUBuffer[dotU.entry[entryCount].offset+34+i];
				}
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
				
				printf("Setting up xattrs\n"); /* DEBUG PRINT */
				attrs=(struct ExtAttr*)malloc(sizeof(struct ExtAttr) * dotU.entry[entryCount].data.finder.xattrHdr.numAttrs);
				entryHeaderOffset=dotU.entry[entryCount].offset+70;
				for(i=0;i<dotU.entry[entryCount].data.finder.xattrHdr.numAttrs;i++){
					/* Debug printing */
					printf("Setting up xattr %i :\n",i);

					entryValueOffset = toBigEndian(&dotUBuffer[entryHeaderOffset],4);
					entryValueLength = toBigEndian(&dotUBuffer[entryHeaderOffset+4],4);
					for(charNum=0;charNum<=1;charNum++){
						attrFlags[charNum]=dotUBuffer[entryHeaderOffset+8+charNum];
					}
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
					attrs[i].value=entryValue;
					attrs[i].valueOffset=entryValueOffset;
					attrs[i].valueLength=entryValueLength;
					for(charNum=0;charNum<2;charNum++) attrs[i].flags[charNum]=attrFlags[charNum];
					attrs[i].nameLength=entryNameLength;
										
					/* Debug printing */
					printf("\tNameOffset:  %li\tNameLength:  %i\t Name:  %s\n",entryHeaderOffset+11,(int) entryNameLength,entryName);
					printf("\tValueOffset: %li\tValueLength: %li\t Value: %s\n",entryValueOffset,entryValueLength,entryValue);
					
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

int createDotUFile(struct DotU dotU, const char * parentFileName){
	char *dotUFileName;
	char *prefix = "a._";
	char *fileBuffer;
	FILE *dotUFile;
	long i,j,k;
	long bufferSize;
	long bufIndex;
	
	bufferSize = sizeNeeded(dotU);
	fileBuffer=(char *)malloc(sizeof(char)*bufferSize);
	
	for(i=0;i<bufferSize;i++){
		fileBuffer[i]='\0';
	}
	
		/* Writing DotU File Header */
	/* 4 bytes  - magic num
	   4 bytes  - version num
	   16 bytes - home file system
	   2 bytes  - number of dotU entries (usually 2)
	*/
	bufWrite(fileBuffer,0,dotU.header.magic,4);
	bufWrite(fileBuffer,4,toSmallEndian((char*)&dotU.header.versionNum,4),4);
	bufWrite(fileBuffer,8,dotU.header.homeFileSystem,16);
	bufWrite(fileBuffer,24,toSmallEndian((char*)&dotU.header.numEntries,2),2);
	
	/* DotU entry list - For each:
	   4 bytes - ID
	   4 bytes - offset
	   4 bytes - length
	*/
		bufIndex=26;
	for(i=0;i<dotU.header.numEntries;i++){
		bufWrite(fileBuffer,bufIndex,toSmallEndian((char*)&dotU.entry[i].id,4),4);
		bufWrite(fileBuffer,bufIndex+4,toSmallEndian((char*)&dotU.entry[i].offset,4),4);
		bufWrite(fileBuffer,bufIndex+8,toSmallEndian((char*)&dotU.entry[i].length,4),4);
		bufIndex+=12;
	}
	
	/* Now write each entry */
	for(i=0;i<dotU.header.numEntries;i++){
		switch(dotU.entry[i].id){
			/* Resource */
			case 2:{
				/* TODO: pad this if needed with 1's? */
				bufWrite(fileBuffer,dotU.entry[i].offset,dotU.entry[i].data.resource.data,dotU.entry[i].length);
			} break;
			
			/* Finder Info, where xattrs live */
			case 9: {
				/* Next comes the Finder Info and Xattr Header (DotU entry ID == 9)
				   TODO: Verify that the offset and length of Finder Info are what would be expected.
				   32 bytes  - header
				   2 bytes   - padding

				   4 bytes   - xattr Magic Num 
				   4 bytes   - debug tag (parent file id)
				   4 bytes   - size of ...?
				   4 bytes   - file offset for xattr data
				   4 bytes   - length of xattr data block
				   12 bytes  - reserved ...?
				   2 bytes   - attribute flags
				   2 bytes   - number of xattrs
				*/
					bufWrite(fileBuffer,dotU.entry[i].offset,dotU.entry[i].data.finder.finderHeader,32);
					bufWrite(fileBuffer,dotU.entry[i].offset+32,dotU.entry[i].data.finder.padding,2);

					bufWrite(fileBuffer,dotU.entry[i].offset+34,dotU.entry[i].data.finder.xattrHdr.headerMagic,4);
					bufWrite(fileBuffer,dotU.entry[i].offset+38,toSmallEndian((char*)&dotU.entry[i].data.finder.xattrHdr.debugTag,4),4);
					bufWrite(fileBuffer,dotU.entry[i].offset+42,toSmallEndian((char*)&dotU.entry[i].data.finder.xattrHdr.size,4),4);
					bufWrite(fileBuffer,dotU.entry[i].offset+46,toSmallEndian((char*)&dotU.entry[i].data.finder.xattrHdr.attrDataOffset,4),4);
					bufWrite(fileBuffer,dotU.entry[i].offset+50,toSmallEndian((char*)&dotU.entry[i].data.finder.xattrHdr.attrDataLength,4),4);
					bufWrite(fileBuffer,dotU.entry[i].offset+54,dotU.entry[i].data.finder.xattrHdr.attrReserved,12);
					bufWrite(fileBuffer,dotU.entry[i].offset+66,dotU.entry[i].data.finder.xattrHdr.attrFlags,2);
					bufWrite(fileBuffer,dotU.entry[i].offset+68,toSmallEndian((char*)&dotU.entry[i].data.finder.xattrHdr.numAttrs,2),2);
					
					/* Now write the xattrs */
					bufIndex=dotU.entry[i].data.finder.xattrHdr.attrDataOffset;
					for(j=0;j<dotU.entry[i].data.finder.xattrHdr.numAttrs;j++){
						/* Each xattr has a header part and a data part.
						   The header part is:
						   4 bytes             - Value offset (from beginning of DotU file)
						   4 bytes             - value length
						   2 bytes             - flags 
						TODO: Figure out what the flags are.
						   1 byte              - name length (128 byte max)
						   name length bytes   - name (null-terminated)
						   padding (if needed) - to nearest word (4-byte) boundary
						
						   The value part is in the heap after all of the headers, 
						   and position is specified by the value's offset and length. */
							bufWrite(fileBuffer,bufIndex,toSmallEndian((char*)&dotU.entry[i].data.finder.attr[j].valueOffset,4),4);
							bufWrite(fileBuffer,bufIndex+4,toSmallEndian((char*)&dotU.entry[i].data.finder.attr[j].valueLength,4),4);
							/* Technically, the nameLength should only be 2 bytes long, but there shouldn't
							   be harm in padding with 0's. */
							bufWrite(fileBuffer,bufIndex+8,dotU.entry[i].data.finder.attr[j].flags,2); 
							bufWrite(fileBuffer,bufIndex+10,(char*)&dotU.entry[i].data.finder.attr[j].nameLength,1); 
							/* The name is stored in the dot-u file as a null-terminated string, 128 bytes max */
							bufWrite(fileBuffer,bufIndex+12,dotU.entry[i].data.finder.attr[j].name,dotU.entry[i].data.finder.attr[j].nameLength+1);
							printf("\nwriting :");
							for(k=0;k<dotU.entry[i].data.finder.attr[j].nameLength+1;k++) printf("%c",dotU.entry[i].data.finder.attr[j].name[k]);
							
							/* Write the value of the attr */
							bufWrite(fileBuffer,dotU.entry[i].data.finder.attr[j].valueOffset,dotU.entry[i].data.finder.attr[j].value, dotU.entry[i].data.finder.attr[j].valueLength);
							
							bufIndex+=attrHdrSize(dotU.entry[i].data.finder.attr[j].nameLength);
					}
					
					
			}break;
			
			/* Other/Unknown */
			default:{ 
				printf("Unknown DotU entry type.  Cannot write.\n");
			} break;
		}
		
	
	}
	/* Last 2 bytes should be EOF */
	for(i=1;i<=2;i++) fileBuffer[bufferSize-i]=EOF;
	
	
		
	
	printf("Allocating filename for ._\n");
	printf("Parent file name is %s and is %i characters long.\n",parentFileName,(int)strlen(parentFileName));
	dotUFileName=(char*)malloc(sizeof(char) * (strlen(parentFileName)+strlen(prefix)));
	strcpy(dotUFileName,prefix);
	/* TODO: Max file name size? */
	dotUFileName=strncat(dotUFileName,parentFileName,(strlen(parentFileName)+strlen(prefix)));
	printf("Child file name is %s %s %s and is %i characters long.\n",prefix,parentFileName,dotUFileName,(int) (strlen(parentFileName)+strlen(prefix)));
	
	printf("Output file: %s\n",dotUFileName);
	
	/* Create and write file */
	dotUFile = fopen(dotUFileName, "wb");
	fwrite(fileBuffer,1,bufferSize,dotUFile);
	fclose(dotUFile);
	return 0;
	
}


#endif