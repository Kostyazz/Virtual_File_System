#include "VFS.h"
#include "File.h"
#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <algorithm>

using namespace std;
using namespace TestTask;

const char Zeroes[VFS::BlockSize] = { 0 };
string delimiter{ "\\" };
unordered_map<std::string, std::shared_ptr<std::shared_mutex> > TestTask::VFS::mutexMap;

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
	return 8 + VFS::MaxNameLength + 8 * (hash<string>{}(s) % VFS::HashDivider);
}

File * VFS::openOrCreate(const char * fullPath, bool open) {
	mutexMap.try_emplace(string(fullPath), new shared_mutex);
	unique_ptr<vector<string> > path = parsePath(fullPath);
	string realFileName = path->front();
	unique_ptr<fstream> fs(new fstream());
	fs->open(realFileName, fstream::in | fstream::out | fstream::binary);

	char buf8[8]{ 0 };
	_int64* nextLink = static_cast<_int64*>((void*)buf8);
	unique_ptr<_int64> prevLink(new _int64(0));
	char curName[MaxNameLength]{ 0 };
	bool notFound = false;

	for (size_t i = 1; i < path->size(); i++) {
		*nextLink += getHashPos((*path)[i]);
		fs->seekp(*nextLink, ios::beg);

		mutexMap[realFileName]->lock_shared();

		*prevLink = *nextLink;
		fs->read(buf8, 8); //reading nextLink value
		//unwinding collisions
		while (*nextLink && strcmp(curName, (*path)[i].c_str())) {
			*prevLink = *nextLink;
			fs->seekp(*nextLink, ios::beg);
			//next link of hash table
			fs->read(buf8, 8); //reading nextLink value
			//name of directory or file
			fs->read(curName, MaxNameLength);
		}

		mutexMap[realFileName]->unlock_shared();

		//if directory or file not found in hash table, create it
		if (strcmp(curName, (*path)[i].c_str())) {
			if (open) {
				return nullptr;
				//throw runtime_error("ERROR: File not found");
			}

			mutexMap[realFileName]->lock(); //only one thread can write to real file

			fs->seekp(0, ios::end);
			*nextLink = fs->tellp();
			fs->seekp(*prevLink, ios::beg);
			fs->write(buf8, 8);
			fs->seekp(*nextLink, ios::beg);
			fs->write(Zeroes, 8);
			fs->write((*path)[i].c_str(), (*path)[i].length());
			fs->write(Zeroes, BlockSize - 8 - (*path)[i].length());
			fs->flush();

			mutexMap[realFileName]->unlock();

			if (i == path->size() - 1) {
				fs->seekp(*nextLink + 8 + MaxNameLength, ios::beg);
			}
		}
	}
	//stream position is set to read bytesInFile value
	fs->seekp(0, ios::cur);
	//curPos is position from where reading/writing will take place
	size_t curPos = size_t(fs->tellp()) + 8;

	if (open) {
		if (mutexMap[fullPath]->try_lock_shared()) {
			File* f = new File(fullPath, realFileName, std::move(fs), curPos, File::read);
			return f;
		} else {
			return nullptr;
		}
	} else {
		if (mutexMap[fullPath]->try_lock()) {
			File* f = new File(fullPath, realFileName, std::move(fs), curPos, File::write);
			return f;
		} else {
			return nullptr;
		}
	}
}

File * VFS::Open(const char * fullPath)
{
	return openOrCreate(fullPath, true);
}

File * VFS::Create(const char * fullPath)
{
	return openOrCreate(fullPath, false);
}

size_t VFS::Read(File * f, char * buff, size_t len)
{
	char buf8[8] = { 0 };
	_int64* nextLink = static_cast<_int64*>((void*)buf8);

	f->fs->clear();
	_int64 curPos = f->fs->tellp();
	size_t bytesRead = 0;
	size_t bytesFromBlock = 0;
	_int64 bytesInBlock = 0;
	bool stop = 0;
	//todo fix
	mutexMap[f->realFileName]->lock_shared();

	while (bytesRead < len && ! stop) {
		f->fs->seekp(curPos - curPos % BlockSize);
		f->fs->read(static_cast<char*>((void*)&bytesInBlock), 8);
		f->fs->seekp(curPos, ios::beg);
		bytesFromBlock = min(_int64(min(len - bytesRead, BlockSize - (curPos % BlockSize) - 16)), bytesInBlock - curPos);
		f->fs->read(buff + bytesRead, bytesFromBlock);
		bytesRead += bytesFromBlock;
		if (bytesFromBlock < BlockSize - (curPos % BlockSize) - 16) {
			stop = true;
		}
		if (bytesRead < len && ! stop) {
			f->fs->read(buf8, 8);
			if (*nextLink <= 0) {
				stop = true;
			} else {
				f->fs->seekp(*nextLink, ios::beg);
				curPos = f->fs->tellp();
			}
		}
	}

	mutexMap[f->realFileName]->unlock_shared();

	return bytesRead;
}

size_t VFS::Write(File * f, char * buff, size_t len)
{
	char buf8[8] = { 0 };
	_int64* intBuf = static_cast<_int64*>((void*)buf8);

	f->fs->clear();
	size_t curPos = f->curPos;
	size_t bytesWritten = 0;
	_int64 bytesToBlock = 0;

	mutexMap[f->realFileName]->lock();

	while (bytesWritten < len) {
		bytesToBlock = min(len - bytesWritten, BlockSize - (curPos % BlockSize) - 16);
		f->fs->read(buf8, 8);
		size_t bytesInBlock = *intBuf;
		f->fs->seekp(-8, ios::cur);
		*intBuf = bytesInBlock + bytesToBlock;
		f->fs->write(buf8, 8);
		f->fs->write(buff + bytesWritten, bytesToBlock);
		bytesWritten += bytesToBlock;
		curPos = size_t(f->fs->tellp());
		f->fs->seekp(0, ios::cur);
		f->fs->read(buf8, 8);
		if (*intBuf == 0) {
			f->fs->seekp(0, ios::end);
			*intBuf = f->fs->tellp();
			f->fs->seekp(curPos, ios::beg);
			f->fs->write(buf8, 8);
		}
		f->fs->write(Zeroes, 8);
		curPos = size_t(f->fs->tellp());
	}
	size_t zeroesNeeded = BlockSize - (curPos % BlockSize);
	f->fs->write(Zeroes, zeroesNeeded);
	f->fs->flush();
	f->fs->seekp(0 - zeroesNeeded, ios::cur);

	mutexMap[f->realFileName]->unlock();
	return bytesWritten;
}

void VFS::Close(File * f)
{
	if (f->openFor == File::read) {
		mutexMap[f->fullFilePath]->unlock_shared();
	}
	else {
		mutexMap[f->fullFilePath]->unlock();
	}
	delete f;
}

VFS::~VFS()
{
}
