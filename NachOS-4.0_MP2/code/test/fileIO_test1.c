#include "syscall.h"

int main(void)
{
	char test[] = "abcdefghijklmnopqrstuvwxyz";
	int success= Create("file1.test");
	OpenFileId fid, fid2, fid3;
	int i;
	char in[26];
	char in2[13];
	if (success != 1) MSG("Failed on creating file");
	fid = Open("file1.test");
	
	if (fid < 0) MSG("Failed on opening file");
	
	for (i = 0; i < 26; ++i) {
		int count = Write(test + i, 1, fid);
		if (count != 1) MSG("Failed on writing file");
	}
	
	success = Close(fid);
	if (success != 1) MSG("Failed on closing file");
	MSG("Success on creating file1.test!!!!!!!!!!!!!!!!!!!!!!!!!");

    MSG("\n\nStarting my test......\n\n");
	success = Create("f1");
	if (success != 1) MSG("Failed on creating file");
	fid = Open("f1");
	fid2 = Open("f2");

	if (fid < 0) MSG("Failed on opening file f1");
    if (fid2< 0) MSG("Failed on opening file f2");
	
	for (i = 0; i < 26; ++i) {
		int count = Write(test + i+3, 1, fid);
		if (count != 1) MSG("Failed on writing file");
	}
    if (Write(test + i, -5, fid) == -1) MSG("Failed on writing file size = -1");
    else MSG("Success write file size = -1");
	fid3 = Open("file1.test");
	if(Read(in, 13, fid3) != 13) MSG("Failed reading");
	MSG(in);
	Close(fid);
	fid = Open("f1");
	if(Write("12345", 5, fid) != 5) MSG("Failed writeing12345");
	Read(in2, 12, fid3);
	MSG(in2);
	fid2 = Open("file0.test");
	if(fid2<0) MSG("Failed Open2");

    success = Close(fid);
	if (success != 1) MSG("Failed on closing file f1");
	MSG("Success on creating f1");

    success = Close(fid2);
	if (success != 1) MSG("Failed on closing file f2");
	MSG("Success on creating f2");

	success = Close(fid3);
	if (success != 1) MSG("Failed on closing file 3");




	Halt();
}

