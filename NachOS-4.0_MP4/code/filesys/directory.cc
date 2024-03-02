// directory.cc
//	Routines to manage a directory of file names.
//
//	The directory is a table of fixed length entries; each
//	entry represents a single file, and contains the file name,
//	and the location of the file header on disk.  The fixed size
//	of each directory entry means that we have the restriction
//	of a fixed maximum size for file names.
//
//	The constructor initializes an empty directory of a certain size;
//	we use ReadFrom/WriteBack to fetch the contents of the directory
//	from disk, and to write back any modifications back to disk.
//
//	Also, this implementation has the restriction that the size
//	of the directory cannot expand.  In other words, once all the
//	entries in the directory are used, no more files can be created.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "utility.h"
#include "filehdr.h"
#include "directory.h"

#define NumDirEntries 64 //MP4 IM
//----------------------------------------------------------------------
// Directory::Directory
// 	Initialize a directory; initially, the directory is completely
//	empty.  If the disk is being formatted, an empty directory
//	is all we need, but otherwise, we need to call FetchFrom in order
//	to initialize it from disk.
//
//	"size" is the number of entries in the directory
//----------------------------------------------------------------------

Directory::Directory(int size)
{
    table = new DirectoryEntry[size];

    // MP4 mod tag
    memset(table, 0, sizeof(DirectoryEntry) * size); // dummy operation to keep valgrind happy

    tableSize = size;
    for (int i = 0; i < tableSize; i++) { //MP4 IM
        table[i].inUse = FALSE;
        table[i].isDir = FALSE;
    }
}

//----------------------------------------------------------------------
// Directory::~Directory
// 	De-allocate directory data structure.
//----------------------------------------------------------------------

Directory::~Directory()
{
    
}

//----------------------------------------------------------------------
// Directory::FetchFrom
// 	Read the contents of the directory from disk.
//
//	"file" -- file containing the directory contents
//----------------------------------------------------------------------

void Directory::FetchFrom(OpenFile *file)
{
    (void)file->ReadAt((char *)table, tableSize * sizeof(DirectoryEntry), 0);
}

//----------------------------------------------------------------------
// Directory::WriteBack
// 	Write any modifications to the directory back to disk
//
//	"file" -- file to contain the new directory contents
//----------------------------------------------------------------------

void Directory::WriteBack(OpenFile *file)
{
    (void)file->WriteAt((char *)table, tableSize * sizeof(DirectoryEntry), 0);
}

//----------------------------------------------------------------------
// Directory::FindIndex
// 	Look up file name in directory, and return its location in the table of
//	directory entries.  Return -1 if the name isn't in the directory.
//
//	"name" -- the file name to look up
//----------------------------------------------------------------------

int Directory::FindIndex(char *name)
{
    for (int i = 0; i < tableSize; i++)
        if (table[i].inUse && !strncmp(table[i].name, name, FileNameMaxLen))
            return i;
    return -1; // name not in directory
}

//----------------------------------------------------------------------
// Directory::Find
// 	Look up file name in directory, and return the disk sector number
//	where the file's header is stored. Return -1 if the name isn't
//	in the directory.
//
//	"name" -- the file name to look up
//----------------------------------------------------------------------
//MP4 IM
int Directory::Find(char *name)
{
    int j=0;
    char buf[10] = "\0\0\0\0\0\0\0\0\0";

    for(int i=0; name[i] != '\0'; i++) {
        if(name[i] == '/') {
            for(int k=0; k<10; k++)
                buf[k] = '\0';
            j=0;
        }
        else
            buf[j++] = name[i];
    }
    buf[j] = '\0';

    // cout << "Find:" << buf << endl;

    int i = FindIndex(buf);

    if (i != -1)
        return table[i].sector;
    return -1;
}

//----------------------------------------------------------------------
// Directory::Add
// 	Add a file into the directory.  Return TRUE if successful;
//	return FALSE if the file name is already in the directory, or if
//	the directory is completely full, and has no more space for
//	additional file names.
//
//	"name" -- the name of the file being added
//	"newSector" -- the disk sector containing the added file's header
//----------------------------------------------------------------------

bool Directory::Add(char *name, int newSector, bool isDir)
{
    int j=0;
    char buf[10] = "\0\0\0\0\0\0\0\0\0";

    for(int i=0; name[i] != '\0'; i++) {
        if(name[i] == '/') {
            for(int k=0; k<10; k++)
                buf[k] = '\0';
            j=0;
        }
        else
            buf[j++] = name[i];
    }
    buf[j] = '\0';

    // cout << "DDDDD" << buf << endl;

    if (FindIndex(buf) != -1)
        return FALSE;

    for (int i = 0; i < tableSize; i++)
        if (!table[i].inUse)
        {
            table[i].inUse = TRUE;
            strncpy(table[i].name, buf, FileNameMaxLen);
            if(!isDir) {
                table[i].isDir = false;
                table[i].sector = newSector;
            }
            else {
                table[i].isDir = true;
                table[i].sector = newSector;
            }
            return TRUE;
        }
    return FALSE; // no space.  Fix when we have extensible files.
}

//----------------------------------------------------------------------
// Directory::Remove
// 	Remove a file name from the directory.  Return TRUE if successful;
//	return FALSE if the file isn't in the directory.
//
//	"name" -- the file name to be removed
//----------------------------------------------------------------------

bool Directory::Remove(char *name, bool recursive)
{
    int j=0;
    char buf[10] = "\0\0\0\0\0\0\0\0\0";

    for(int i=0; name[i] != '\0'; i++) {
        if(name[i] == '/') {
            for(int k=0; k<10; k++)
                buf[k] = '\0';
            j=0;
        }
        else
            buf[j++] = name[i];
    }
    buf[j] = '\0';

    int i = FindIndex(buf);

    if (i == -1 || (table[i].isDir && !recursive))
        return FALSE; // name not in directory

    if(recursive) {
        Directory* next = new Directory(NumDirEntries);
        OpenFile* next_file = new OpenFile(table[i].sector);
        next->FetchFrom(next_file);
        next->RemoveAll(next_file);
    }

    table[i].inUse = FALSE;
    table[i].isDir = FALSE;
    return TRUE;
}

//----------------------------------------------------------------------
// Directory::List
// 	List all the file names in the directory.
//----------------------------------------------------------------------

void Directory::List(bool recursive, int Le)
{
    if(!recursive) {
        for (int i = 0; i < tableSize; i++)
            if (table[i].inUse) {
                
                    cout << table[i].name << endl;
            }
    }
    else {
        for (int i = 0; i < tableSize; i++)
            if (table[i].inUse) {
                for(int j=0; j<Le; j++) 
                    cout << "    ";

                if(table[i].isDir) {
                    cout << "[D] " << table[i].name << endl;
                    Directory* next = new Directory(NumDirEntries);
                    OpenFile* nextFile = new OpenFile(table[i].sector);
                    next->FetchFrom(nextFile);
                    next->List(recursive, Le+1);

                    delete next;
                    delete nextFile;
                }
                else
                    printf("[F] %s\n", table[i].name);
            }
    }
}

//----------------------------------------------------------------------
// Directory::Print
// 	List all the file names in the directory, their FileHeader locations,
//	and the contents of each file.  For debugging.
//----------------------------------------------------------------------

void Directory::Print()
{
    FileHeader *hdr = new FileHeader(false);

    printf("Directory contents:\n");
    for (int i = 0; i < tableSize; i++)
        if (table[i].inUse)
        {
            printf("Name: %s, Sector: %d\n", table[i].name, table[i].sector);
            hdr->FetchFrom(table[i].sector);
            hdr->Print();
        }
    printf("\n");
    delete hdr;
}

//MP4 IM
void Directory::RemoveAll(OpenFile* dirFile) {
    // cout << "In RemoveAll" << endl;

    FileHeader* fileHdr = new FileHeader(false);
    OpenFile* freeMapFile;
    PersistentBitmap *freeMap;

    for (int i = 0; i < tableSize; i++) {
        if (table[i].inUse && table[i].isDir) {
            Directory* next = new Directory(NumDirEntries);
            OpenFile* next_file = new OpenFile(table[i].sector);
            next->FetchFrom(next_file);
            next->RemoveAll(next_file);
            fileHdr->FetchFrom(table[i].sector);
            table[i].inUse = FALSE;
            table[i].isDir = FALSE;
        }
        else if(table[i].inUse) {
            fileHdr->FetchFrom(table[i].sector);
            table[i].inUse = FALSE;
            table[i].isDir = FALSE;
        }

        if(table[i].inUse) {
            freeMapFile = new OpenFile(0);
            freeMap = new PersistentBitmap(freeMapFile, NumSectors);

            fileHdr->Deallocate(freeMap); // remove data blocks
            freeMap->Clear(table[i].sector);       // remove header block
            // directory->Remove(name, recursive);

            freeMap->WriteBack(freeMapFile);     // flush to disk
            // directory->WriteBack(directoryFile); // flush to disk
            WriteBack(dirFile);
        }
    }
}