#pragma once
#include "IVFS.h"
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
	};
}
