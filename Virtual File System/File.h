#pragma once
#include <string>
#include <fstream>

namespace TestTask
{
	struct File 
	{
	public:
		std::unique_ptr<std::fstream> fs;
		std::string fullFilePath;
		enum openType {read, write};
		openType openFor;

		File(std::string name, std::unique_ptr<std::fstream> _fs, openType ot) : fullFilePath(name), fs(std::move(_fs)), openFor(ot) {};
		~File();
	private:

	};
}