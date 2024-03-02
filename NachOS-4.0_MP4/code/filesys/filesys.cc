// filesys.cc
//	Routines to manage the overall operation of the file system.
//	Implements routines to map from textual file names to files.
//
//	Each file in the file system has:
//	   A file header, stored in a sector on disk
//		(the size of the file header data structure is arranged
//		to be precisely the size of 1 disk sector)
//	   A number of data blocks
//	   An entry in the file system directory
//
// 	The file system consists of several data structures:
//	   A bitmap of free disk sectors (cf. bitmap.h)
//	   A directory of file names and file headers
//
//      Both the bitmap and the directory are represented as normal
//	files.  Their file headers are located in specific sectors
//	(sector 0 and sector 1), so that the file system can find them
//	on bootup.
//
//	The file system assumes that the bitmap and directory files are
//	kept "open" continuously while Nachos is running.
//
//	For those operations (such as Create, Remove) that modify the
//	directory and/or bitmap, if the operation succeeds, the changes
//	are written immediately back to disk (the two files are kept
//	open during all this time).  If the operation fails, and we have
//	modified part of the directory and/or bitmap, we simply discard
//	the changed version, without writing it back to disk.
//
// 	Our implementation at this point has the following restrictions:
//
//	   there is no synchronization for concurrent accesses
//	   files have a fixed size, set when the file is created
//	   files cannot be bigger than about 3KB in size
//	   there is no hierarchical directory structure, and only a limited
//	     number of files can be added to the system
//	   there is no attempt to make the system robust to failures
//	    (if Nachos exits in the middle of an operation that modifies
//	    the file system, it may corrupt the disk)
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.
#ifndef FILESYS_STUB

#include "copyright.h"
#include "debug.h"
#include "disk.h"
#include "pbitmap.h"
#include "directory.h"
#include "filehdr.h"
#include "filesys.h"

// Sectors containing the file headers for the bitmap of free sectors,
// and the directory of files.  These file headers are placed in well-known
// sectors, so that they can be located on boot-up.
#define FreeMapSector 0
#define DirectorySector 1 //Trace code

// Initial file sizes for the bitmap and directory; until the file system
// supports extensible files, the directory size sets the maximum number
// of files that can be loaded onto the disk.
#define FreeMapFileSize (NumSectors / BitsInByte)
#define NumDirEntries 64 //MP4 IM
#define DirectoryFileSize (sizeof(DirectoryEntry) * NumDirEntries)

//----------------------------------------------------------------------
// FileSystem::FileSystem
// 	Initialize the file system.  If format = TRUE, the disk has
//	nothing on it, and we need to initialize the disk to contain
//	an empty directory, and a bitmap of free sectors (with almost but
//	not all of the sectors marked as free).
//
//	If format = FALSE, we just have to open the files
//	representing the bitmap and the directory.
//
//	"format" -- should we initialize the disk?
//----------------------------------------------------------------------

FileSystem::FileSystem(bool format)
{
    DEBUG(dbgFile, "Initializing the file system.");
    if (format)
    {
        PersistentBitmap *freeMap = new PersistentBitmap(NumSectors);
        Directory *directory = new Directory(NumDirEntries);
        FileHeader *mapHdr = new FileHeader(false);
        FileHeader *dirHdr = new FileHeader(false);

        DEBUG(dbgFile, "Formatting the file system.");

        // First, allocate space for FileHeaders for the directory and bitmap
        // (make sure no one else grabs these!)
        freeMap->Mark(FreeMapSector);
        freeMap->Mark(DirectorySector);

        // Second, allocate space for the data blocks containing the contents
        // of the directory and bitmap files.  There better be enough space!

        ASSERT(mapHdr->Allocate(freeMap, FreeMapFileSize));
        ASSERT(dirHdr->Allocate(freeMap, DirectoryFileSize));

        // Flush the bitmap and directory FileHeaders back to disk
        // We need to do this before we can "Open" the file, since open
        // reads the file header off of disk (and currently the disk has garbage
        // on it!).

        DEBUG(dbgFile, "Writing headers back to disk.");
        mapHdr->WriteBack(FreeMapSector);
        dirHdr->WriteBack(DirectorySector);

        // OK to open the bitmap and directory files now
        // The file system operations assume these two files are left open
        // while Nachos is running.

        freeMapFile = new OpenFile(FreeMapSector);
        directoryFile = new OpenFile(DirectorySector);

        // Once we have the files "open", we can write the initial version
        // of each file back to disk.  The directory at this point is completely
        // empty; but the bitmap has been changed to reflect the fact that
        // sectors on the disk have been allocated for the file headers and
        // to hold the file data for the directory and bitmap.

        DEBUG(dbgFile, "Writing bitmap and directory back to disk.");
        freeMap->WriteBack(freeMapFile); // flush changes to disk
        directory->WriteBack(directoryFile);

        if (debug->IsEnabled('f'))
        {
            freeMap->Print();
            directory->Print();
        }
        delete freeMap;
        delete directory;
        delete mapHdr;
        delete dirHdr;
    }
    else
    {
        // if we are not formatting the disk, just open the files representing
        // the bitmap and directory; these are left open while Nachos is running
        freeMapFile = new OpenFile(FreeMapSector);
        directoryFile = new OpenFile(DirectorySector);
    }
}

//----------------------------------------------------------------------
// MP4 mod tag
// FileSystem::~FileSystem
//----------------------------------------------------------------------
FileSystem::~FileSystem()
{
    delete freeMapFile;
    delete directoryFile;
}

//----------------------------------------------------------------------
// FileSystem::Create
// 	Create a file in the Nachos file system (similar to UNIX create).
//	Since we can't increase the size of files dynamically, we have
//	to give Create the initial size of the file.
//
//	The steps to create a file are:
//	  Make sure the file doesn't already exist
//        Allocate a sector for the file header
// 	  Allocate space on disk for the data blocks for the file
//	  Add the name to the directory
//	  Store the new file header on disk
//	  Flush the changes to the bitmap and the directory back to disk
//
//	Return TRUE if everything goes ok, otherwise, return FALSE.
//
// 	Create fails if:
//   		file is already in directory
//	 	no free space for file header
//	 	no free entry for file in directory
//	 	no free space for data blocks for the file
//
// 	Note that this implementation assumes there is no concurrent access
//	to the file system!
//
//	"name" -- name of file to be created
//	"initialSize" -- size of file to be created
//----------------------------------------------------------------------

bool FileSystem::Create(char *name, int initialSize)
{
    Directory *directory;
    PersistentBitmap *freeMap;
    FileHeader *hdr;
    int sector;
    bool success;

    DEBUG(dbgFile, "Creating file " << name << " size " << initialSize);

    directory = new Directory(NumDirEntries);
    // directory->FetchFrom(directoryFile);

    // directory = Traverse(name);
    OpenFile* dirFile = Traverse(name);
    directory->FetchFrom(dirFile);

    if (directory->Find(name) != -1)
        success = FALSE; // file is already in directory
    else
    {
        freeMap = new PersistentBitmap(freeMapFile, NumSectors);
        sector = freeMap->FindAndSet(); // find a sector to hold the file header
        if (sector == -1)
            success = FALSE; // no free block for file header
        else if (!directory->Add(name, sector, false))
            success = FALSE; // no space in directory
        else
        {
            hdr = new FileHeader(true);
            if (!hdr->Allocate(freeMap, initialSize))
                success = FALSE; // no space on disk for data
            else
            {
                success = TRUE;
                // everthing worked, flush all changes back to disk
                hdr->WriteBack(sector);
                // directory->WriteBack(directoryFile);
                directory->WriteBack(dirFile);
                freeMap->WriteBack(freeMapFile);
            }
            delete hdr;
        }
        delete freeMap;
    }
    delete directory;
    return success;
}

//----------------------------------------------------------------------
// FileSystem::Open
// 	Open a file for reading and writing.
//	To open a file:
//	  Find the location of the file's header, using the directory
//	  Bring the header into memory
//
//	"name" -- the text name of the file to be opened
//----------------------------------------------------------------------

OpenFile * FileSystem::Open(char *name)
{
    Directory *directory = new Directory(NumDirEntries);
    // Directory* directory = Traverse(name);
    OpenFile *openFile = NULL;
    int sector;
    OpenFile* dirFile = Traverse(name);
    

    DEBUG(dbgFile, "Opening file" << name);
    // directory->FetchFrom(directoryFile);
    directory->FetchFrom(dirFile);

    sector = directory->Find(name);
    if (sector >= 0)
        openFile = new OpenFile(sector); // name was found in directory
    delete directory;
    return openFile; // return NULL if not found
}

//----------------------------------------------------------------------
// FileSystem::Remove
// 	Delete a file from the file system.  This requires:
//	    Remove it from the directory
//	    Delete the space for its header
//	    Delete the space for its data blocks
//	    Write changes to directory, bitmap back to disk
//
//	Return TRUE if the file was deleted, FALSE if the file wasn't
//	in the file system.
//
//	"name" -- the text name of the file to be removed
//----------------------------------------------------------------------

bool FileSystem::Remove(char *name, bool recursive)
{
    Directory *directory;
    PersistentBitmap *freeMap;
    FileHeader *fileHdr;
    int sector;

    // directory = new Directory(NumDirEntries);
    // directory->FetchFrom(directoryFile);
    // directory = Traverse(name);
    OpenFile* dirFile = Traverse(name);
    directory = new Directory(NumDirEntries);
    directory->FetchFrom(dirFile);

    sector = directory->Find(name);
    if (sector == -1)
    {
        delete directory;
        cout << "Remove not found" << endl;
        return FALSE; // file not found
    }
    fileHdr = new FileHeader(false);
    fileHdr->FetchFrom(sector);

    freeMap = new PersistentBitmap(freeMapFile, NumSectors);

    if(!directory->Remove(name, recursive))
        cout << "Failed on remove" << endl;
    fileHdr->Deallocate(freeMap); // remove data blocks
    freeMap->Clear(sector);       // remove header block
    // directory->Remove(name, recursive);

    freeMap->WriteBack(freeMapFile);     // flush to disk
    // directory->WriteBack(directoryFile); // flush to disk
    directory->WriteBack(dirFile);
    delete fileHdr;
    delete directory;
    delete freeMap;
    return TRUE;
}

//----------------------------------------------------------------------
// FileSystem::List
// 	List all the files in the file system directory.
//----------------------------------------------------------------------

void FileSystem::List(char* name, bool recursive)
{
    // Directory *directory = new Directory(NumDirEntries);

    // directory->FetchFrom(directoryFile);
    // Directory* directory = Traverse(name);
    OpenFile* dirFile = Traverse(name);
    Directory *directory = new Directory(NumDirEntries);

    directory->FetchFrom(dirFile);

    int sector = directory->Find(name);
    if(sector!=-1) {
        OpenFile* cur_directoryFile = new OpenFile(sector);
        directory->FetchFrom(cur_directoryFile);
        delete cur_directoryFile;
    }

    directory->List(recursive, 0);
    delete directory;
}

//----------------------------------------------------------------------
// FileSystem::Print
// 	Print everything about the file system:
//	  the contents of the bitmap
//	  the contents of the directory
//	  for each file in the directory,
//	      the contents of the file header
//	      the data in the file
//----------------------------------------------------------------------

void FileSystem::Print()
{
    FileHeader *bitHdr = new FileHeader(false);
    FileHeader *dirHdr = new FileHeader(false);
    PersistentBitmap *freeMap = new PersistentBitmap(freeMapFile, NumSectors);
    Directory *directory = new Directory(NumDirEntries);

    printf("Bit map file header:\n");
    bitHdr->FetchFrom(FreeMapSector);
    bitHdr->Print();

    printf("Directory file header:\n");
    dirHdr->FetchFrom(DirectorySector);
    dirHdr->Print();

    freeMap->Print();

    directory->FetchFrom(directoryFile);
    directory->Print();

    delete bitHdr;
    delete dirHdr;
    delete freeMap;
    delete directory;
}


//MP4 IM

OpenFileId FileSystem::OpenAFile(char* name) {
    OpenFile* O_File = Open(name);
    if(O_File != NULL) {
        OF_table[1] = O_File;
        return 1;
    }
    else return 0;
}

int FileSystem::CreateI(char* name, int size) {
    if(Create(name, size))
        return 1;
    else
        return 0;
}

int FileSystem::Read(char *buf, int size, OpenFileId id) {
    return OF_table[id]->Read(buf, size);
}

int FileSystem::Write(char *buf, int size, OpenFileId id) {
    return OF_table[id]->Write(buf, size);
}

int FileSystem::Close(OpenFileId id) {
    OF_table[id] = NULL;
    return 1;
}

bool FileSystem::mkdir(char* name) {
    // Directory* dir = Traverse(name);
    OpenFile* dirFile = Traverse(name);
    Directory *dir = new Directory(NumDirEntries);

    dir->FetchFrom(dirFile);
    PersistentBitmap *freeMap = new PersistentBitmap(freeMapFile, NumSectors);
    FileHeader *hdr;
    int sector;
    bool success;
    // name = getName(name);
    // dir->List(false, 0);

    Directory *directory = new Directory(NumDirEntries);

    sector = freeMap->FindAndSet(); // find a sector to hold the file header
    if (sector == -1)
        success = FALSE; // no free block for file header
    else if (!dir->Add(name, sector, true))
        success = FALSE; // no space in directory
    else
    {
        hdr = new FileHeader(false);
        if (!hdr->Allocate(freeMap, DirectoryFileSize))
            success = FALSE; // no space on disk for data
        else
        {
            success = TRUE;
            // everthing worked, flush all changes back to disk
            hdr->WriteBack(sector);
            OpenFile* newDir = new OpenFile(sector);
            directory->WriteBack(newDir);
            freeMap->WriteBack(freeMapFile);
            // cout << "mkdir" << sector << endl;
            dir->WriteBack(dirFile);
        }

        delete hdr;
    }
    delete freeMap;
    delete dir;
    delete directory;

    return success;
}

OpenFile* FileSystem::Traverse(char* path) {
    Directory* cur_dir = new Directory(NumDirEntries);
    Directory* old_dir;
    OpenFile* cur_directoryFile = new OpenFile(DirectorySector);
    OpenFile* old_directoryFile;
    cur_dir->FetchFrom(cur_directoryFile);
    int j=0;
    char buf[10] = "\0\0\0\0\0\0\0\0\0";
    int sector;

    for(int i=1; path[i] != '\0'; i++) {
        if(path[i] == '/') {
            buf[j] = '\0';
            j=0;
            // cout << "AAA" << buf << endl;

            sector = cur_dir->Find(buf);
            if(sector!=-1) {
                // cout << "BBB" << buf << sector << endl;
                old_dir = cur_dir;
                cur_dir = new Directory(NumDirEntries);
                old_directoryFile = cur_directoryFile;
                cur_directoryFile = new OpenFile(sector);
                cur_dir->FetchFrom(cur_directoryFile);
                delete old_dir;
                delete old_directoryFile;
            }

            for(int k=0; k<10; k++)
                buf[k] = '\0';
        }
        else
            buf[j++] = path[i];
    }

    tmp_name = buf;
    if(path[1] == '\0')
        tmp_name = "/";

    // cout << buf << endl;
    // cout << tmp_name << endl;

    return cur_directoryFile;
}

#endif // FILESYS_STUB