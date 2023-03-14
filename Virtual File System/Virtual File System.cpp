#include "VFS.h"
#include <iostream>
using namespace std;
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
	
	const int n1 = 1200;
	const int n2 = 1000;
	char toWrite[n1 + 1] = { 0 };
	char toRead[n2 + 1] = { 0 };
	for (size_t i = 0; i < n1; i++) {
		toWrite[i] = 'A' + i % 26;
	}

	VFS vfs;
	if (File* f1 = vfs.Create("chunk1.bin\\aaa\\ccc.txt")) {
		cout << f1 << endl;
		cout << vfs.Write(f1, toWrite, n1) << endl; //todo file not closed
		vfs.Close(f1);
	}
	if (File* f2 = vfs.Open("chunk1.bin\\aaa\\ccc.txt")) {
		cout << f2 << endl;
		cout << vfs.Read(f2, toRead, n2) << endl;
		cout << toRead << endl;
		vfs.Close(f2);
	}
}
