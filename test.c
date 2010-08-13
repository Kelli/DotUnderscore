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
#include <libgen.h>





int main(int argc, char *argv[]){
	struct DotU myDotU;
	int i,j,testFileNum;
	long ok=0;
	long nok=0;
	char testCommand[MAXCOMMANDSIZE];
	char dirName[MAXDIRNAMESIZE];
	char fileName[MAXFILENAMESIZE];
	char testFilePrefix[MAXFILENAMESIZE];
	char testFileName[MAXFILENAMESIZE];	
	
	if(argc!=2){
		printf("Usage: %s filename\n",argv[0]);
		return -1;
	}
	
	/* Start program */
	printf("Welcome.\n");


	/* Initializing DotU struct with data from file*/
	myDotU = readDotUFile(argv[1]);
	printDotUDetail(myDotU);
	strcpy(dirName,dirname(argv[1]));
	strcpy(fileName,basename(argv[1]));
	printf("\n\nFile is %s/%s\n",dirName,fileName);	
	
	testFileNum=0;
	
	/* Test the create file method */
	testFileNum++;
	snprintf(testFileName,MAXFILENAMESIZE,"%s/t%i-%s",dirName,testFileNum,fileName);
	if(createDotUFileSpecName(myDotU,argv[1],testFileName)!=0){
		printf("NOK - Error creating DotU File from struct.\n");
		nok++;
	} else {
		printf("OK - Created DotU File from struct.\n");
		ok++;
	}
	/* Should match original file */
	snprintf(testCommand, MAXCOMMANDSIZE, "cmp -bl %s %s",argv[1],testFileName);
	if(system(testCommand)!=0){
		printf("NOK - File recreated from struct does not match original file.\n");
		nok++;
	} else {
		printf("OK - File recreated from struct matches original file.\n");
		ok++;
	}
	
	
	/* Test the calculations of the offsets */
	if(setOffsets(&myDotU)!=0){
		printf("NOK - Error calculating offsets for dotU struct!\n");
		nok++;
	} else {
		printf("OK - Done calculating offsets for dotU struct!\n");
		ok++;
	}
	/* Create file to compare to previous - should match. */
	testFileNum++;
	snprintf(testFileName,MAXFILENAMESIZE,"%s/t%i-%s",dirName,testFileNum,fileName);
	if(createDotUFileSpecName(myDotU,argv[1],testFileName)!=0){
		printf("NOK - Error creating DotU File from struct.\n");
		nok++;
	} else {
		printf("OK - Created DotU File from struct.\n");
		ok++;
	}
	snprintf(testCommand, MAXCOMMANDSIZE, "cmp -bl %s %s",argv[1],testFileName);
	if(system(testCommand)!=0){
		printf("NOK - File recreated from struct does not match original file.\n");
		nok++;
	} else {
		printf("OK - File recreated from struct matches original file.\n");
		ok++;
	}
	
	
	/* Test adding an xattr */
	if(addAttr(&myDotU,"test1","value1")!=0){
		printf("NOK - Error adding new xattr.\n");
		nok++;
	} else {
		printf("OK - Added new xattr.\n");
		ok++;
	}
	/* Create file to compare to previous - should match except for new xattr. */
	testFileNum++;
	snprintf(testFileName,MAXFILENAMESIZE,"%s/t%i-%s",dirName,testFileNum,fileName);
	if(createDotUFileSpecName(myDotU,argv[1],testFileName)!=0){
		printf("NOK - Error creating DotU File from struct.\n");
		nok++;
	} else {
		printf("OK - Created DotU File from struct.\n");
		ok++;
	}
	/* TODO - add diff but take into account expected difference */

	
	/* Test adding an xattr value to an xattr that already exists */
	if(addAttr(&myDotU,"test1","value1-B")!=0){
		printf("NOK - Error modifying xattr value via addAttr method.\n");
		nok++;
	} else {
		printf("OK - Modified existing xattr's value via addAttr method.\n");
		ok++;
	}
	/*printf("Index of new attr is: %i\n",getAttrIndex(myDotU,"test1"));
	 printf("Index of finder entry is: %i\n",getFinderInfoEntry(myDotU)); */
	/* Create file to compare to previous - should match except for new value for xattr. */
	testFileNum++;
	snprintf(testFileName,MAXFILENAMESIZE,"%s/t%i-%s",dirName,testFileNum,fileName);
	if(createDotUFileSpecName(myDotU,argv[1],testFileName)!=0){
		printf("NOK - Error creating DotU File from struct.\n");
		nok++;
	} else {
		printf("OK - Created DotU File from struct.\n");
		ok++;
	}
	/* TODO - add diff but take into account expected difference */

	
	/* Test removing an xattr value that doesn't exist */
	if(rmAttr(&myDotU,"test-does-not-exist")!=0){
		printf("OK - Couldn't remove non-existent xattr.\n");
		ok++;
	} else {
		printf("NOK - Removed non-existent xattr?\n");
		nok++;
	}
	/* Create file to compare to previous - should match. */
	testFileNum++;
	snprintf(testFileName,MAXFILENAMESIZE,"%s/t%i-%s",dirName,testFileNum,fileName);
	if(createDotUFileSpecName(myDotU,argv[1],testFileName)!=0){
		printf("NOK - Error creating DotU File from struct.\n");
		nok++;
	} else {
		printf("OK - Created DotU File from struct.\n");
		ok++;
	}
	snprintf(testCommand, MAXCOMMANDSIZE, "cmp -bl %s/t%i-%s %s",dirName,testFileNum-1,fileName,testFileName);
	if(system(testCommand)!=0){
		printf("NOK - File doesn't match after trying to remove non-existent xattr.\n");
		nok++;
	} else {
		printf("OK - File matches after trying to remove non-existent xattr.\n");
		ok++;
	}
	

	/* Test removing an xattr that does exist */
	if(rmAttr(&myDotU,"test1")!=0){
		printf("NOK - Error removing existent xattr.\n");
		nok++;
	} else {
		printf("OK - Removed existing xattr.\n");
		ok++;
	}
	/* Create file to compare to previous - should match original file */
	testFileNum++;
	snprintf(testFileName,MAXFILENAMESIZE,"%s/t%i-%s",dirName,testFileNum,fileName);
	if(createDotUFileSpecName(myDotU,argv[1],testFileName)!=0){
		printf("NOK - Error creating DotU File from struct.\n");
		nok++;
	} else {
		printf("OK - Created DotU File from struct.\n");
		ok++;
	}
	snprintf(testCommand, MAXCOMMANDSIZE, "cmp -bl %s %s",argv[1],testFileName);
	if(system(testCommand)!=0){
		printf("NOK - File doesn't match after removing xattr we know exists.\n");
		nok++;
	} else {
		printf("OK - File matches after removing known xattr.\n");
		ok++;
	}
	
	/* Create brand-new dotu struct */
	myDotU = iniDotU(argv[1]);
	/* Create dotU file - should have 0 xattrs */
	testFileNum++;
	snprintf(testFileName,MAXFILENAMESIZE,"%s/t%i-%s",dirName,testFileNum,fileName);
	if(createDotUFileSpecName(myDotU,argv[1],testFileName)!=0){
		printf("NOK - Error creating DotU File from struct.\n");
		nok++;
	} else {
		printf("OK - Created DotU File from struct.\n");
		ok++;
	}	
	
	/* Add an xattr to the blank dotu struct */
	if(addAttr(&myDotU,"test1","value1")!=0){
		printf("NOK - Error adding new xattr to blank struct.\n");
		nok++;
	} else {
		printf("OK - Added new xattr to blank struct.\n");
		ok++;
	}
	/* Create file to compare to previous - should match except for new xattr. */
	testFileNum++;
	snprintf(testFileName,MAXFILENAMESIZE,"%s/t%i-%s",dirName,testFileNum,fileName);
	if(createDotUFileSpecName(myDotU,argv[1],testFileName)!=0){
		printf("NOK - Error creating DotU File file from struct.\n");
		nok++;
	} else {
		printf("OK - Created DotU File from struct.\n");
		ok++;
	}
	/* TODO - add diff but take into account expected difference */
	


	/* Print summary of tests */
	if(nok==0) printf("All %u tests OK!\n",ok);
	else {
		printf("%u of %u tests failed.\n",nok,nok+ok);
	}
	
	
	
	

	printf("\nGoodbye.\n");
	return 0;
}