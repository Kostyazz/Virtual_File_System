#pragma once
#include "IVFS.h"
#include <unordered_map>
#include <shared_mutex>

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
	private:
		static std::unordered_map<std::string, std::shared_mutex> mutexMap;
		File * openOrCreate(const char * fullPath, bool open);
	};
}
