#include "VFS.h"
#include <iostream>
#include <thread>
#include <chrono>
using namespace std;
using namespace TestTask;

void prepareRealFile(string filename) {
	std::fstream fs;
	fs.open(filename, std::fstream::out | std::fstream::binary);
	char Zeroes[VFS::BlockSize] = { 0 };
	fs.write(Zeroes, 8);
	fs.write(filename.c_str(), filename.length());
	fs.write(Zeroes, sizeof Zeroes - filename.length() - 8);
	fs.flush();
}

const string realFiles[2] = { "chunk1.bin",
							  "chunk2.bin" };

int main()
{

	prepareRealFile("chunk1.bin");

	const int n1 = 5000;
	const int n2 = 6000;
	const int n3 = 10'764;
	const int n4 = 15'289;
	const int n5 = 20'000;

	char toWrite1[n1 + 1] = { 0 };
	char toWrite2[n2 + 1] = { 0 };
	char toWrite3[n3 + 1] = { 0 };
	char toWrite4[n4 + 1] = { 0 };
	char toWrite5[n5 + 1] = { 0 };

	char toRead1[n1 + 1] = { 0 };
	char toRead2[n2 + 1] = { 0 };
	char toRead3[n3 + 1] = { 0 };
	char toRead4[n4 + 1] = { 0 };
	char toRead5[n5 + 1] = { 0 };
	for (size_t i = 0; i < n1; i++) {
		toWrite1[i] = 'A' + i % 26;
	}
	for (size_t i = 0; i < n2; i++) {
		toWrite2[i] = 'z' - i % 26;
	}
	for (size_t i = 0; i < n3; i++) {
		toWrite3[i] = '0' + i % 10;
	}
	for (size_t i = 0; i < n4; i++) {
		toWrite4[i] = '9' - i % 10;
	}
	for (size_t i = 0; i < n5; i++) {
		toWrite5[i] = 'Z' + i % 10;
	}

	VFS vfs;

	for (string s : realFiles) {
		VFS::mutexMap.emplace(s, new shared_mutex);
	}

//	std::thread t1([&]() {
		File* f1 = vfs.Create("chunk1.bin\\dir1\\1a.txt");
		if (f1) {
			vfs.Write(f1, toWrite1, n1);
			vfs.Write(f1, toWrite2, n2);
		}
		File* f2 = vfs.Create("chunk1.bin\\dir2\\2a.txt");
		if (f2) {
			vfs.Write(f2, toWrite5, n5);
			vfs.Write(f2, toWrite2, n2);
		}
		if (f1) vfs.Close(f1);


		f1 = vfs.Create("chunk1.bin\\dir1\\1b.txt");
		if (f1) {
			vfs.Write(f1, toWrite3, n3);
			vfs.Write(f1, toWrite4, n4);
		}
		f2 = vfs.Create("chunk1.bin\\dir1\\1c.txt");
		if (f2) {
			vfs.Write(f2, toWrite2, n3);
			vfs.Write(f2, toWrite4, n5);
		}
		if (f1) vfs.Close(f1);


		File* f3;
		do {
			f3 = vfs.Open("chunk1.bin\\dir1\\1b.txt");
		} while (!f3);
		vfs.Read(f3, toRead3, n3);
		vfs.Read(f3, toRead4, n4);

		if (f2) vfs.Close(f2);
		if (f3) vfs.Close(f3);
//	});
/*	File* f3;
	do {
		f3 = vfs.Open("chunk1.bin\\dir1\\1a.txt");
	} while (f3 == nullptr);
	vfs.Read(f3, toRead1, n1);
	vfs.Read(f3, toRead2, n2);
	if (f2) vfs.Close(f2);
	if (f3) vfs.Close(f3);
//	t1.join(); */
	cout << toWrite1 << endl;
	cout << toRead1 << endl;
	cout << strcmp(toWrite1, toRead1) << endl;
	cout << strcmp(toWrite2, toRead2) << endl;
	cout << strcmp(toWrite3, toRead3) << endl;
	cout << strcmp(toWrite4, toRead4) << endl;

}
