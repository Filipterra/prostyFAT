/*
 ============================================================================
 Name        : filesystem.c
 Author      : Filip Przybysz
 Version     :
 Copyright   : 
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include "drive.h"
#include "struct.h"

void printCommands()
{
	printf("Commands:\n"); //TODO:command list
}

int main(int argc, char **argv) {

	char command[MAX_PATH_LENGTH+20];
	int h=1;
	unsigned long long lR=0;
	int nR=0;

	FILE* drive;
	drive = fopen(argv[1], "rb+");

	if (drive==NULL) {
		printf("Such drive does not exist. Do you wish to create one? [y/n]\n");

		while(h)
		{
		scanf("%s", command);
		if (command[0]=='n' || command[0]=='N') return EXIT_SUCCESS;
		else if (command[0]=='y' || command[0]=='Y') {
			printf("Please input desired drive size in bytes.\n");
			nR=scanf("%llu", &lR);
			if (nR!=1) {
				printf("Incorrect parameter.");
				continue;
			}
			drive = createDrive(argv[1], lR);
			h=0;
		}
		else printf("[y/n]?\n");
		}
	}

	fseek(drive, 0, SEEK_SET);
	struct driveInfo dinfo;
	fread(&dinfo, sizeof(struct driveInfo), 1, drive);

	struct folderInfo folinfo[FOLDER_NUMBER];
	struct fileInfo finfo[dinfo.sectors];
	struct sectorInfo sinfo[dinfo.sectors];
	fread(&folinfo, sizeof(struct folderInfo), FOLDER_NUMBER, drive);
	fread(&finfo, sizeof(struct fileInfo), dinfo.sectors, drive);
	fread(&sinfo, sizeof(struct sectorInfo), dinfo.sectors, drive);

	struct allInfo ainfo = {&dinfo, folinfo, finfo, sinfo, sizeof(dinfo)+sizeof(folinfo)+sizeof(finfo)+sizeof(sinfo), 0};

	printCommands();

	while(1)
	{
		scanf("%s", command);

		if (!strcmp(command,"exit"))
		{
			fclose(drive);
			return EXIT_SUCCESS;
		}
		if (!strcmp(command,"help"))
		{
			printCommands();
		}
		if (!strcmp(command,"vcopym"))
		{
			scanf("%s", command);
			vCopym(&ainfo,drive,command);
		}
		if (!strcmp(command,"mcopyv"))
		{
			scanf("%s", command);
			mCopyv(&ainfo,drive,command);
		}
		if (!strcmp(command,"lf"))
		{
			listFiles(&ainfo);
		}
		if (!strcmp(command,"le"))
		{
			listElements(&ainfo);
		}
		if (!strcmp(command,"ls"))
		{
			for (long i=0; i<ainfo.dinf->sectors; ++i) printf("%ld %ld\n", i, ainfo.sinf[i].next);
		}
		if (!strcmp(command,"lfs"))
		{
			for (long i=0; i<ainfo.dinf->sectors; ++i) printf("%ld %ld\n", i, finfo[i].sectorAdr);
		}
		if (!strcmp(command,"lfol"))
		{
			listFolders(&ainfo);
		}
		if (!strcmp(command,"mkdir"))
		{
			scanf("%s", command);
			makeDir(&ainfo,drive,command);
		}
		if (!strcmp(command,"cd"))
		{
			scanf("%s", command);
			changeDir(&ainfo,drive,command);
		}
		if (!strcmp(command,"driveinfo"))
		{
			printf("%llu kb\n", dinfo.size/1000);
		}
		if (!strcmp(command,"folderinfo"))
		{
			showFolderInfo(&ainfo);
		}
		if (!strcmp(command,"del"))
		{
			scanf("%s", command);
			unlink(&ainfo,drive,command, ainfo.curFolNum);
			updateDrive(&ainfo, drive);
		}
		if (!strcmp(command,"addsize"))
		{
			nR=scanf("%s %llu", command, &lR);
			if (nR!=2) {
				printf("Incorrect parameters.");
				continue;
			}
			addSize(&ainfo,drive,command,lR);
		}
		if (!strcmp(command,"subsize"))
		{
			nR=scanf("%s %llu", command, &lR);
			if (nR!=2) {
				printf("Incorrect parameters.");
				continue;
			}
			substractSize(&ainfo,drive,command,lR);
		}
	}


	fclose(drive);
	return EXIT_SUCCESS;
}
