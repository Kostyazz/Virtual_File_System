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

size_t const getHashPos(const string &s) {
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
		if (*nextLink == 0) {
			*nextLink = *prevLink;
		}
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

			//getting end of file position
			fs->seekp(0, ios::end);
			*nextLink = fs->tellp();
			//jumping to position where link to new block should be
			fs->seekp(*prevLink, ios::beg);
			fs->write(buf8, 8);
			//link for next collision is 0
			fs->seekp(*nextLink, ios::beg);
			fs->write(Zeroes, 8);
			//writing directory or file name and finishing block with zeroes
			fs->write((*path)[i].c_str(), (*path)[i].length());
			fs->write(Zeroes, BlockSize - 8 - (*path)[i].length());
			fs->flush();

			mutexMap[realFileName]->unlock();

			if (i == path->size() - 1) {
				fs->seekp(*nextLink + 8 + MaxNameLength, ios::beg);
			}
		}
		else {
			
		}
	}
	//stream position is set to read bytesInFile value
	fs->seekp(0, ios::cur);
	//curPos is position from where reading/writing will take place
	size_t curPos = size_t(fs->tellp()) + 8;

	//creating File object. nullptr if file already opened in another mode
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
	size_t curPos = f->curPos;
	size_t bytesRead = 0;
	size_t bytesFromBlock = 0;
	size_t bytesInBlock = 0;
	bool stop = 0;
	//todo fix
	mutexMap[f->realFileName]->lock_shared();

	while (bytesRead < len && ! stop) {
		//learn how many bytes of this block are part of the file
		f->fs->read(static_cast<char*>((void*)&bytesInBlock), 8);
		//jump to reading position
		f->fs->seekp(curPos, ios::beg);
		bytesFromBlock = min(len - bytesRead, BlockSize - curPos % BlockSize - 8);
		//if we're still in first block of the file (containing it's name)
		if (f->bytesBehind < BlockSize - MaxNameLength - 16) {
			bytesFromBlock = min(bytesFromBlock, 16 + MaxNameLength + bytesInBlock - curPos % BlockSize);
		} else {
			bytesFromBlock = min(bytesFromBlock, 8 + bytesInBlock - curPos % BlockSize);
		}
		//reading file data
		f->fs->read(buff + bytesRead, bytesFromBlock);
		bytesRead += bytesFromBlock;
		//if there's no more data, stop (even if we didn't reach 'len' value)
		if (bytesFromBlock < BlockSize - (curPos % BlockSize) - 8) {
			stop = true;
		}
		if (! stop) {
			//read next block position
			f->fs->read(buf8, 8);
			if (*nextLink <= 0) {
				stop = true;
			} else {
				f->fs->seekp(*nextLink, ios::beg);
				curPos = size_t(f->fs->tellp()) + 8;
			}
		} else {
			curPos = f->fs->tellp();
		}
	}

	mutexMap[f->realFileName]->unlock_shared();

	f->curPos = curPos;
	f->bytesBehind += bytesRead;
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
		//bytes we are going to write to this block
		bytesToBlock = min(len - bytesWritten, BlockSize - (curPos % BlockSize) - 8);
		//next 8 bytes tell how many bytes already written in this block (by previous calls of Write() )
		f->fs->read(buf8, 8);
		size_t bytesInBlock = *intBuf;
		//rewriting them with new value
		f->fs->seekp(-8, ios::cur);
		*intBuf = bytesInBlock + bytesToBlock;
		f->fs->write(buf8, 8);

		//writing actual file data
		f->fs->seekp(curPos, ios::beg);
		f->fs->write(buff + bytesWritten, bytesToBlock);
		bytesWritten += bytesToBlock;
		//if we still have data to write to next blocks...
		if (bytesWritten < len) {
			//saving current position (8 bytes before end of block)
			curPos = size_t(f->fs->tellp());
			f->fs->seekp(0, ios::cur);
			//apparently switching read and write doesn't work without flushing occasionally
			f->fs->flush();
			//reading next block position
			f->fs->read(buf8, 8);
			//if 0, we will write to end of file
			if (*intBuf == 0) {
				f->fs->seekp(0, ios::end);
				*intBuf = f->fs->tellp();
				//filling new block with zeroes
				f->fs->write(Zeroes, BlockSize);
				//getting back to previous block, rewriting it's link to the new one
				f->fs->seekp(curPos, ios::beg);
				f->fs->write(buf8, 8);
			}
			//now *intBuf has next block's position
			f->fs->seekp(*intBuf, ios::beg);
			//fs->tellp points to bytesInBlock value, while curPos points to where we'll write
			curPos = size_t(f->fs->tellp()) + 8;
		}
	}
	//if we finished writing, save curPos for possible next call of Write()
	curPos = size_t(f->fs->tellp());
	f->fs->flush();
	f->fs->seekp(curPos - (curPos % BlockSize), ios::beg);
	f->curPos = curPos;

	mutexMap[f->realFileName]->unlock();
	f->bytesBehind += bytesWritten;
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
