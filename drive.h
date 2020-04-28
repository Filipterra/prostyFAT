/*
 * drive.h
 *
 *  Created on: May 27, 2019
 *      Author: filip
 */

#ifndef DRIVE_H_
#define DRIVE_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "struct.h"

//TODO:remove FILE* where not needed

void updateDrive (struct allInfo *ainf, FILE* drive) {
	fseek(drive, 0, SEEK_SET);
	fwrite(ainf->dinf, sizeof(struct driveInfo), 1, drive);
	fwrite(ainf->folinf, sizeof(struct folderInfo), FOLDER_NUMBER, drive);
	fwrite(ainf->finf, sizeof(struct fileInfo), ainf->dinf->sectors, drive);
	fwrite(ainf->sinf, sizeof(struct sectorInfo), ainf->dinf->sectors, drive);
}

FILE* createDrive (char* name, unsigned long long sizeb) {
	FILE* drive = fopen(name, "wb+");

	struct driveInfo dsec;
	strcpy(dsec.name, name);
	dsec.size=sizeb;
	dsec.folderNum=1;
	dsec.fileNum=0;
	if (sizeb%SECTOR_SIZE==0) dsec.freeSectors=sizeb/SECTOR_SIZE;
	else dsec.freeSectors=(sizeb/SECTOR_SIZE)+1;
	dsec.sectors=dsec.freeSectors;
	fwrite(&dsec, sizeof(struct driveInfo), 1, drive);

	long secNum=dsec.freeSectors;
	struct folderInfo folsec[FOLDER_NUMBER];
	struct fileInfo fsec[secNum];
	struct sectorInfo ssec[secNum];

	for (long i=0; i<secNum; ++i)
	{
		strcpy(fsec[i].name, "");
		fsec[i].size=0;
		fsec[i].sectorAdr=-1;
		fsec[i].links=0;
		ssec[i].size=SECTOR_SIZE;
		ssec[i].used=0;
		ssec[i].next=-2;
	}
	ssec[secNum-1].size=sizeb-((secNum-1)*SECTOR_SIZE);

	for (int i=0; i<FOLDER_NUMBER; ++i)	{
		strcpy(folsec[i].name, "");
		folsec[i].parent=-1;
		folsec[i].elements=0;
		for (int j=0; j<FOLDER_ELEMENTS_NUMBER; ++j)	{
			folsec[i].type[j]=FREE;
			folsec[i].address[j]=-1;
		}
		folsec[i].links=0;
	}
	strcpy(folsec[0].name, "root");
	folsec[0].parent=0;

	fwrite(&folsec, sizeof(struct folderInfo), FOLDER_NUMBER, drive);
	fwrite(&fsec, sizeof(struct fileInfo), secNum, drive);
	fwrite(&ssec, sizeof(struct sectorInfo), secNum, drive);

	char secch[SECTOR_SIZE];
	for (long i=0; i<secNum-1; ++i) fwrite(secch, SECTOR_SIZE, 1, drive);
	char lsecch[ssec[secNum-1].size];
	fwrite(lsecch, ssec[secNum-1].size, 1, drive);

	return drive;
}

int addToFolder (struct allInfo *ainf, FILE* drive, long folnum, char type, long address)	{

	if (ainf->folinf[folnum].elements==FOLDER_ELEMENTS_NUMBER)	{
		printf("Reached maximum number of elements in this folder.");
		return -1;
	}

	for (long i=0; i<FOLDER_ELEMENTS_NUMBER; ++i) {
		if (ainf->folinf[folnum].type[i]==FREE){
			ainf->folinf[folnum].type[i]=type;
			ainf->folinf[folnum].address[i]=address;
			break;
		}
	}
	ainf->folinf[folnum].elements++;
	//updateDrive(ainf, drive);
	return 0;
}

long findInFolder (const struct allInfo *ainf, const FILE* drive, long folnum, char type, long address)	{
	long en=ainf->folinf[folnum].elements;
	long i=0;
	while (en)
	{
		if (ainf->folinf[folnum].type[i]!=FREE)	{
			if (ainf->folinf[folnum].type[i]==type && ainf->folinf[folnum].address[i]==address)	{
			return i;
			}
			--en;
		}
	++i;
	}
	return -1;
}

long findInFolderByName (const struct allInfo *ainf, const FILE* drive, long folnum, const char* name)	{
	long en=ainf->folinf[folnum].elements;
	long i=0;
	while (en)
	{
		if (ainf->folinf[folnum].type[i]!=FREE)	{
			if (ainf->folinf[folnum].type[i]==ELEM && !strcmp(ainf->finf[ainf->folinf[folnum].address[i]].name, name))	{
			return i;
			}
			if (ainf->folinf[folnum].type[i]==FOLDER && !strcmp(ainf->folinf[ainf->folinf[folnum].address[i]].name, name))	{
			return i;
			}
			--en;
		}
	++i;
	}
	return -1;
}

void removeFromFolder (struct allInfo *ainf, FILE* drive, long folnum, char type, long address)	{

	long i=findInFolder (ainf, drive, folnum, type, address);
	if (i>=0)
	{
				ainf->folinf[folnum].type[i]=FREE;
				ainf->folinf[folnum].address[i]=-1;
				ainf->folinf[folnum].elements--;
	}
}

void mCopyv (struct allInfo *ainf, FILE* drive, char* fname) {
	//open file
	FILE* fcopy = fopen(fname, "rb");
	if (fcopy==NULL) {
		printf("No such file.\n");
		return;
	}

	//check file size
	fseek(fcopy, 0, SEEK_END);
	unsigned long long fsize=ftell(fcopy);
	if (ainf->dinf->size<fsize)
	{
		printf("%llu\n", fsize);
		printf("Not enough space.\n");
		fclose(fcopy);
		return;
	}

	long fnum=0;
	while(ainf->finf[fnum].sectorAdr != -1) ++fnum;
	long snum=0;
	while(ainf->sinf[snum].next != -2) ++snum;

	//add to folder info
	int h=addToFolder(ainf, drive, ainf->curFolNum, ELEM, fnum);
	if (h==-1)	{
		fclose(fcopy);
		return;
	}

	//fill file info
	strcpy(ainf->finf[fnum].name,fname);
	ainf->finf[fnum].sectorAdr=snum;
	ainf->finf[fnum].size=fsize;
	ainf->dinf->size-=ainf->sinf[snum].size;
	ainf->dinf->freeSectors--;
	ainf->dinf->fileNum++;

	//read data and save to drive
	fseek(fcopy, 0, SEEK_SET);
	char buff[SECTOR_SIZE];
	int read;
	int nexts;
	while (fsize>0) {
		read=0;
		for (;read<ainf->sinf[snum].size && (fread(buff+read, 1, 1, fcopy))!=0; ++read);
		fsize-=read;
		ainf->sinf[snum].used=read;

		fseek(drive, ainf->headerSize+(snum*SECTOR_SIZE), SEEK_SET);
		fwrite(buff, SECTOR_SIZE, 1, drive);

		if(fsize<=0) {
			ainf->sinf[snum].next=-1;
			break;
		}
		nexts=snum+1;
		while(ainf->sinf[nexts].next != -2) ++nexts;
		ainf->sinf[snum].next=nexts;
		snum=nexts;
		ainf->dinf->size-=ainf->sinf[snum].size;
		ainf->dinf->freeSectors--;
	}
	fclose(fcopy);

	updateDrive(ainf, drive);
}

long findFileByName (const struct allInfo *ainf, const FILE* drive, const char* fname, const long startfol) {
	long fnum=0;
	long folnum=startfol;
		char folname[NAME_SIZE];
		char ch[2];
		strcpy(folname, "");
		strcpy(ch, "");
		long foltabnum=0;
		for (int i=0; i<strlen(fname); ++i)	{

			if (fname[i]=='/')	{
				if (!strcmp(folname, "root")) folnum=0;
				else if(!strcmp(folname, "..")) folnum=ainf->folinf[folnum].parent;
				else	{
					foltabnum=findInFolderByName(ainf, drive, folnum, folname);
					if (foltabnum<0 || ainf->folinf[folnum].type[foltabnum]!=FOLDER)	{
						printf("Incorrect path.");
						return -1;
					}
					folnum=ainf->folinf[folnum].address[foltabnum];
				}
				strcpy(folname, "");
			}
			else {
				ch[0]=fname[i];
				strcat(folname, ch);
			}
		}
		fnum=findInFolderByName (ainf, drive, folnum, folname);
	if (fnum==-1 || ainf->folinf[folnum].type[fnum]!=ELEM)
	{
		//printf("Such file does not exist in this directory.\n");
		return -1;
	}
	fnum=ainf->folinf[folnum].address[fnum];
	if (ainf->finf[fnum].links==-1) fnum=ainf->finf[fnum].sectorAdr;
	return fnum;
}

long findFolderByName (const struct allInfo *ainf, const FILE* drive, const char* fname, const long startfol) {
	long fnum=0;
	long folnum=startfol;
		char folname[NAME_SIZE];
		char ch[2];
		strcpy(folname, "");
		strcpy(ch, "");
		long foltabnum=0;
		for (int i=0; i<strlen(fname); ++i)	{

			if (fname[i]=='/')	{
				if (!strcmp(folname, "root")) folnum=0;
				else if(!strcmp(folname, "..")) folnum=ainf->folinf[folnum].parent;
				else	{
					foltabnum=findInFolderByName(ainf, drive, folnum, folname);
					if (foltabnum<0 || ainf->folinf[folnum].type[foltabnum]!=FOLDER)	{
						printf("Incorrect path.");
						return -1;
					}
					folnum=ainf->folinf[folnum].address[foltabnum];
				}
				strcpy(folname, "");
			}
			else {
				ch[0]=fname[i];
				strcat(folname, ch);
			}
		}
		fnum=findInFolderByName (ainf, drive, folnum, folname);
	if (fnum==-1 || ainf->folinf[folnum].type[fnum]!=FOLDER)
	{
		//printf("Such folder does not exist in this directory.\n");
		return -1;
	}
	fnum=ainf->folinf[folnum].address[fnum];
	if (ainf->folinf[fnum].links==-1) fnum=ainf->folinf[fnum].parent;
	return fnum;
}

void vCopym (struct allInfo *ainf, FILE* drive, char* fname) {

	long fnum=findFileByName (ainf, drive, fname, ainf->curFolNum);
	if (fnum==-1) {
		printf("Such file does not exist in this directory.\n");
		return;
	}

	FILE* fcopy = fopen(ainf->finf[fnum].name, "wb");
	if (fcopy==NULL) {
		printf("Error copying the file.\n");
		return;
	}

	long curs=ainf->finf[fnum].sectorAdr;
	char buff[SECTOR_SIZE];
	while(1) {
		fseek(drive, ainf->headerSize+(curs*SECTOR_SIZE), SEEK_SET);
		for(int i=0; i<ainf->sinf[curs].used; ++i) fread(buff+i, 1, 1, drive);
		fwrite(buff, ainf->sinf[curs].used, 1, fcopy);

		curs=ainf->sinf[curs].next;
		if(curs<0) break;
	}
	fclose(fcopy);
}

void delFile (struct allInfo *ainf, FILE* drive, long fnum, long folnum) {

	long curs=ainf->finf[fnum].sectorAdr;
	long nexts=ainf->sinf[curs].next;
	while(1) {
		ainf->dinf->freeSectors++;
		ainf->dinf->size+=ainf->sinf[curs].size;
		ainf->sinf[curs].used=0;
		nexts=ainf->sinf[curs].next;
		ainf->sinf[curs].next=-2;

		if(nexts<0) break;
		curs=nexts;
		}

	//del from folder info
	removeFromFolder(ainf, drive, folnum, ELEM, fnum);

	strcpy(ainf->finf[fnum].name, "");
	ainf->finf[fnum].sectorAdr=-1;
	ainf->finf[fnum].size=0;
	ainf->dinf->fileNum--;

	//updateDrive(ainf, drive);
}

int addDir (struct allInfo *ainf, FILE* drive, long folnum, char* fname)	{
	if (ainf->folinf[folnum].elements==FOLDER_ELEMENTS_NUMBER)	{
		printf("Reached maximum number of elements in this folder.");
		return -1;
	}
	if (ainf->dinf->folderNum==FOLDER_NUMBER)	{
		printf("Reached maximum number of folders.");
		return -1;
	}

	long i=0;
	while (ainf->folinf[i].parent!=-1) ++i;
	ainf->folinf[i].parent=folnum;
	strcpy(ainf->folinf[i].name, fname);
	addToFolder(ainf, drive, folnum, FOLDER, i);
	ainf->dinf->folderNum++;

	return i;
}

void makeDir (struct allInfo *ainf, FILE* drive, char* fname)	{
	long folnum=ainf->curFolNum;
	char folname[NAME_SIZE];
	char ch[2];
	strcpy(folname, "");
	strcpy(ch, "");
	for (int i=0; i<strlen(fname); ++i)	{
		if (fname[i]=='/')	{
			folnum=addDir(ainf, drive, folnum, folname);
			if (folnum<0) {
				updateDrive(ainf, drive);
				return;
			}
			strcpy(folname, "");
		}
		else {
			ch[0]=fname[i];
			strcat(folname, ch);
		}
	}
	addDir(ainf, drive, folnum, folname);

	updateDrive(ainf, drive);
}

void changeDir (struct allInfo *ainf, FILE* drive, char* fname) {
	long fnum=0;
	long folnum=ainf->curFolNum;
		char folname[NAME_SIZE];
		char ch[2];
		strcpy(folname, "");
		strcpy(ch, "");
		long foltabnum=0;
		for (int i=0; i<strlen(fname); ++i)	{
			if (fname[i]=='/')	{
				if (!strcmp(folname, "root")) folnum=0;
				else if(!strcmp(folname, "..")) folnum=ainf->folinf[folnum].parent;
				else	{
					foltabnum=findInFolderByName(ainf, drive, folnum, folname);
					if (foltabnum<0 || ainf->folinf[folnum].type[foltabnum]!=FOLDER)	{
						printf("Incorrect path.");
						return;
					}
					folnum=ainf->folinf[folnum].address[foltabnum];
				}
				strcpy(folname, "");
			}
			else {
				ch[0]=fname[i];
				strcat(folname, ch);
			}
		}
		if (!strcmp(folname, "root")) {
			ainf->curFolNum=0;
			return;
		}
		else if(!strcmp(folname, "..")) {
			ainf->curFolNum=ainf->folinf[folnum].parent;
			return;
		}
		else	fnum=findInFolderByName (ainf, drive, folnum, folname);

	if (fnum==-1 || ainf->folinf[folnum].type[fnum]!=FOLDER)
	{
		printf("Such folder does not exist in this directory.\n");
		return;
	}
	fnum=ainf->folinf[folnum].address[fnum];
	if (ainf->folinf[fnum].links==-1) fnum=ainf->folinf[fnum].parent;
	ainf->curFolNum=fnum;
}

void addSize (struct allInfo *ainf, FILE* drive, char* fname, unsigned long long plusSize) {
	//check parameters
	if (plusSize<=0)	{
		printf("Incorrect parameter.");
		return;
	}
	long fnum=findFileByName (ainf, drive, fname, ainf->curFolNum);
	if (fnum==-1) {
		printf("Such file does not exist in this directory.\n");
		return;
	}

	//find last sector of given file
	int curs=ainf->finf[fnum].sectorAdr;
	while (ainf->sinf[curs].next>=0) curs=ainf->sinf[curs].next;

	//check size left on it and if not enough - left on drive minus left on that sector
	if (plusSize>ainf->sinf[curs].size-ainf->sinf[curs].used && plusSize-(ainf->sinf[curs].size-ainf->sinf[curs].used)>ainf->dinf->size)	{
		printf("Not enough space.");
		return;
	}

	//reserve more space in the sector and more sectors if necessary (update dinfo)
	if (plusSize<=ainf->sinf[curs].size-ainf->sinf[curs].used)	ainf->sinf[curs].used+=plusSize;
	else	{
	long nexts=0;
		while (1)	{
			if (plusSize<=ainf->sinf[curs].size-ainf->sinf[curs].used)	{
				ainf->finf[fnum].size+=plusSize;
				ainf->sinf[curs].used+=plusSize;
				ainf->sinf[curs].next=-1;
				break;
			}

			while(ainf->sinf[nexts].next != -2) ++nexts;
			plusSize-=ainf->sinf[curs].size-ainf->sinf[curs].used;
			ainf->finf[fnum].size+=ainf->sinf[curs].size-ainf->sinf[curs].used;
			ainf->sinf[curs].used=ainf->sinf[curs].size;
			ainf->sinf[curs].next=nexts;
			curs=nexts;
			++nexts;
			ainf->dinf->freeSectors--;
			ainf->dinf->size-=ainf->sinf[curs].size;
		}

	}
	updateDrive(ainf, drive);
}


void substractSize (struct allInfo *ainf, FILE* drive, char* fname, unsigned long long minusSize) {
	if (minusSize<=0)	{
		printf("Incorrect parameter.");
		return;
	}
	long fnum=findFileByName (ainf, drive, fname, ainf->curFolNum);
	if (fnum==-1) {
		printf("Such file does not exist in this directory.\n");
		return;
	}

	if (ainf->finf[fnum].size<=minusSize)	{
		printf("File too small.");
		return;
	}

	//find last sector of the file and substract from used as much as possible
	while (1)	{
		int curs=ainf->finf[fnum].sectorAdr;
		int lasts=curs;
		while (ainf->sinf[curs].next>=0) {lasts=curs; curs=ainf->sinf[curs].next;}

		if (minusSize<ainf->sinf[curs].used)	{
			ainf->finf[fnum].size-=minusSize;
			ainf->sinf[curs].used-=minusSize;
			ainf->sinf[curs].next=-1;
			break;
		}

		minusSize-=ainf->sinf[curs].used;
		ainf->finf[fnum].size-=ainf->sinf[curs].used;
		ainf->sinf[curs].used=0;
		ainf->sinf[curs].next=-2;
		ainf->sinf[lasts].next=-1;
		ainf->dinf->freeSectors++;
		ainf->dinf->size+=ainf->sinf[curs].size;
	}
	updateDrive(ainf, drive);
}

//TODO: create link
void createLink (struct allInfo *ainf, FILE* drive, char* fname, char* lname, long folnum) {
	//flinks=-1, fsecadr= numer pliku, size = 0, name = lname
	long fnum=0;
		if ((fnum=findFileByName (ainf, drive, fname, folnum))>=0) {

		}
		else if ((fnum=findFolderByName (ainf, drive, fname, folnum))>0)	{

		}
		else	{
			printf("Incorrect path.\n");
			return;
		}
	updateDrive(ainf, drive);
}


void delFolder (struct allInfo *ainf, FILE* drive, long folnum);
//TODO: unlink
void unlink (struct allInfo *ainf, FILE* drive, char* fname, long folnum) {//TODO: enable recursion with finding
	//find element in current folder or by given path (return if original root folder path)
	long fnum=0;
	if ((fnum=findFileByName (ainf, drive, fname, folnum))>=0) {
		if (ainf->finf[fnum].links==0)	{
		delFile(ainf, drive, fnum, folnum);
	}
	}
	else if ((fnum=findFolderByName (ainf, drive, fname, folnum))>0)	{
		if (ainf->folinf[fnum].links==0)	{
		delFolder(ainf, drive, fnum);
	}
	}
	else	{
		printf("Incorrect path.\n");
		return;
	}

	//if unlinking original (links>0) then move info into one of the other links and update adress for all the rest
	//get rid of link and update rest
	//updateDrive(ainf, drive);
}

void delFolder (struct allInfo *ainf, FILE* drive, long folnum) {
	long parent=ainf->folinf[folnum].parent;

	long en=ainf->folinf[folnum].elements;
	long i=0;
	while (en)
	{
		if (ainf->folinf[folnum].type[i]==ELEM)	{
			unlink(ainf, drive, ainf->finf[ainf->folinf[folnum].address[i]].name, folnum);
			--en;
		}
		else if (ainf->folinf[folnum].type[i]==FOLDER)	{
			unlink(ainf, drive, ainf->folinf[ainf->folinf[folnum].address[i]].name, folnum);
			--en;
		}
	++i;
	}

	removeFromFolder(ainf, drive, parent, FOLDER, folnum);

	ainf->folinf[folnum].parent=-1;
	strcpy(ainf->folinf[folnum].name, "");
	ainf->dinf->folderNum--;
}

void listFiles (const struct allInfo *ainf) {
	long fi=ainf->dinf->fileNum;
	long i=0;
	while(fi)
	{
		if (ainf->finf[i].sectorAdr!=-1) {
			printf("%s", ainf->finf[i].name);
				if(ainf->finf[i].links==-1) printf (" (%s)", ainf->finf[ainf->finf[i].sectorAdr].name);
			--fi;
			printf("\t");
		}
		++i;
	}
	printf("\n");
}

void listFolders (const struct allInfo *ainf) {
	long fi=ainf->dinf->folderNum;
	long i=0;
	while(fi)
	{
		if (ainf->folinf[i].parent!=-1) {
			printf("%ld %s", ainf->folinf[i].parent, ainf->folinf[i].name);
				if(ainf->folinf[i].links==-1) printf (" (%s)", ainf->folinf[ainf->folinf[i].parent].name);
			--fi;
			printf("\t");
		}
		++i;
	}
	printf("\n");
}

void listElements (const struct allInfo *ainf) {
	long fi=ainf->folinf[ainf->curFolNum].elements;
	long i=0;
	while(fi)
	{
		if (ainf->folinf[ainf->curFolNum].type[i]==ELEM) {
			printf("%s", ainf->finf[ainf->folinf[ainf->curFolNum].address[i]].name);
				if(ainf->finf[i].links==-1) printf (" (%s)", ainf->finf[ainf->finf[i].sectorAdr].name);
			--fi;
			printf("\t");
		}
		else if (ainf->folinf[ainf->curFolNum].type[i]==FOLDER)	{
			printf("%s", ainf->folinf[ainf->folinf[ainf->curFolNum].address[i]].name);
				if(ainf->folinf[i].links==-1) printf (" (%s)", ainf->folinf[ainf->folinf[i].parent].name);
			--fi;
			printf("\t");
		}
		++i;
	}
	printf("\n");
}

unsigned long long countFolderSize (const struct allInfo *ainf, long folnum)	{
	unsigned long long fullSize=0;
	long en=ainf->folinf[folnum].elements;
	long i=0, h=0;
	while (en)
	{
		if (ainf->folinf[folnum].type[i]==ELEM)	{
			h=ainf->folinf[folnum].address[i];
			if (ainf->finf[h].links==-1) h=ainf->finf[h].sectorAdr;
			fullSize+=ainf->finf[h].size;
			--en;
		}
		else if (ainf->folinf[folnum].type[i]==FOLDER)	{
			h=ainf->folinf[folnum].address[i];
			if (ainf->folinf[h].links==-1) h=ainf->folinf[h].parent;
			fullSize+=countFolderSize(ainf, h);
			--en;
		}

	++i;
	}
	return fullSize;
}

void showFolderInfo (const struct allInfo *ainf) {
	unsigned long long thisSize=0, childrenSize=0;
	long en=ainf->folinf[ainf->curFolNum].elements;
	long i=0, h=0;
	while (en)
	{
		if (ainf->folinf[ainf->curFolNum].type[i]==ELEM)	{
			h=ainf->folinf[ainf->curFolNum].address[i];
			if (ainf->finf[h].links==-1) h=ainf->finf[h].sectorAdr;
			thisSize+=ainf->finf[h].size;
			--en;
		}
		else if (ainf->folinf[ainf->curFolNum].type[i]==FOLDER)	{
			h=ainf->folinf[ainf->curFolNum].address[i];
			if (ainf->folinf[h].links==-1) h=ainf->folinf[h].parent;
			childrenSize+=countFolderSize(ainf, h);
			--en;
		}
		++i;
	}
	printf("This folder:%llu\tThis path:%llu\tThis drive:%llu\n", thisSize, thisSize+childrenSize, ainf->dinf->size);
}

#endif /* DRIVE_H_ */
