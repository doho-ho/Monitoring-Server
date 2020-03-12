#pragma once

struct monitorData
{
	unsigned int	dataNameSize;
	std::string		dataName;
};

struct loginData
{
	short					clientType;
	unsigned char	dataSize;
	unsigned int		clientNameSize;
	std::string			clientName;
	monitorData		*Data;
};



class monitorServer : protected LanServer
{
private:
	char					IP[16];
	unsigned short	Port;
	unsigned int		maxClientCount;
	unsigned int		workerThreadCount;
	bool					nagleOpt;

	LONG	clientCounter;

	std::vector<serverSession*> Client;
	SRWLOCK	clientLock;

	bool	terminateFlag;

protected:
	resourceGetter *Getter;

private:
	void loadConfig(const char *_configData);

	void	clientLogin(const unsigned __int64 _Index, Sbuf *_buf);

	int		checkSameName(const std::string _Name);
	LONG setSessionData(const unsigned __int64 _Index, const loginData *_Data);

	
	loginData* packetDisassembleLogin(Sbuf *_buf);
	void	proc_dataSet(unsigned __int64 _Index, Sbuf *_buf);
	void	setMonitorData(serverSession *_Session, Sbuf *_buf);
	serverSession*	getSession(unsigned __int64 _Index);
	const std::string getNowTime();

	Sbuf* packetAssembleLogin(LONG _returnValue);
	
	void printMonitorServerData();
	void printTotalResourceData();
public:
	monitorServer(const char *_configData);
	~monitorServer();

	void printMonitoringData();
// Thread
	static unsigned __stdcall recvCheckThread(LPVOID _data);

protected:
// virtual function
	void onClientJoin(unsigned __int64 _Index);
	void onClientLeave(unsigned __int64 _Index);
	void onRecv(unsigned __int64 _Index, Sbuf *_buf);
	void onSend(unsigned __int64 _Index, int _sendSize);

	void onError(int _errorCode, WCHAR *_string);
};