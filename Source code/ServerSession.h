#pragma once

class serverSession
{
private:
	unsigned __int64		Index;
	std::string				Name;
	monitorClientType	Type;

	std::vector<ULONGLONG>	Data;
	std::vector<std::string>		dataName;

	ULONG			dataVectorCount;
	ULONGLONG	recentRecvTime;

	csvMaker		CSV;

public:
	serverSession()	{
		Index = 0;
		Type = monitorClientType::Null;
		dataVectorCount = 0;
		recentRecvTime = 0;
	}
	~serverSession() {
		CSV.fileClose();
	}
	void setSessionIndex(const unsigned __int64 _Index)	{
		Index = _Index;
	}
	void setSessionName(const std::string _Name)	{
		if (_Name.size() == 0)
			return;
		Name = _Name;
		std::string fileName = _Name + ".csv";
		CSV.newFile(fileName);
		CSV.Print(Name);
	}

	void setSessionType(const monitorClientType _Type)	{
		Type = _Type;
	}
	void setData(const unsigned char _Index, const ULONGLONG _Data)	{
		if (_Index > dataVectorCount) return;
		Data[_Index] = _Data;
	}
	void setDataSize(unsigned int _Size)	{
		Data.resize(_Size);
	}
	void setDataName(const unsigned char _Index, const std::string _Name)	{
		if (_Name.size() == 0) return;
		dataName.push_back(_Name);
		dataVectorCount++;
	}
	void writeDataName()	{
		for (auto &i : dataName)
			CSV.writeData(i);
	}
	void writeData()	{
		for (auto &i : Data)
			CSV.writeData(i);
	}
	void writeTime(const std::string _Time) {
		CSV.writeData(_Time);
	}
	void writeNewLine() {
		CSV.newLine();
	}
	void setRecvTime(const ULONGLONG _Time)	{
		recentRecvTime = _Time;
	}
	const ULONGLONG getRecvTime() {
		return recentRecvTime;
	}
	ULONG getDataSize()	{
		return dataVectorCount;
	}
	unsigned __int64 getSessionIndex()	{
		return Index;
	}
	void initData()	{
		for (int i = 0; i < dataVectorCount; i++)
			Data[i] = 0;
	}
	void printData()	{
		std::cout << "[" << Name.c_str() << "]==============================================" << std::endl;
		for (int Count = 0; Count < dataVectorCount; ++Count)
			std::cout << " [" << dataName[Count].c_str() << "] : " << Data[Count];
		std::cout << std::endl;
	}

	std::string getName() const	{
		return Name;
	}
};