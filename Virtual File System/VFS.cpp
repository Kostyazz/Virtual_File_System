#include "VFS.h"
#include "File.h"
#include <vector>
#include <string>
#include <fstream>
#include <iostream>

using namespace std;
using namespace TestTask;

const string delimiter = "\\";
const size_t MaxNameLength = 256; //including \0
const size_t BlockSize = 1024;
const size_t HashDivider = (BlockSize - MaxNameLength - 8) / 8; //95
const char Zeroes[BlockSize] = { 0 };

unique_ptr<vector<string> > parsePath(const char * name) {
	unique_ptr<vector<string> > v(new vector<string>);
	string s = string(name);
	size_t delimPos = 0;
	while (delimPos != string::npos) {
		delimPos = s.find(delimiter);
		v->push_back(s.substr(0, delimPos));
		s.erase(0, delimPos + delimiter.length());
	}
	return v;
}

size_t getHashPos(string s) {
	return 8 + MaxNameLength + 8 * (hash<string>{}(s) % HashDivider);
}


File * VFS::Open(const char * fullPath)
{
	unique_ptr<vector<string> > path = parsePath(fullPath);
	string realFileName = path->front();
	unique_ptr<fstream> fs(new fstream());

	//todo follow path

	if (File::mutexMap[fullPath].try_lock_shared()) {
		fs->open(realFileName, fstream::in);
		File* f = new File(fullPath, std::move(fs), File::read);
		return f;
	} else {
		return nullptr;
	}
}

File * VFS::Create(const char * fullPath)
{
	unique_ptr<vector<string> > path = parsePath(fullPath);
	string realFileName = path->front();
	unique_ptr<fstream> fs(new fstream());

	fs->open(realFileName, fstream::in | fstream::out | fstream::binary);

	char buf8[8];
	_int64* nextLink = static_cast<_int64*>((void*)buf8);
	unique_ptr<_int64> prevLink(new _int64(0));
	char curName[MaxNameLength]{ 0 };
	bool notFound = false;

	for (size_t i = 1; i < path->size(); i++) {
		*nextLink = getHashPos((*path)[i]);
		*prevLink = *nextLink;
		fs->seekg(*nextLink, ios::beg);
		fs->read(buf8, 8); //reading nextLink value
		//unwinding collisions
		while (*nextLink && strcmp(curName, (*path)[i].c_str())) {
			*prevLink = *nextLink;
			fs->seekg(*nextLink, ios::beg);
			//next link of hash table
			fs->read(buf8, 8); //reading nextLink value
			//name of directory or file
			fs->read(curName, MaxNameLength);
		} 
		//if directory or file not found in hash table, create it
		if (strcmp(curName, (*path)[i].c_str())) {
			File::mutexMap[fullPath].lock(); //file doesn't exist, no need to check for it being open
			File::mutexMap[realFileName].lock(); //only one thread can write to real file

			fs->seekg(0, ios::end);
			*nextLink = fs->tellg();
			fs->seekp(*prevLink, ios::beg);
			fs->write(buf8, 8);
			fs->seekp(*nextLink, ios::beg);
			fs->write(Zeroes, 8);
			fs->write((*path)[i].c_str(), (*path)[i].length());
			fs->write(Zeroes, BlockSize - 8 - (*path)[i].length());
			fs->flush();

			File::mutexMap[fullPath].unlock();
			File::mutexMap[realFileName].unlock();
		} else {//directory or file found
			
		}

	}
	//todo follow path

	if (File::mutexMap[fullPath].try_lock()) {
		fs->open(realFileName, fstream::out | fstream::in | fstream::binary);
		File* f = new File(fullPath, std::move(fs), File::write);
		return f;
	}
	else {
		return nullptr;
	}
}

size_t VFS::Read(File * f, char * buff, size_t len)
{
	return size_t();
}

size_t VFS::Write(File * f, char * buff, size_t len)
{
	return size_t();
}

void VFS::Close(File * f)
{
	delete f;
}

VFS::~VFS()
{
}
