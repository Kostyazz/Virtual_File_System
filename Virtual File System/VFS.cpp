#include "VFS.h"
#include "File.h"
#include <vector>
#include <string>
#include <fstream>
#include <iostream>

using namespace std;
using namespace TestTask;

const string delimiter = "\\";

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

	fs->open(realFileName, fstream::in);
	for (size_t i = 0; i < path->size() - 1; i++) {
		char buf8[8];
		_int64* nextLink;
		char curName[256];
		bool notFound = false;
		do {
			//next link of hash table
			fs->read(buf8, 8);
			nextLink = static_cast<_int64*>((void*)buf8);
			//name of directory or file
			fs->read(curName, 256);
			//unwinding collisions list if didn't find current directory or file
			notFound = *nextLink && strcmp(curName, (*path)[i].c_str());
			if (notFound) {
				fs->seekg(*nextLink, ios::beg);
			}
		} while (notFound);
		//if directory or file not found in hash table, create it
		if (!*nextLink && !strcmp(curName, (*path)[i].c_str())) {
			//todo lock mutex and write
		}

	}
	//todo follow path

	if (File::mutexMap[fullPath].try_lock()) {
		fs->open(realFileName, fstream::out, fstream::app);
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
