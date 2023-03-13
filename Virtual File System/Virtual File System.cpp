#include "VFS.h"
using namespace TestTask;

int main()
{
	///*
	std::fstream fs;
	fs.open("chunk1.bin", std::fstream::out);
	char zeroes[8] = { 0 };
	char tailingZeroes[1024 - 18] = { 0 };
	fs.write(zeroes, 8);
	fs << "chunk1.bin";
	fs.write(tailingZeroes, sizeof tailingZeroes);
	fs.flush();
	//*/
	char toWrite[5000] = "";
	for (int i = 0; i < 5000; i++) {
		toWrite[i] = 'A';
	}
	VFS vfs;
	File* f = vfs.Create("chunk1.bin\\aaa\\ddd.txt");
	vfs.Write(f, toWrite, 5000);
	vfs.Close(f);
	File* f1 = vfs.Open("chunk1.bin\\aaa\\ddd.txt");
	vfs.Close(f1);
}
