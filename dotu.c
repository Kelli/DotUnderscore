/* This file reads the dot-underscore files that are formed
	 when copying a file from an OSX machine onto a *nix.
 
	 Author: Kelli Ireland
	 Summer 2010
	 As part of the Google Summer of Code
*/

#include "dotu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>





int main(int argc, char *argv[]){
	struct DotU myDotU;
	int i,j;
	
	/* Start program */
	printf("Welcome.\n");

	/* Initializing DotU struct with data from file*/
	myDotU = readDotUFile("test/dotu-f1");


	
	
	/* Printing details of DotU Object */
	printf("\nFile Magic Number: 0x%.8x",myDotU.header.magicNum);
	printf("\nFile Version Number: 0x%.8x",myDotU.header.versionNum);
	printf("\nFile Home File System: %16s",myDotU.header.homeFileSystem);
	printf("\nFile Number of entries: %i",myDotU.header.numEntries);
	

	for(i=0;i<myDotU.header.numEntries;i++){
		printf("\nEntry ID: %i",myDotU.entry[i].id);
		printf("\n\tEntry Offset: %i", myDotU.entry[i].offset);
		printf("\n\tEntry Length: %i", myDotU.entry[i].length);
		
		switch(myDotU.entry[i].id){
			case 2:{
				printf("\n\tResource Fork.");
				/* Shows data in the resource fork */
				printf("\n\t\tData: ");
				for(j=0;j<myDotU.entry[i].length;j++)printChar(myDotU.entry[i].data.resource.data[j]);
			}break;
			case 9:{
				printf("\n\tFinder Info.");
				printf("\n\tFirst 32 bytes reserved by FinderInfo (16 header, 16 extended): ");
					for(j=0;j<32;j++) printChar(myDotU.entry[i].data.finder.finderHeader[j]);
				printf("\n\tPadding (16 bits): ");
					for(j=0;j<2;j++) printChar(myDotU.entry[i].data.finder.padding[j]);				
				printf("\n\tExt Attr Header:");
				printf("\n\t\tAttr Header Magic: %.8x",myDotU.entry[i].data.finder.xattrHdr.headerMagicNum);
				printf("\n\t\tAttr Header Debug tag: %.8x",myDotU.entry[i].data.finder.xattrHdr.debugTag);
				printf("\n\t\tAttr Header Total Size: %i",myDotU.entry[i].data.finder.xattrHdr.size);
				printf("\n\t\tAttr Header Data Start: %i",myDotU.entry[i].data.finder.xattrHdr.attrDataOffset);
				printf("\n\t\tAttr Header Data Length: %i",myDotU.entry[i].data.finder.xattrHdr.attrDataLength);
				printf("\n\t\tAttr Header Reserved: ");
					for(j=0;j<12;j++) printChar(myDotU.entry[i].data.finder.xattrHdr.attrReserved[j]);				
				printf("\n\t\tAttr Header Flags: ");
					for(j=0;j<2;j++) printChar(myDotU.entry[i].data.finder.xattrHdr.attrFlags[j]);				
				printf("\n\t\tAttr Header Num Attrs: %i",myDotU.entry[i].data.finder.xattrHdr.numAttrs);
				
				for(j=0;j<myDotU.entry[i].data.finder.xattrHdr.numAttrs;j++){
					printf("\n\t\t\tAttr #%i : %s : %s",j,myDotU.entry[i].data.finder.attr[j].name,myDotU.entry[i].data.finder.attr[j].data);
				}
				
				
			}break;
			default: {
				printf("\nUnknown Entry Type.\n");
			}break;
			
		}
	
	}


	printf("\n\nTesting create-a-dotU:\n");
	if(createDotUFile!=0){
		printf("\nError creating ._ file!\n");
	} else {
		printf("Done!");
	}
	

	printf("\nGoodbye.\n");
	return 0;
}