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
	char toRead11[n1 + 1] = { 0 };
	char toRead22[n2 + 1] = { 0 };
	char toRead33[n3 + 1] = { 0 };
	char toRead44[n4 + 1] = { 0 };
	for (size_t i = 0; i < n1; i++) {
		toWrite1[i] = 'A' + i % 26;
	}
	for (size_t i = 0; i < n1; i++) {
		toWrite2[i] = 'z' - i % 26;
	}
	for (size_t i = 0; i < n1; i++) {
		toWrite3[i] = '0' + i % 10;
	}
	for (size_t i = 0; i < n1; i++) {
		toWrite4[i] = '9' - i % 10;
	}
	for (size_t i = 0; i < n1; i++) {
		toWrite5[i] = 'Z' + i % 10;
	}

	VFS vfs;

	for (string s : realFiles) {
		VFS::mutexMap.insert(make_pair(s, new shared_mutex));
	}

	std::thread t1([&]() {
		if (File* f = vfs.Create("chunk1.bin\\dir1\\1a.txt")) {
			vfs.Write(f, toWrite1, n1);
			this_thread::sleep_for(chrono::milliseconds(100));
			vfs.Write(f, toWrite2, n2);
			vfs.Close(f);
		}
		if (File* f = vfs.Create("chunk1.bin\\dir2\\2a.txt")) {
			vfs.Write(f, toWrite5, n5);
			this_thread::sleep_for(chrono::milliseconds(100));
			vfs.Write(f, toWrite2, n2);
			vfs.Close(f);
		}
		if (File* f = vfs.Open("chunk1.bin\\dir1\\1bc.txt")) {
			vfs.Read(f, toRead5, n5);
			this_thread::sleep_for(chrono::milliseconds(100));
			vfs.Read(f, toRead2, n2);
			vfs.Close(f);
		}
	});

	if (File* f = vfs.Create("chunk1.bin\\dir1\\1bc.txt")) {
		vfs.Write(f, toWrite2, n2);
		this_thread::sleep_for(chrono::milliseconds(100));
		vfs.Write(f, toWrite4, n4);
		vfs.Close(f);
	}
	if (File* f = vfs.Create("chunk1.bin\\dir1\\1bc.txt")) {
		vfs.Write(f, toWrite2, n3);
		this_thread::sleep_for(chrono::milliseconds(100));
		vfs.Write(f, toWrite4, n5);
		vfs.Close(f);
	}
	if (File* f = vfs.Open("chunk1.bin\\dir1\\1a.txt")) {
		vfs.Read(f, toRead4, n4);
		this_thread::sleep_for(chrono::milliseconds(100));
		vfs.Read(f, toRead2, n2);
		vfs.Close(f);
	}
	
	t1.join();
}
