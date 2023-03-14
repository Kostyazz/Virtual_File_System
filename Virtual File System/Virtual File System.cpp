#include "VFS.h"
using namespace TestTask;

int main()
{
	
	std::fstream fs;
	fs.open("chunk1.bin", std::fstream::out);
	char zeroes[8] = { 0 };
	char tailingZeroes[1024 - 18] = { 0 };
	fs.write(zeroes, 8);
	fs.write("chunk1.bin", 10);
	fs.write(tailingZeroes, sizeof tailingZeroes);
	fs.flush();
	
	VFS vfs;
	char toWrite[5000];
	for (size_t i = 0; i < 5000; i++) {
		toWrite[i] = 'A';
	}
	File* f = vfs.Create("chunk1.bin\\aaa\\ccc.txt");
	vfs.Write(f, toWrite, 5000);
	vfs.Close(f);
	//File* f1 = vfs.Open("chunk1.bin\\aaa\\ccc.txt");
	//vfs.Close(f1);
}
