#pragma once
#include <fstream>

class csvMaker
{
private:
	std::ofstream *Out;

private:
	bool loadFile(const std::string _fileName) {
		std::ofstream *File = new std::ofstream;
		File->open(_fileName.c_str());
		if (!File->is_open())
		{
			File->close();
			return false;
		}
		Out = File;
		return true;
	}

public:
	template<typename T> void Print(const T& t)
	{
		*Out << t << std::endl;
	}

	template<typename Name, typename... Arg>
	void Print(const Name& _Name, const Arg&... _Arg)
	{
		*Out << _Name << ',';
		Print(_Arg...);
	}
	template<typename T>
	void writeData(T& _Data)	{
		*Out << _Data << ',';
	}
	void newLine()	{
		*Out << std::endl;
	}
	bool newFile(const std::string _fileName) {
		return loadFile(_fileName);
	};
	void setTitle(const std::string _Title) { 
		*Out << _Title.c_str() << std::endl; 
	};
	void fileClose() { 
		delete Out;
	};
};