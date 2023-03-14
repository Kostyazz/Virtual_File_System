#pragma once
#include "IVFS.h"
#include <unordered_map>
#include <shared_mutex>
#include <concurrent_unordered_map.h>

namespace TestTask {
	struct VFS : public IVFS
	{
	public:
		File* Open(const char* name) override;
		File* Create(const char* name) override;
		size_t Read(File* f, char* buff, size_t len) override;
		size_t Write(File* f, char* buff, size_t len) override;
		void Close(File* f) override;
		~VFS() override;
		static const size_t BlockSize = 1024;
		static const size_t MaxNameLength = 256; //including \0
		static const size_t HashDivider = (BlockSize - MaxNameLength - 8) / 8; //95
		static std::unordered_map<std::string, std::shared_mutex*> mutexMap;
	private:
		File * openOrCreate(const char * fullPath, bool open);
	};
}
