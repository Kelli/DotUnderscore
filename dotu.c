#include "dotu.h"


void 
printChar(char thisChar){
	if((thisChar>=48 && thisChar<=125)) printf("%c",thisChar);
	else printf("%.2X",thisChar);
}

/* Writes char array to buffer */
void 
bufWrite(char* buf,uint32_t startIndex,char* data,uint32_t length){
	uint32_t i;
	for(i=0;i<length;i++){
		buf[startIndex+i] = data[i];
		
	}
	return;
}

/* Small endian to big */
uint32_t 
toBigEndian(char* charArray, uint32_t numBytes){
	uint32_t i,total;
	total=0;
	for(i=0;i<numBytes;i++){
		total = total*256 + (unsigned char) charArray[i];
	}
	return total;
}

/* Big endian to small */
char* 
toSmallEndian(char* num, uint32_t numBytes){
	int i;
	char * returnArray=(char *)malloc(sizeof(char)*numBytes);
	for(i=0;i<numBytes;i++){
		returnArray[i]=num[numBytes-i-1];
	}
	return returnArray;
	
}

/* Returns the maximum byte number needed for
   the dot-underscore file. */
uint32_t 
sizeNeeded(struct DotU dotU){
	uint32_t max=0;
	int i;
	for(i=0;i<dotU.header.numEntries;i++){
		if(dotU.entry[i].offset+dotU.entry[i].length>max){
			max = dotU.entry[i].offset+dotU.entry[i].length;
		}
	}
	return max;
}

/* Rounds up to nearest 4096 - dotU file size */
uint32_t roundup4096(uint32_t size){
	uint32_t bitFilter = 4095L;
	return (size + bitFilter) & ~bitFilter;
}


/* Returns the header size */
uint32_t 
attrHdrSize(uint32_t nameLength){
	/* 4 bytes each for value offset, value length, 
	   2 bytes for flags, 1 bytes for name length
	 Plus the number of bytes in the name
	 *** NOT Plus 1 for being null-terminated   *** Name length includes the \0.
	 Then round up to the nearest 4-byte word boundary because
	 of course mem reads are faster that way... */
	uint32_t bitFilter = 3L;
	return ((sizeof(char)*(11+(uint32_t)nameLength /* +1 */) + bitFilter) & ~bitFilter);
}

struct DotU 
readDotUFile(const char *fileName){
	struct stat statBuffer;
	uint32_t i;
	uint32_t entryCount,dotUOffset;
	int fileDescriptor;
	char *data;
	FILE *dotUFile;
	char *dotUBuffer;
	int readChar;
	uint32_t fileLength;
	
	struct DotU dotU;
	struct ExtAttr *attrs;
	char *entryName;
	char *entryValue;
	uint32_t charNum;
	uint8_t entryNameLength;
	uint32_t entryValueLength;
	uint32_t entryHeaderOffset;
	uint32_t entryValueOffset;
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
	dotUBuffer = (char*)malloc(sizeof(char)*fileLength);
	printf("Creating buffer of size: %li\n",fileLength);
	if(dotUBuffer==NULL){
		/* TODO: handle error */
		printf("Error allocating dot underscore file buffer.\n");
	}
	
	printf("Reading File\n"); /* DEBUG PRINT */

	for (i = 0; (readChar = getc(dotUFile)) != EOF && i < fileLength; dotUBuffer[i++] = readChar) /* DEBUG ONLY */ printChar(readChar) ;
	fclose(dotUFile);
	
	/* Fill dotU struct */
	printf("Setting up header\n"); /* DEBUG PRINT */
	/* dotU header */
	dotU.header.magic      = (u_int32_t) toBigEndian(&dotUBuffer[4],4);
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
				dotU.entry[entryCount].data.finder.xattrHdr.headerMagic       = (uint32_t) toBigEndian(&dotUBuffer[dotU.entry[entryCount].offset+34],4);
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
					
					/* Entry name length includes \0, but entry value length does not. */
					entryName=malloc(sizeof(char)*entryNameLength /* +1 */);
					entryValue=malloc(sizeof(char)*entryValueLength+1);
					
					for(charNum=0;charNum<=entryNameLength;charNum++){
						entryName[charNum]=dotUBuffer[entryHeaderOffset+11+charNum];
					}
					/*entryName[entryNameLength]='\0';  Uneeded - see above*/
					for(charNum=0;charNum<entryValueLength;charNum++){
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

int 
createDotUFile(struct DotU dotU, const char * parentFileName, const char * suffix){
	char *dotUFileName;
	char *fileBuffer;
	FILE *dotUFile;
	uint32_t i,j,k;
	uint32_t bufferSize;
	uint32_t bufIndex;
	
	/* Make sure all of the offsets are good before going any further */
	if(setOffsets(&dotU)!=0){
		printf("Error setting offsets.\n");
		return -1;
	}
	
	
	
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
	bufWrite(fileBuffer,0,toSmallEndian((char*)&dotU.header.magic,4),4);
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

					bufWrite(fileBuffer,dotU.entry[i].offset+34,toSmallEndian((char*)&dotU.entry[i].data.finder.xattrHdr.headerMagic,4),4);
					bufWrite(fileBuffer,dotU.entry[i].offset+38,toSmallEndian((char*)&dotU.entry[i].data.finder.xattrHdr.debugTag,4),4);
					bufWrite(fileBuffer,dotU.entry[i].offset+42,toSmallEndian((char*)&dotU.entry[i].data.finder.xattrHdr.size,4),4);
					bufWrite(fileBuffer,dotU.entry[i].offset+46,toSmallEndian((char*)&dotU.entry[i].data.finder.xattrHdr.attrDataOffset,4),4);
					bufWrite(fileBuffer,dotU.entry[i].offset+50,toSmallEndian((char*)&dotU.entry[i].data.finder.xattrHdr.attrDataLength,4),4);
					bufWrite(fileBuffer,dotU.entry[i].offset+54,dotU.entry[i].data.finder.xattrHdr.attrReserved,12);
					bufWrite(fileBuffer,dotU.entry[i].offset+66,dotU.entry[i].data.finder.xattrHdr.attrFlags,2);
					bufWrite(fileBuffer,dotU.entry[i].offset+68,toSmallEndian((char*)&dotU.entry[i].data.finder.xattrHdr.numAttrs,2),2);
					
					/* Now write the xattrs */
					bufIndex=dotU.entry[i].offset+70;
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
							bufWrite(fileBuffer,bufIndex+11,dotU.entry[i].data.finder.attr[j].name,dotU.entry[i].data.finder.attr[j].nameLength);
				


	/*printf("\nBuffer is: Writing at buf index: %li\n",bufIndex+11);
	for(k=0;k<bufferSize;k++) printChar(fileBuffer[k]);
*/

							printf("%s : %s at %u : %u\n",dotU.entry[i].data.finder.attr[j].name,dotU.entry[i].data.finder.attr[j].value,bufIndex+10,dotU.entry[i].data.finder.attr[j].valueOffset);
							/* Write the value of the attr */
							bufWrite(fileBuffer,dotU.entry[i].data.finder.attr[j].valueOffset,dotU.entry[i].data.finder.attr[j].value, dotU.entry[i].data.finder.attr[j].valueLength);
							
/*	printf("\nBuffer is: Writing at buf index: %u\n",dotU.entry[i].data.finder.attr[j].valueOffset);
	for(k=0;k<bufferSize;k++) printChar(fileBuffer[k]);
*/
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
	
	/* For debugging - delete or leave commented forever.
	printf("\nBuffer is:\n");
	for(i=0;i<bufferSize;i++) printChar(fileBuffer[i]);
	*/
	
	
		
	
	printf("\nAllocating filename for ._\n");
	printf("Parent file name is %s and is %i characters long.\n",parentFileName,(int)strlen(parentFileName));
	dotUFileName=(char*)malloc(sizeof(char) * (strlen(parentFileName)+strlen(suffix)));
	/*strcpy(dotUFileName,prefix); */
	strcpy(dotUFileName,parentFileName);
	/* TODO: Max file name size? */
	/*dotUFileName=strncat(dotUFileName,parentFileName,(strlen(parentFileName)+strlen(prefix)));*/
	dotUFileName=strncat(dotUFileName,suffix,(strlen(parentFileName)+strlen(suffix)));
	printf("Child file name is %s %s %s and is %i characters long.\n",suffix,parentFileName,dotUFileName,(int) (strlen(parentFileName)+strlen(suffix)));
	
	printf("Output file: %s\n",dotUFileName);
	
	/* Create and write file */
	dotUFile = fopen(dotUFileName, "wb");
	fwrite(fileBuffer,1,bufferSize,dotUFile);
	fclose(dotUFile);
	return 0;
	
}

int 
setOffsets(struct DotU *dotU){
	uint32_t i,j;
	uint32_t currentValueOffset,currentNameOffset;
	uint32_t sizeNeeded, sizeResource, sizeFinder;
	/* 
	Set dotU entry  (resource, finder)
		offsets
		lengths
	
	Set offsets for all xattr values
	   Set lengths for all xattr names and values
	In xattr header:
	   	uint32_t size;
			uint32_t attrDataOffset;
			uint32_t attrDataLength;
	and numAttrs
	
	*/
	
	/* Add up all of the xattr value lengths
	   Add up all of the header sizes of all of the xattr names
	   Calculate size of finder info
	*/
	
		printf("There are %i entries in the dotU file\n",(*dotU).header.numEntries);
	/* dot U file size is a multiple of 4096 bytes */
	/* Resource fork starts at (total file size - resource size) */
	for(i=0;i<(*dotU).header.numEntries;i++){
		switch((*dotU).entry[i].id){
			case 2:{
				/* Resource fork */
				(*dotU).entry[i].offset=0;
				sizeResource = (*dotU).entry[i].length;
			}break;
			case 9:{
				/* Finder info - xattrs live here */
				/* Baseline the xattr values */
				currentValueOffset = 0;
				currentNameOffset=0;
				for(j=0;j<(*dotU).entry[i].data.finder.xattrHdr.numAttrs;j++){
					(*dotU).entry[i].data.finder.attr[j].valueOffset = currentValueOffset;
					currentValueOffset+=(*dotU).entry[i].data.finder.attr[j].valueLength;
				
				/* Get the size of the xattr names */
					currentNameOffset+=attrHdrSize((*dotU).entry[i].data.finder.attr[j].nameLength);
				
				}
				/* Now we have the total length of the xattr data */
				(*dotU).entry[i].data.finder.xattrHdr.attrDataLength = currentValueOffset;
				printf("Attrs data length is %li\n",(*dotU).entry[i].data.finder.xattrHdr.attrDataLength);
				printf("Attrs header length is %li\n",currentNameOffset);
				listAttrs(*dotU);

				
					/* Names of xattrs start at byte #120 
					50 bytes of dotU header + entries
					70 bytes of Finder Info header   */
				currentValueOffset=currentNameOffset+120;
				/* Add the right offset ot all of the xattr values */
				for(j=0;j<(*dotU).entry[i].data.finder.xattrHdr.numAttrs;j++){
					(*dotU).entry[i].data.finder.attr[j].valueOffset += currentValueOffset;
				}
				sizeFinder = (*dotU).entry[i].data.finder.attr[(*dotU).entry[i].data.finder.xattrHdr.numAttrs-1].valueOffset + (*dotU).entry[i].data.finder.attr[j].valueLength;


				}break;
			default:{
				/* Unknown dotU entry */
				printf("Unknown entry id in list.\n");
			}break;
			
		}
	}

	/* 50 bytes of dotU header + entry list */
	sizeNeeded = roundup4096(sizeResource + sizeFinder + 50);
	printf("Total ._ file size will be %u + %u + 50 = %u\n",sizeResource, sizeFinder, sizeNeeded);
	

	for(i=0;i<(*dotU).header.numEntries;i++){
		switch((*dotU).entry[i].id){
			case 2:{
				/* Resource fork */
				(*dotU).entry[i].offset = sizeNeeded - sizeResource;
			}break;
			case 9:{
				/* Finder Info */
				/* Set the offset for the finder info - first entry */
				/* Finder Info is always 50 bytes from start of file */
				/* DotU header + Entry list = 50 bytes. Finder info follows. */
				(*dotU).entry[i].offset=50;
				
				/* Set the size of the finder entry 
				   Total size of file minus the resource and minus the 
				   dotU header and entry list */
				(*dotU).entry[i].length = sizeNeeded - sizeResource - 50;
			}break;
			default:{
				printf("Unknown entry id.\n");
			}break;
		}
	}
	
	return 0;
}

/* Add in a new extended attribute.  Return 0 if good, -1 if fail */
int 
addAttr(struct DotU *dotU, const char * name, const char * value){
	int index,finderEntry;
	uint32_t attrNum;
	struct ExtAttr* attrs;
	int cmpString;
	int charNum;
	
	/* TODO - make sure there's room - if not, what happens? */

	/* Check to see if it's a new attr name.  If not, 
	   just rewrite the existing value and update
	   the value length. */
	finderEntry=getFinderInfoEntry((*dotU));
	index=getAttrIndex((*dotU),name);
	
	if(index!=-1){
		/* Remove the old value */
		printf("Found attr %s\n",name);
		free((*dotU).entry[finderEntry].data.finder.attr[index].value);
		/* Add in the new one and be sure to set the length of it (length not including the \0). */
		(*dotU).entry[finderEntry].data.finder.attr[index].value=malloc(sizeof(char)*strlen(value)+1);
		strcpy((*dotU).entry[finderEntry].data.finder.attr[index].value,value);
		(*dotU).entry[finderEntry].data.finder.attr[index].valueLength=strlen(value);
		
		
	}	else {
		/* Else if it is new, 
		   - increment the number of xattrs
		   - add the xattr name,  
		   Keep xattrs sorted alphabetically! */
		printf("Creating attr %s\n",name);
		(*dotU).entry[finderEntry].data.finder.xattrHdr.numAttrs++;
		attrs=(struct ExtAttr*)malloc(sizeof(struct ExtAttr) * (*dotU).entry[finderEntry].data.finder.xattrHdr.numAttrs);
		attrNum=0;
		while(index<0 && attrNum<(*dotU).entry[finderEntry].data.finder.xattrHdr.numAttrs-1){
			cmpString = strcmp((*dotU).entry[finderEntry].data.finder.attr[attrNum].name,name);
			printf("The strcmp value is %i\n",cmpString);
			if(cmpString<0 && index<0)
			{
				/* Alphabetically before the new xattr */
				printf("Not there yet - addxattr\n");
				attrs[attrNum].name=malloc(sizeof(char)*strlen((*dotU).entry[finderEntry].data.finder.attr[attrNum].name)+1);
				strcpy(attrs[attrNum].name,(*dotU).entry[finderEntry].data.finder.attr[attrNum].name);
				attrs[attrNum].value=malloc(sizeof(char)*strlen((*dotU).entry[finderEntry].data.finder.attr[attrNum].value)+1);
				strcpy(attrs[attrNum].value,(*dotU).entry[finderEntry].data.finder.attr[attrNum].value);
				attrs[attrNum].nameLength=strlen((*dotU).entry[finderEntry].data.finder.attr[attrNum].name)+1;
				attrs[attrNum].valueLength=strlen((*dotU).entry[finderEntry].data.finder.attr[attrNum].value);
				for(charNum=0;charNum<2;charNum++) attrs[attrNum].flags[charNum]=(*dotU).entry[finderEntry].data.finder.attr[attrNum].flags[charNum];
				printf("Listing %s %s\n",attrs[attrNum].name,attrs[attrNum].value);
				
				
				attrNum++;
				
			} else {
				index = attrNum;
			}
		}
		/* Insert here */
		printf("This is the one - addxattr\n");
		attrs[attrNum].name=malloc(sizeof(char)*strlen(name)+1);
		strcpy(attrs[attrNum].name,name);
		attrs[attrNum].value=malloc(sizeof(char)*strlen(value)+1);		
		strcpy(attrs[attrNum].value,value);
		/* Entry name length includes \0, but entry value length does not. */
		attrs[attrNum].nameLength=strlen(name)+1;
		attrs[attrNum].valueLength=strlen(value);
		index=attrNum;
		printf("Adding in %s %s\n",attrs[attrNum].name,attrs[attrNum].value);
		printf("Index of attr %s is %i\n",attrs[attrNum].name,attrNum);
		/* Set flags to 0's */
		for(charNum=0;charNum<2;charNum++) attrs[attrNum].flags[charNum]=0;		
		attrNum++;
		
		/* Finish off the array */
		while(attrNum<(*dotU).entry[finderEntry].data.finder.xattrHdr.numAttrs){
			printf("Finishing up xattr list - addxattr\n");
			attrs[attrNum]=(*dotU).entry[finderEntry].data.finder.attr[attrNum-1];
			/* Keep alphabetical order after the new xattr */
			attrs[attrNum].name=malloc(sizeof(char)*strlen((*dotU).entry[finderEntry].data.finder.attr[attrNum-1].name)+1);
			strcpy(attrs[attrNum].name,(*dotU).entry[finderEntry].data.finder.attr[attrNum-1].name);
			attrs[attrNum].value=malloc(sizeof(char)*strlen((*dotU).entry[finderEntry].data.finder.attr[attrNum-1].value)+1);
			strcpy(attrs[attrNum].value,(*dotU).entry[finderEntry].data.finder.attr[attrNum-1].value);
			attrs[attrNum].nameLength=strlen((*dotU).entry[finderEntry].data.finder.attr[attrNum-1].name)+1;
			attrs[attrNum].valueLength=strlen((*dotU).entry[finderEntry].data.finder.attr[attrNum-1].value);
			/* Set flags to 0's */
			for(charNum=0;charNum<2;charNum++) attrs[attrNum].flags[charNum]=(*dotU).entry[finderEntry].data.finder.attr[attrNum-1].flags[charNum];
			printf("Listing %s %s\n",attrs[attrNum].name,attrs[attrNum].value);
				
			attrNum++;
		}
		
		/* List all values - debug */
		for(attrNum=0;attrNum<(*dotU).entry[finderEntry].data.finder.xattrHdr.numAttrs;attrNum++){
			printf("Attr %s : %s\n",attrs[attrNum].name,attrs[attrNum].value);
		}
		printf("Setting attr list\n");
		/* set attrs in dotU */
		free((*dotU).entry[finderEntry].data.finder.attr);
		printf("Freed mem\n");
		(*dotU).entry[finderEntry].data.finder.attr=attrs;	
		printf("Set attr ptr");
		printf("New attr is %s \n",(*dotU).entry[finderEntry].data.finder.attr[index].name);
					
		printf("Done Setting attr list\n");
	}
	
	
	printf("New attr %s is %s\n",(*dotU).entry[finderEntry].data.finder.attr[index].name,(*dotU).entry[finderEntry].data.finder.attr[index].value);
	
	
	listAttrs((*dotU));
	
	return 0;
}

/* Remove an extended attribute.  Return 0 if good, -1 if fail */
int 
rmAttr(struct DotU * dotU, const char * name){
	/* Find xattr */
	int index=getAttrIndex((*dotU),name);
	int finderEntry=getFinderInfoEntry((*dotU));
	uint32_t attrNum,charNum;
	struct ExtAttr* attrs;

	/* If not found, return -1 */
	if(index==-1) return -1;
	
	/* Create new struct, one xattr fewer */
	(*dotU).entry[finderEntry].data.finder.xattrHdr.numAttrs--;
	attrs=(struct ExtAttr*)malloc(sizeof(struct ExtAttr) * (*dotU).entry[finderEntry].data.finder.xattrHdr.numAttrs);
	attrNum=0;
	
	
	/* Copy everything except the one we're removing */
	for(attrNum=0;attrNum<index;attrNum++){
		/* Copy all attrs that come alphabetically before the one to be removed */
		printf("Not there yet - rmAttr\n");
		attrs[attrNum].name=malloc(sizeof(char)*strlen((*dotU).entry[finderEntry].data.finder.attr[attrNum].name)+1);
		strcpy(attrs[attrNum].name,(*dotU).entry[finderEntry].data.finder.attr[attrNum].name);
		attrs[attrNum].value=malloc(sizeof(char)*strlen((*dotU).entry[finderEntry].data.finder.attr[attrNum].value)+1);
		strcpy(attrs[attrNum].value,(*dotU).entry[finderEntry].data.finder.attr[attrNum].value);
		attrs[attrNum].nameLength=strlen((*dotU).entry[finderEntry].data.finder.attr[attrNum].name)+1;
		attrs[attrNum].valueLength=strlen((*dotU).entry[finderEntry].data.finder.attr[attrNum].value);
		for(charNum=0;charNum<2;charNum++) attrs[attrNum].flags[charNum]=(*dotU).entry[finderEntry].data.finder.attr[attrNum].flags[charNum];
		printf("Keeping %s %s\n",attrs[attrNum].name,attrs[attrNum].value);
	}
	for(attrNum=index;attrNum<(*dotU).entry[finderEntry].data.finder.xattrHdr.numAttrs;attrNum++){
		/* Copy all attrs that come alphabetically after the one to be removed */
		printf("Copying rest of xattrs - rmAttr\n");
		attrs[attrNum].name=malloc(sizeof(char)*strlen((*dotU).entry[finderEntry].data.finder.attr[attrNum].name)+1);
		strcpy(attrs[attrNum].name,(*dotU).entry[finderEntry].data.finder.attr[attrNum].name);
		attrs[attrNum].value=malloc(sizeof(char)*strlen((*dotU).entry[finderEntry].data.finder.attr[attrNum].value)+1);
		strcpy(attrs[attrNum].value,(*dotU).entry[finderEntry].data.finder.attr[attrNum].value);
		attrs[attrNum].nameLength=strlen((*dotU).entry[finderEntry].data.finder.attr[attrNum].name)+1;
		attrs[attrNum].valueLength=strlen((*dotU).entry[finderEntry].data.finder.attr[attrNum].value);
		for(charNum=0;charNum<2;charNum++) attrs[attrNum].flags[charNum]=(*dotU).entry[finderEntry].data.finder.attr[attrNum].flags[charNum];
		printf("Keeping %s %s\n",attrs[attrNum].name,attrs[attrNum].value);
	}
	
	/* Free the old memory */
	free((*dotU).entry[finderEntry].data.finder.attr);
	printf("Freed mem\n");
		
	/* Move the pointer. */
	(*dotU).entry[finderEntry].data.finder.attr=attrs;	
		printf("Set attr ptr");	
	
	return 0;
}

char* 
getAttrValue(struct DotU dotU, const char * name){
	char * value;
	int finderEntry = getFinderInfoEntry(dotU);
	int index = getAttrIndex(dotU,name);
	
	if(finderEntry >=0 && index >=0)
		return dotU.entry[finderEntry].data.finder.attr[index].value;
	
	return "";
}

/* Returns the index of the attribute in question, -1 if not found. */
int
getAttrIndex(struct DotU dotU, const char * name){
	int i;
	int finderEntry = getFinderInfoEntry(dotU);
	printf("Looking for %s\n",name);
	if(finderEntry<0){
		printf("Cannot find FinderInfo, so cannot locate xattr.\n");
		return -1;
	}
	
	for(i=0;i<dotU.entry[finderEntry].data.finder.xattrHdr.numAttrs;i++){
		printf("Comparing %s and %s\n",dotU.entry[finderEntry].data.finder.attr[i].name,name);
		printf("Strcmp returns %i\n",strcmp(dotU.entry[finderEntry].data.finder.attr[i].name,name));
		if(strcmp(dotU.entry[finderEntry].data.finder.attr[i].name,name)<0){
			printf("Not there yet.\n");
		} else if(strcmp(dotU.entry[finderEntry].data.finder.attr[i].name,name)==0){
			printf("Match!\n"); return i;  
		} else {
			printf("Too far.\n"); return -1; 
		}
		
	}	
	printf("Couldn't find %s in attr list.\n",name);
	return -1;
}

/* Returns the entry index of the finder info, -1 if not found. */
int
getFinderInfoEntry(struct DotU dotU){
	int i;
	for(i=0;i<dotU.header.numEntries;i++){
		if(dotU.entry[i].id==9) return i;
	}
	return -1;
}

void listAttrs(struct DotU dotU){
	int finderEntry = getFinderInfoEntry(dotU);
	int j;
	
	for(j=0;j<dotU.entry[finderEntry].data.finder.xattrHdr.numAttrs;j++){
		printf("\n\t\t\tAttr #%i : %s : %s",j,dotU.entry[finderEntry].data.finder.attr[j].name,dotU.entry[finderEntry].data.finder.attr[j].value);
	}
	printf("\n");
	return;
}

void 
printDotUDetail(struct DotU dotU){
	uint32_t i,j;
	
	/* Printing details of DotU Object */
	printf("\nFile Magic Number: 0x%.8x",dotU.header.magic);
	printf("\nFile Version Number: 0x%.8x",dotU.header.versionNum);
	printf("\nFile Home File System: %16s",dotU.header.homeFileSystem);
	printf("\nFile Number of entries: %i",dotU.header.numEntries);
	

	for(i=0;i<dotU.header.numEntries;i++){
		printf("\nEntry ID: %i",dotU.entry[i].id);
		printf("\n\tEntry Offset: %i", dotU.entry[i].offset);
		printf("\n\tEntry Length: %i", dotU.entry[i].length);
		
		switch(dotU.entry[i].id){
			case 2:{
				printf("\n\tResource Fork.");
				/* Shows data in the resource fork */
				printf("\n\t\tData: ");
				for(j=0;j<dotU.entry[i].length;j++)printChar(dotU.entry[i].data.resource.data[j]);
			}break;
			case 9:{
				printf("\n\tFinder Info.");
				printf("\n\tFirst 32 bytes reserved by FinderInfo (16 header, 16 extended): ");
					for(j=0;j<32;j++) printChar(dotU.entry[i].data.finder.finderHeader[j]);
				printf("\n\tPadding (16 bits): ");
					for(j=0;j<2;j++) printChar(dotU.entry[i].data.finder.padding[j]);				
				printf("\n\tExt Attr Header:");
				printf("\n\t\tAttr Header Magic: %.8x",dotU.entry[i].data.finder.xattrHdr.headerMagic);				
				printf("\n\t\tAttr Header Debug tag: %.8x",dotU.entry[i].data.finder.xattrHdr.debugTag);
				printf("\n\t\tAttr Header Total Size: %i",dotU.entry[i].data.finder.xattrHdr.size);
				printf("\n\t\tAttr Header Data Start: %i",dotU.entry[i].data.finder.xattrHdr.attrDataOffset);
				printf("\n\t\tAttr Header Data Length: %i",dotU.entry[i].data.finder.xattrHdr.attrDataLength);
				printf("\n\t\tAttr Header Reserved: ");
					for(j=0;j<12;j++) printChar(dotU.entry[i].data.finder.xattrHdr.attrReserved[j]);				
				printf("\n\t\tAttr Header Flags: ");
					for(j=0;j<2;j++) printChar(dotU.entry[i].data.finder.xattrHdr.attrFlags[j]);				
				printf("\n\t\tAttr Header Num Attrs: %i",dotU.entry[i].data.finder.xattrHdr.numAttrs);
				
				for(j=0;j<dotU.entry[i].data.finder.xattrHdr.numAttrs;j++){
					printf("\n\t\t\tAttr #%i : %s : %s",j,dotU.entry[i].data.finder.attr[j].name,dotU.entry[i].data.finder.attr[j].value);
				}
				
				
			}break;
			default: {
				printf("Error - Unknown DotU Entry Type in input file.\n");
			}break;
			
		}
	
	}

}

struct DotU iniDotU(const char * parentFileName){
	/* Create dotU struct */
	struct DotU dotU;
	
	struct stat statBuffer;
	uint32_t i;
	uint32_t entryCount,dotUOffset;
	int fileDescriptor;
	char *data;
	FILE *dotUFile;
	char *dotUBuffer;
	int readChar;
	uint32_t fileLength;
	
	struct ExtAttr *attrs;
	char *entryName;
	char *entryValue;
	uint32_t charNum;
	uint8_t entryNameLength;
	uint32_t entryValueLength;
	uint32_t entryHeaderOffset;
	uint32_t entryValueOffset;
	char attrFlags[2];
	
	
	/* Fill dotU struct */
	printf("Setting up header\n"); /* DEBUG PRINT */
	/* dotU header */
	dotU.header.magic = 0x00051607;
	dotU.header.versionNum = 0x00020000;
	strcpy(dotU.header.homeFileSystem,"Mac OS X        ");
	dotU.header.numEntries = (uint16_t) 1; /* We don't need the resource fork if it's a brand new dotU file */
	
	
	
	/* The dotU file has various entries.
	/ Extended attributes are usually in the Finder Info.*/
	printf("Setting up dotu entries\n"); /* DEBUG PRINT */

	/* Entry 0 will be the Finder Info (where the xattrs will go) */
	dotU.entry[0].id=9;
	dotU.entry[0].offset=50;
	dotU.entry[0].length=4046;
	/* The finder header is all 0's to start. Data in the header includes
	   file type, file creator, some flag bits, and some stuff about the 
	   Finder's GUI */
	printf("Setting up Finder Info header.\n");
	for(i=0;i<32;i++) dotU.entry[0].data.finder.finderHeader[i]='\0';
	for(i=0;i<2;i++)  dotU.entry[0].data.finder.padding[i]     ='\0';      
	
	printf("Setting up Extended Finder Info.\n");   
	dotU.entry[0].data.finder.xattrHdr.headerMagic = 0x41545452;
	/* TODO: Get the file id */
	/* dotU.entry[entryCount].data.finder.xattrHdr.debugTag          = (uint32_t) toBigEndian(&dotUBuffer[dotU.entry[entryCount].offset+38],4); */
	printf("1");
	dotU.entry[0].data.finder.xattrHdr.size              = (uint32_t) 4046;
	printf("1");
	dotU.entry[0].data.finder.xattrHdr.attrDataOffset    = (uint32_t) 120;
	printf("1");
	dotU.entry[0].data.finder.xattrHdr.attrDataLength    = (uint32_t) 0;
	printf("1");
	for(i=0;i<12;i++) dotU.entry[0].data.finder.xattrHdr.attrReserved[i] = 0;
	printf("1");
	for(i=0;i<2;i++)  dotU.entry[0].data.finder.xattrHdr.attrFlags[i]    = 0;
	printf("1");
	dotU.entry[0].data.finder.xattrHdr.numAttrs          = 0;
	
	/* Set up a blank resource fork */
	/*printf("Setting up resource fork\n"); /* DEBUG PRINT */
	/*dotU.entry[1].id = 2;
	dotU.entry[1].offset = 3810;
	dotU.entry[1].length = 286;
	dotU.entry[1].data.resource.data=(char*)malloc(dotU.entry[1].length);
	dotU.entry[entryCount].data.resource.data= TODO ;
	*/

	return dotU;
	/*
	
	for(j=0;j<4;j++) printChar(dotU.header.magic[j]);
	printf("\nFile Magic Number: 0x%.8x",dotU.header.magic);
	
	*/
}