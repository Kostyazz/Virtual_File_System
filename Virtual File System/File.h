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
		std::string realFileName;
		enum openType {read, write};
		openType openFor;

		File(std::string name, std::string _realFileName, std::unique_ptr<std::fstream> _fs, openType ot) : 
			fullFilePath(name), realFileName(_realFileName), fs(std::move(_fs)), openFor(ot) {};
	private:

	};
}