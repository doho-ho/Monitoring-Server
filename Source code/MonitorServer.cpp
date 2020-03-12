#include "stdafx.h"

monitorServer::monitorServer(const char *_configData)
{
	loadConfig(_configData);

	clientCounter = 0;
	terminateFlag = false;
	InitializeSRWLock(&clientLock);

	Getter = new resourceGetter();
	HANDLE handle = (HANDLE)_beginthreadex(NULL, 0, recvCheckThread, (LPVOID)this, 0, 0);
	Start(IP, Port, workerThreadCount, nagleOpt, maxClientCount);
	printf("Monitor server start!!\n");
}

monitorServer::~monitorServer()
{
	Stop();

	terminateFlag = true;
	serverSession *ss = nullptr;
	for (int i = 0; i < Client.size(); ++i)
	{
		ss = Client[i];
		delete ss;
	}
	Client.clear();
	Client.shrink_to_fit();

	delete Getter;
}

void monitorServer::loadConfig(const char *_configData)
{
	rapidjson::Document Doc;
	Doc.Parse(_configData);

	rapidjson::Value &Val = Doc["Monitor"];

	strcpy_s(IP, 16, Val["Server_ip"].GetString());
	Port = Val["Server_port"].GetUint();
	workerThreadCount = Val["Workerthread_count"].GetUint();
	maxClientCount = Val["Max_client_count"].GetUint();
	nagleOpt = Val["Nagle_option"].GetBool();
}

void monitorServer::printMonitorServerData()
{
	Getter->calProcessResourceValue();
	std::cout << "[Monitor server]" << std::endl;
	std::cout << "  [CPU] : " << Getter->getProcessCPU() <<"%";
	std::cout << "  [Memory] : " << Getter->getProcessMem() << "Mb";
	std::cout << std::endl;
	
}

void monitorServer::printMonitoringData()
{
	printTotalResourceData();
	printMonitorServerData();
	for (auto Iter : Client)
	{
		Iter->printData();
		Iter->initData();
	}
	std::cout << std::endl << std::endl;
}

void monitorServer::printTotalResourceData()
{
	Getter->calTotalResourceValue();
	std::cout << "[Total]" << std::endl;
	std::cout << "  [CPU] : " << Getter->getTotalCPU() << "%";
	std::cout << "  [Memory] : " << Getter->getTotalMem() << "Mb";
	std::cout << std::endl;
}

void monitorServer::clientLogin(const unsigned __int64 _Index, Sbuf *_buf)
{
	loginData* Data = packetDisassembleLogin(_buf);
	LONG returnValue = setSessionData(_Index, Data);
	Sbuf *buf = packetAssembleLogin(returnValue);
	SendPacket(_Index, buf);
	buf->Free();
	delete[] Data->Data;
	delete Data;
}

loginData* monitorServer::packetDisassembleLogin(Sbuf *_buf)
{
	// short					clientType
	//	unsigned char	dataSize
	// monitorData		Data[]

	loginData *Data = new loginData;

	*_buf >> Data->clientType;
	*_buf >> Data->clientNameSize;

	Data->clientName.insert(0,_buf->getFrontPtr(), Data->clientNameSize);
	_buf->moveFrontPos(Data->clientNameSize);

	*_buf >> Data->dataSize;
	if (Data->dataSize < 0) return nullptr;

	Data->Data = new monitorData[Data->dataSize];

	for (int i = 0; i < Data->dataSize; i++)
	{
		*_buf >> Data->Data[i].dataNameSize;
		Data->Data[i].dataName.insert(0,_buf->getFrontPtr(), Data->Data[i].dataNameSize);
		_buf->moveFrontPos(Data->Data[i].dataNameSize);
	}

	return Data;
}

void monitorServer::proc_dataSet(unsigned __int64 _Index, Sbuf *_buf)
{
	auto Client = getSession(_Index);
	if (!Client)
		return;
	setMonitorData(Client, _buf);
	const std::string Time = getNowTime();
	Client->writeTime(Time);
	Client->writeData();
	Client->writeNewLine();
}

const std::string monitorServer::getNowTime()
{
	SYSTEMTIME nowTime;
	GetLocalTime(&nowTime);
	std::string Time;
	Time = std::to_string(nowTime.wMonth) + '/' + std::to_string(nowTime.wDay) + ' ' + 
		std::to_string(nowTime.wHour) + ':' + std::to_string(nowTime.wMinute) + ':' + std::to_string(nowTime.wSecond);
	return Time;
}

serverSession* monitorServer::getSession(unsigned __int64 _Index)
{
	for (auto i : Client)
		if (i->getSessionIndex() == _Index)
			return i;
}

void monitorServer::setMonitorData(serverSession *_Session, Sbuf *_buf)
{
	//	unsigned char	authorizedClientCode
	//	unsigned char	dataSize
	//	ULONGLONG		Data[]

	unsigned char	authorizedClientCode = 0;
	unsigned char	dataSize = 0;
	ULONGLONG		Data = 0;

	*_buf >> authorizedClientCode;
	*_buf >> dataSize;

	if (authorizedClientCode > Client.size()) return;
	if (dataSize != Client[authorizedClientCode]->getDataSize()) return;

	for (int i = 0; i < dataSize; i++)
	{
		*_buf >> Data;
		Client[authorizedClientCode]->setData(i, Data);
	}	
}

int monitorServer::checkSameName(const std::string _Name)
{
	int Count = 0;
	for (auto &i : Client)
	{
		if (i->getName() == _Name)
			Count++;
	}
	return Count;
}

LONG monitorServer::setSessionData(const unsigned __int64 _Index, const loginData *_Data)
{
	if (!_Data) return -1;
	int i = 0;
	AcquireSRWLockExclusive(&clientLock);
	for (i ; i < Client.size(); i++)
	{
		if (Client[i]->getSessionIndex() == _Index)
			break;
	}
	if (i == Client.size()) return -1;
	auto &Session = Client[i];
	ReleaseSRWLockExclusive(&clientLock);
	
	int retVal = checkSameName(_Data->clientName);

	std::string clientName = _Data->clientName;
	if (retVal != 0)
		clientName += '#' + std::to_string(retVal);
			
	Session->setSessionName(clientName);
	Session->setSessionType((monitorClientType)_Data->clientType);
	for (int j = 0; j < _Data->dataSize; j++)
		Session->setDataName(j, _Data->Data[j].dataName);
	Session->setDataSize(_Data->dataSize);

	Session->writeTime("Time");
	Session->writeDataName();
	Session->writeNewLine();

	return i;
}

Sbuf* monitorServer::packetAssembleLogin(LONG _returnValue)
{
	//	short					protocolType
	//	unsigned char	loginResut
	//	unsigned char	authorizedClientCode
	//	unsigned char	authorizedDataSize

	Sbuf *buf = Sbuf::lanAlloc();
	*buf << (short)monitorProtocol::responseClientLogin;
	if (_returnValue >= 0)
	{
		*buf << (unsigned char)1;
		*buf << (unsigned char)_returnValue;
		*buf << Client[_returnValue]->getDataSize();
	}
	else
		*buf << (unsigned char)0;
	return buf;	
}

unsigned __stdcall monitorServer::recvCheckThread(LPVOID _data)
{
	monitorServer *Server = (monitorServer*)_data;

	ULONGLONG nowTime = 0;
	while (!Server->terminateFlag)
	{
		nowTime = GetTickCount64();
		for (auto &i : Server->Client)
		{
			if (nowTime - (i->getRecvTime()) > 1500)
				i->initData();
		}
		Sleep(999);
	}
	return 0;
}

void monitorServer::onClientJoin(unsigned __int64 _Index)
{
	serverSession *Session = new serverSession;
	Session->setSessionIndex(_Index);
	AcquireSRWLockExclusive(&clientLock);
	Client.push_back(Session);
	ReleaseSRWLockExclusive(&clientLock);
	InterlockedIncrement(&clientCounter);
}

void monitorServer::onClientLeave(unsigned __int64 _Index)
{
	AcquireSRWLockExclusive(&clientLock);
	for (std::vector<serverSession*>::iterator Iter = Client.begin() ; Iter != Client.end(); Iter++)
	{
		if ((*Iter)->getSessionIndex() == _Index)
		{
			serverSession *Session = (*Iter);
			delete Session;
			Client.erase(Iter);
			break;
		}
	}
	ReleaseSRWLockExclusive(&clientLock);
}

void monitorServer::onRecv(unsigned __int64 _Index, Sbuf *_buf)
{
	short protocolType;
	*_buf >> protocolType;

	switch (protocolType)
	{
	case requestClientLogin:
		clientLogin(_Index, _buf);
		break;
	case requestSetMonitorData:
		proc_dataSet(_Index, _buf);
		break;
	default:
		CCrashDump::Crash();
		break;
	}
}

void monitorServer::onSend(unsigned __int64 _Index, int _sendSize)
{

}

void monitorServer::onError(int _errorCode, WCHAR *_string)
{
	std::cout << _errorCode << _string << std::endl;
}