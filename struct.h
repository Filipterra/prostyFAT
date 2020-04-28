/*
 * struct.h
 *
 *  Created on: May 27, 2019
 *      Author: filip
 */

#ifndef STRUCT_H_
#define STRUCT_H_

#define SECTOR_SIZE 4000
#define FOLDER_NUMBER 30	//TODO: check always when creating folders/links to folders
#define FOLDER_ELEMENTS_NUMBER 10	//TODO: check always when creating files/folders/links
#define MAX_PATH_LENGTH	100	//TODO: check always when creating files/folders/links
#define NAME_SIZE 30
//type definitions for folderInfo
#define FREE 0
#define ELEM 1
#define FOLDER 2

struct sectorInfo {
	int size;
	int used;
	long next;
};

struct fileInfo {
	char name[NAME_SIZE];
	unsigned long long size;
	long sectorAdr;
	int links;	//-1 for links themselves and real file under sectorAdr //TODO:check max file number when creating links
};

struct folderInfo {
	char name[NAME_SIZE];
	long parent;
	long elements;
	char type[FOLDER_ELEMENTS_NUMBER];
	long address[FOLDER_ELEMENTS_NUMBER];
	int links;	//-1 for links themselves and real file under parent
};

struct driveInfo {
	char name[NAME_SIZE];
	unsigned long long size;
	long folderNum;
	long fileNum;
	long sectors;
	long freeSectors;
};

struct allInfo {
	struct driveInfo* dinf;
	struct folderInfo* folinf;
	struct fileInfo* finf;
	struct sectorInfo* sinf;
	unsigned long headerSize;
	long curFolNum;
};

#endif /* STRUCT_H_ */
