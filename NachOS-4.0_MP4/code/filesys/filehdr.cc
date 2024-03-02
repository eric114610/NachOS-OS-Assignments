// filehdr.cc
//	Routines for managing the disk file header (in UNIX, this
//	would be called the i-node).
//
//	The file header is used to locate where on disk the
//	file's data is stored.  We implement this as a fixed size
//	table of pointers -- each entry in the table points to the
//	disk sector containing that portion of the file data
//	(in other words, there are no indirect or doubly indirect
//	blocks). The table size is chosen so that the file header
//	will be just big enough to fit in one disk sector,
//
//      Unlike in a real system, we do not keep track of file permissions,
//	ownership, last modification date, etc., in the file header.
//
//	A file header can be initialized in two ways:
//	   for a new file, by modifying the in-memory data structure
//	     to point to the newly allocated data blocks
//	   for a file already on disk, by reading the file header from disk
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"

#include "filehdr.h"
#include "debug.h"
#include "synchdisk.h"
#include "main.h"


#include <sstream>
//----------------------------------------------------------------------
// MP4 mod tag
// FileHeader::FileHeader
//	There is no need to initialize a fileheader,
//	since all the information should be initialized by Allocate or FetchFrom.
//	The purpose of this function is to keep valgrind happy.
//----------------------------------------------------------------------
FileHeader::FileHeader(bool firstf)
{
	numBytes = -1;
	numSectors = -1;
	memset(dataSectors, -1, sizeof(dataSectors));
	first = firstf;
}

//----------------------------------------------------------------------
// MP4 mod tag
// FileHeader::~FileHeader
//	Currently, there is not need to do anything in destructor function.
//	However, if you decide to add some "in-core" data in header
//	Always remember to deallocate their space or you will leak memory
//----------------------------------------------------------------------
FileHeader::~FileHeader()
{
	// nothing to do now
}

//----------------------------------------------------------------------
// FileHeader::Allocate
// 	Initialize a fresh file header for a newly created file.
//	Allocate data blocks for the file out of the map of free disk blocks.
//	Return FALSE if there are not enough free blocks to accomodate
//	the new file.
//
//	"freeMap" is the bit map of free disk sectors
//	"fileSize" is the bit map of free disk sectors
//----------------------------------------------------------------------
//Trace code
bool FileHeader::Allocate(PersistentBitmap *freeMap, int fileSize)
{
	// numBytes = fileSize;
	// numSectors = divRoundUp(fileSize, SectorSize);
	// if (freeMap->NumClear() < numSectors)
	// 	return FALSE; // not enough space

	// for (int i = 0; i < numSectors; i++)
	// {
	// 	dataSectors[i] = freeMap->FindAndSet();
	// 	// since we checked that there was enough free space,
	// 	// we expect this to succeed
	// 	ASSERT(dataSectors[i] >= 0);
	// }
	// DEBUG(dbgDisk, "size:" << fileSize << " " << MaxFileSize*NumDirect);

	remainSize = fileSize;
    // DEBUG(dbgDisk, "level start");
	if (fileSize <= MaxFileSize) {
		DEBUG(dbgDisk, "level 1 H");
		level = 1;
		numBytes = fileSize;
		numSectors = divRoundUp(fileSize, SectorSize);
		if (freeMap->NumClear() < numSectors)
			return FALSE; // not enough space

		for (int i = 0; i < numSectors; i++)
		{
			dataSectors[i] = freeMap->FindAndSet();
			// since we checked that there was enough free space,
			// we expect this to succeed
			ASSERT(dataSectors[i] >= 0);
		}
		if(first) {
			first = false;
			// DEBUG(dbgHead, "File with size %d, use level 1 header, header size = 1 (Sectors)", fileSize);
			// cout << "File with size " << fileSize << ", use level 1 header, header size = 1 (Sectors)" << endl;
			ostringstream S;
			S << "File with size " << fileSize << ", use level 1 header, header size = 1 (Sectors)";
			string S2 = S.str();
			DEBUG(dbgHead, S2);

		}
	}
	else if(fileSize <= MaxFileSize*NumDirect) {
		DEBUG(dbgDisk, "level 2 H");
		level = 2;
		numBytes = fileSize;
		numSectors = divRoundUp(fileSize, MaxFileSize);
		if (freeMap->NumClear() < numSectors)
			return FALSE; // not enough space

		for(int i=0; i<numSectors; i++) {
			dataSectors[i] = freeMap->FindAndSet();
			child[i] = new FileHeader(false);
			if(remainSize >= MaxFileSize)
				child[i]->Allocate(freeMap, MaxFileSize);
			else
				child[i]->Allocate(freeMap, remainSize);
			child[i]->WriteBack(dataSectors[i]);
			remainSize -= MaxFileSize;
		}

		if(first) {
			first = false;
			// DEBUG(dbgHead, "File with size %d, use level 2 header, contains %d level 1 header, total header size = %d (Sectors)", fileSize, numSectors, (numSectors+1));
			// cout << "File with size " << fileSize << ", use level 2 header, contains " << numSectors << " level 1 header, total header size = " << (numSectors+1) << " (Sectors)" << endl;
			ostringstream S;
			S << "File with size " << fileSize << ", use level 2 header, contains " << numSectors << " level 1 header, total header size = " << (numSectors+1) << " (Sectors)";
			string S2 = S.str();
			DEBUG(dbgHead, S2);
		}
	}
	else if(fileSize <= MaxFileSize*NumDirect*NumDirect) {
		DEBUG(dbgDisk, "level 3 H");
		level = 3;
		numBytes = fileSize;
		numSectors = divRoundUp(fileSize, MaxFileSize*NumDirect);
		if (freeMap->NumClear() < numSectors)
			return FALSE; // not enough space

		for(int i=0; i<numSectors; i++) {
			dataSectors[i] = freeMap->FindAndSet();
			child[i] = new FileHeader(false);
			if(remainSize >= MaxFileSize*NumDirect)
				child[i]->Allocate(freeMap, MaxFileSize*NumDirect);
			else
				child[i]->Allocate(freeMap, remainSize);
			child[i]->WriteBack(dataSectors[i]);
			remainSize -= MaxFileSize*NumDirect;
		}

		if(first) {
			first = false;
			// DEBUG(dbgHead, "File with size %d, use level 3 header, contains %d level 2 header, total header size = %d (Sectors)", fileSize, numSectors, (numSectors*31+1));
			// cout << "File with size " << fileSize << ", use level 3 header, contains " << numSectors << " level 2 header, total header size = " << (numSectors*31+1) << " (Sectors)" << endl;
			ostringstream S;
			S << "File with size " << fileSize << ", use level 3 header, contains " << numSectors << " level 2 header, total header size = " << (numSectors*31+1) << " (Sectors)";
			string S2 = S.str();
			DEBUG(dbgHead, S2);
		}
	}
	else {
        DEBUG(dbgDisk, "level 4 H");
		level = 4;
		numBytes = fileSize;
		numSectors = divRoundUp(fileSize, MaxFileSize*NumDirect*NumDirect);
		if (freeMap->NumClear() < numSectors)
			return FALSE; // not enough space
		
		for(int i=0; i<numSectors; i++) {
			dataSectors[i] = freeMap->FindAndSet();
			child[i] = new FileHeader(false);
			if(remainSize >= MaxFileSize)
				child[i]->Allocate(freeMap, MaxFileSize*NumDirect*NumDirect);
			else
				child[i]->Allocate(freeMap, remainSize);
			child[i]->WriteBack(dataSectors[i]);
			remainSize -= MaxFileSize*NumDirect*NumDirect;
		}

		if(first) {
			first = true;
			// DEBUG(dbgHead, "File with size %d, use level 4 header, contains %d level 3 header, total header size = %d (Sectors)", fileSize, numSectors, (numSectors*931+1));
			// cout << "File with size " << fileSize << ", use level 4 header, contains " << numSectors << " level 3 header, total header size = " << (numSectors*931+1) << " (Sectors)" << endl;
			ostringstream S;
			S << "File with size " << fileSize << ", use level 4 header, contains " << numSectors << " level 3 header, total header size = " << (numSectors*931+1) << " (Sectors)";
			string S2 = S.str();
			DEBUG(dbgHead, S2);
		}
	}


	return TRUE;
}

//----------------------------------------------------------------------
// FileHeader::Deallocate
// 	De-allocate all the space allocated for data blocks for this file.
//
//	"freeMap" is the bit map of free disk sectors
//----------------------------------------------------------------------

void FileHeader::Deallocate(PersistentBitmap *freeMap)
{
	// for (int i = 0; i < numSectors; i++)
	// {
	// 	ASSERT(freeMap->Test((int)dataSectors[i])); // ought to be marked!
	// 	freeMap->Clear((int)dataSectors[i]);
	// }
	if(level == 1) {
		for (int i = 0; i < numSectors; i++)
		{
			ASSERT(freeMap->Test((int)dataSectors[i])); // ought to be marked!
			freeMap->Clear((int)dataSectors[i]);
		}
	}
	else {
		for (int i = 0; i < numSectors; i++)
		{
			ASSERT(freeMap->Test((int)dataSectors[i])); // ought to be marked!
			freeMap->Clear((int)dataSectors[i]);
			child[i]->Deallocate(freeMap);
		}
	}
}

//----------------------------------------------------------------------
// FileHeader::FetchFrom
// 	Fetch contents of file header from disk.
//
//	"sector" is the disk sector containing the file header
//----------------------------------------------------------------------

void FileHeader::FetchFrom(int sector)
{
	// kernel->synchDisk->ReadSector(sector, (char *)this);

	/*
		MP4 Hint:
		After you add some in-core informations, you will need to rebuild the header's structure
	*/

	int buf[SectorSize/sizeof(int)];
	kernel->synchDisk->ReadSector(sector, (char*) buf);
	numBytes = buf[0];
	numSectors = buf[1];
	level = buf[2];
	memcpy(dataSectors, buf+3, sizeof(dataSectors));

	if(level != 1) {
		for(int i=0; i<numSectors; i++) {
			child[i] = new FileHeader(false);
			child[i]->FetchFrom(dataSectors[i]);
		}
	}
}

//----------------------------------------------------------------------
// FileHeader::WriteBack
// 	Write the modified contents of the file header back to disk.
//
//	"sector" is the disk sector to contain the file header
//----------------------------------------------------------------------

void FileHeader::WriteBack(int sector)
{
	// kernel->synchDisk->WriteSector(sector, (char *)this);

	/*
		MP4 Hint:
		After you add some in-core informations, you may not want to write all fields into disk.
		Use this instead:
		char buf[SectorSize];
		memcpy(buf + offset, &dataToBeWritten, sizeof(dataToBeWritten));
		...
	*/
	int buf[SectorSize/sizeof(int)];
	buf[0] = numBytes;
	buf[1] = numSectors;
	buf[2] = level;
	memcpy(buf+3, dataSectors, sizeof(dataSectors));

	kernel->synchDisk->WriteSector(sector, (char*) buf);


	if(level != 1) {
		for (int i = 0; i < numSectors; i++)
		{
			child[i]->WriteBack(dataSectors[i]);
		}
	}

}

//----------------------------------------------------------------------
// FileHeader::ByteToSector
// 	Return which disk sector is storing a particular byte within the file.
//      This is essentially a translation from a virtual address (the
//	offset in the file) to a physical address (the sector where the
//	data at the offset is stored).
//
//	"offset" is the location within the file of the byte in question
//----------------------------------------------------------------------

int FileHeader::ByteToSector(int offset)
{
	// return (dataSectors[offset / SectorSize]);

	if(level == 1)
		return (dataSectors[offset / SectorSize]);
	else if(level == 2) {
		int next = offset / MaxFileSize;
		return child[next]->ByteToSector(offset % MaxFileSize);
	}
	else if(level == 3) {
		int next = offset / (MaxFileSize*NumDirect);
		return child[next]->ByteToSector(offset % (MaxFileSize*NumDirect));
	}
	else {
		int next = offset / (MaxFileSize*NumDirect*NumDirect);
		return child[next]->ByteToSector(offset % (MaxFileSize*NumDirect*NumDirect));
	}
}

//----------------------------------------------------------------------
// FileHeader::FileLength
// 	Return the number of bytes in the file.
//----------------------------------------------------------------------

int FileHeader::FileLength()
{
	return numBytes;
}

//----------------------------------------------------------------------
// FileHeader::Print
// 	Print the contents of the file header, and the contents of all
//	the data blocks pointed to by the file header.
//----------------------------------------------------------------------

void FileHeader::Print()
{
	int i, j, k;
	char *data = new char[SectorSize];

	printf("FileHeader contents.  File size: %d.  File blocks:\n", numBytes);
	for (i = 0; i < numSectors; i++)
		printf("%d ", dataSectors[i]);
	printf("\nFile contents:\n");
	for (i = k = 0; i < numSectors; i++)
	{
		kernel->synchDisk->ReadSector(dataSectors[i], data);
		for (j = 0; (j < SectorSize) && (k < numBytes); j++, k++)
		{
			if ('\040' <= data[j] && data[j] <= '\176') // isprint(data[j])
				printf("%c", data[j]);
			else
				printf("\\%x", (unsigned char)data[j]);
		}
		printf("\n");
	}
	delete[] data;
}
