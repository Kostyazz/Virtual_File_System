#include "VFS.h"
using namespace TestTask;

int main()
{
	std::fstream fs;
	fs.open("chunk1.bin", std::fstream::out);
	char zeroes[8] = { 0 };
	char tailingZeroes[1024 - 18] = { 0 };
	fs.write(zeroes, 8);
	fs << "chunk1.bin";
	fs.write(tailingZeroes, sizeof tailingZeroes);
	fs.flush();

	VFS vfs;
	File* f = vfs.Create("chunk1.bin\\aaa\\bbb");
	vfs.Close(f);
	File* f1 = vfs.Create("chunk1.bin\\aaa\\bbb");
}
