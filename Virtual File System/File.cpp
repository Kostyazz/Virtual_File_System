#include "File.h"
#include <fstream>

using namespace std;
using namespace TestTask;

unordered_map<std::string, std::shared_mutex> TestTask::File::mutexMap;

TestTask::File::~File()
{
	if (openFor == read) {
		File::mutexMap[fullFilePath].unlock_shared();
	} else {
		File::mutexMap[fullFilePath].unlock();
	}
}
