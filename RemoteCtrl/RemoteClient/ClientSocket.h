#pragma once
#include "pch.h"
#include "framework.h"
#include <string>
#include<vector>
#include <map>
#include<list>
#include <mutex>
#define WM_SEND_PACKET (WM_USER+1)//发送包数据
#define WM_SEND_ACK (WM_USER+2)//发送包数据应答

#pragma pack(push)
#pragma pack(1)

class CPacket
{
public:
	WORD sHead;//包头。固定位 FEFF
	DWORD nLength;//包长度（从控制命令到和校验结束）
	WORD sCmd;//控制命令
	std::string strData;//包数据
	WORD sSum;//和校验
	
	//std::string strOut;//整个包的信息
	CPacket() {}
	CPacket(WORD nCmd, const BYTE* pData, size_t nSize)
	{
		sHead = 0xFEFF;
		nLength = nSize + 4;
		sCmd = nCmd;
		if (nSize > 0)
		{
			strData.resize(nSize);
			memcpy((void*)strData.c_str(), pData, nSize);
		}
		else
		{
			strData.clear();
		}
		sSum = 0;
		for (size_t j = 0; j < strData.size(); j++)
		{
			sSum += BYTE(strData[j]) & 0xFF;
		}
		
	}
	CPacket(const CPacket& mpack)
	{
		sHead = mpack.sHead;
		nLength = mpack.nLength;
		sCmd = mpack.sCmd;
		strData = mpack.strData;
		sSum = mpack.sSum;
	
	}
	CPacket(const BYTE* pData, size_t& nSize)
	{
		size_t i = 0;
		for (; i < nSize; i++)//可能有问题
		{
			if (*(WORD*)(pData + i) == 0xFEFF)
			{
				sHead = *(WORD*)(pData + i);
				i += 2;
				break;
			}
		}
		if (i + 4 + 2 + 2 > nSize)//包数据可能不全，或者包头未能全部接收到
		{
			nSize = 0;
			return;
		}
		nLength = *(DWORD*)(pData + i); i += 4;
		if (nLength + i > nSize)//包未完全接收到，就返回，解析失败
		{
			nSize = 0;
			return;
		}
		sCmd = *(WORD*)(pData + i); i += 2;
		if (nLength > 4)
		{
			strData.resize(nLength - 2 - 2);
			memcpy((void*)strData.c_str(), pData + i, nLength - 4);
			i += nLength - 4;
		}
		sSum = *(WORD*)(pData + i); i += 2;
		WORD sum = 0;
		for (size_t j = 0; j < strData.size(); j++)
		{
			sum += BYTE(strData[j]) & 0xFF;
		}
		if (sum == sSum)
		{
			nSize = i;//head 2,length 4,cmd+data+sum nLength
			return;
		}
		nSize = 0;
	}
	~CPacket() {}
	CPacket& operator =(const CPacket& mpack)
	{
		if (this != &mpack)
		{
			sHead = mpack.sHead;
			nLength = mpack.nLength;
			sCmd = mpack.sCmd;
			strData = mpack.strData;
			sSum = mpack.sSum;
			
		}
		return *this;
	}
	int Size()
	{
		return nLength + 2 + 4;
	}
	const char* Data(std::string& strOut)const
	{
		strOut.resize(nLength + 6);
		BYTE* pData = (BYTE*)strOut.c_str();
		*(WORD*)pData = sHead; pData += 2;
		*(DWORD*)pData = nLength; pData += 4;
		*(WORD*)pData = sCmd; pData += 2;
		memcpy(pData, strData.c_str(), strData.size()); pData += strData.size();
		*(WORD*)pData = sSum;
		return strOut.c_str();
	}
};
#pragma pack(pop)

typedef struct MouseEvent
{
	MouseEvent()
	{
		nAction = 0;
		nButton = -1;
		ptXY.x = 0;
		ptXY.y = 0;
	}
	WORD nAction;//单击，双击，移动
	WORD nButton;//左键，右键，中键
	POINT ptXY;//坐标
}MOUSEEV, * PMOUSSEV;

typedef struct  file_info
{
	file_info()
	{
		IsInvalid = FALSE;
		IsDirectory = -1;
		HasNext = TRUE;
		memset(szFileName, 0, sizeof(szFileName));
	}
	BOOL IsInvalid;//是否有效
	BOOL IsDirectory;//是否为目录  0否 1是
	BOOL HasNext;//是否有后续  0没有 有
	char szFileName[256];//文件名


} FILEINFO, * PFILEINFO;

enum
{
	CSM_AUTOCLOSE = 1,//自动关闭
};

typedef struct PackData
{
	std::string strData;
	UINT nMode;
	WPARAM wParam;
	PackData(const char* pData, size_t nLen, UINT mode,WPARAM nParam=0)
	{
		strData.resize(nLen);
		memcpy((char*)strData.c_str(), pData, nLen);
		nMode = mode;
		wParam = nParam;
	}
	PackData(const PackData& data)
	{
		strData = data.strData;
		nMode = data.nMode;
		wParam = data.wParam;
	}
	PackData& operator =(const PackData& data)
	{
		if (this != &data)
		{
			strData = data.strData;
			nMode = data.nMode;
			wParam = data.wParam;
		}
		return *this;
	}
	
}PACKET_DATA;

std::string GetMyErrInfo(int wsaErrCode);


class CClientSocket
{
public:
	static CClientSocket* getInstance()
	{
		if (m_instance == NULL)//静态函数没有this指针，无法访问成员变量
		{
			m_instance = new CClientSocket();
		}
		return m_instance;
	}
	bool InitSocket()
	{
		if (m_sock != INVALID_SOCKET)CloseSocket();
		m_sock = socket(PF_INET, SOCK_STREAM, 0);
		//校验
		if (m_sock == -1)
		{
			return false;
		}
		sockaddr_in serv_adr;
		memset(&serv_adr, 0, sizeof(serv_adr));
		serv_adr.sin_family = AF_INET;
		serv_adr.sin_addr.s_addr = htonl(m_nIp);
		serv_adr.sin_port = htons(m_nPort);
		if (serv_adr.sin_addr.s_addr == INADDR_NONE)
		{
			AfxMessageBox("指定的IP地址不存在");
			return false;
		}
		int ret=connect(m_sock, (sockaddr*)&serv_adr, sizeof(serv_adr));
		if (ret == -1)
		{
			AfxMessageBox("连接失败");
			TRACE("连接失败，%d %s\r\n", WSAGetLastError(), GetMyErrInfo(WSAGetLastError()).c_str());
		}
		return true;
	}

#define BUFFER_SIZE 40960000
	int DealCommand()
	{
		if (m_sock == -1)
		{
			return -1;
		}
		//char buffer[1024] = "";
		char* buffer =m_buffer.data();//接收包的缓冲区 TODO:多线程发送命令时冲突，使用了同一个buffer
		
		static size_t index = 0;
		while (true)
		{
			size_t len = recv(m_sock, buffer + index, BUFFER_SIZE - index, 0);
			if (((int)len <= 0)&&((int)index==0))
			{
				return -1;
			}
			index += len;
			len = index;
			m_packet = CPacket((BYTE*)buffer, len);
			if (len > 0)
			{
				memmove(buffer, buffer + len, index - len);
				index -= len;
				return m_packet.sCmd;
			}
		}
		return -1;
	}
	

	bool SendPacket(HWND hWnd, const CPacket& pack, bool isAutoClosed = true,WPARAM nWaram=0);

	/*bool SendPacket(const CPacket& pack,std::list<CPacket>&lstPacks,bool isAutoClosed=true)
	{
		if (m_sock == INVALID_SOCKET&&m_hThread==INVALID_HANDLE_VALUE)
		{
			//if (InitSocket() == false)return false;
			m_hThread=(HANDLE)_beginthread(&CClientSocket::threadEntry, 0, this);
		}

		m_lock.lock();
		auto pr = m_mapAck.insert(std::pair<HANDLE, std::list<CPacket>&>(pack.hEvent, lstPacks));
		m_mapAutoClosed.insert(std::pair<HANDLE, bool>(pack.hEvent, isAutoClosed));
		
		m_lstSend.push_back(pack);
		m_lock.unlock();
		WaitForSingleObject(pack.hEvent, INFINITE);
		std::map<HANDLE, std::list<CPacket>&>::iterator it;
		it= m_mapAck.find(pack.hEvent);
		if (it != m_mapAck.end())
		{
			m_lock.lock();
			m_mapAck.erase(it);
			m_lock.unlock();
			return true;
		}
		return false;
	}*/

	bool GetFilePath(std::string& strPath)
	{
		if ((m_packet.sCmd == 2) || (m_packet.sCmd == 3) || (m_packet.sCmd == 4))
		{
			strPath = m_packet.strData;
			return true;
		}
		return false;
	}
	bool GetMouseEvent(MOUSEEV& mouse)
	{
		if (m_packet.sCmd == 5)
		{
			memcpy(&mouse, m_packet.strData.c_str(), sizeof(MOUSEEV));
			return true;
		}
		return false;
	}
	CPacket& GetPacket()
	{
		return m_packet;
	}
	void CloseSocket()
	{
		closesocket(m_sock);
		m_sock = INVALID_SOCKET;
	}
	void MyUpdateAddress(int nIp, int nPort)
	{
		if ((m_nIp != nIp) || (m_nPort != nPort))
		{

			m_nIp = nIp;
			m_nPort = nPort;
		
		}
	}

private:
	UINT m_nThreadID;
	HANDLE m_hThread;
	std::mutex m_lock;
	bool m_bAutoClosed;
	std::list<CPacket>m_lstSend;
	std::map<HANDLE, std::list<CPacket>&> m_mapAck;
	std::map<HANDLE, bool>m_mapAutoClosed;
	int m_nIp;//IP地址
	int m_nPort;//端口
	std::vector<char> m_buffer;
	SOCKET m_sock;
	CPacket m_packet;

	typedef void(CClientSocket::* MSGFUNC)(UINT nMsg, WPARAM wParam, LPARAM lParam);
	std::map<UINT, MSGFUNC>m_mapFunc;

	bool Send(const char* pData, int nsize)
	{
		if (m_sock == -1)
		{
			return false;
		}
		return send(m_sock, pData, nsize, 0) > 0;
	}
	bool Send(const CPacket& pack)
	{
		if (m_sock == -1)return false;
		std::string strOut;
		pack.Data(strOut);

		return send(m_sock, strOut.c_str(), strOut.size(), 0) > 0;
	}

	void SendPack(UINT nMsg, WPARAM wParam, LPARAM lParam);

	CClientSocket() :
		m_nIp(INADDR_ANY), m_nPort(0),m_sock(INVALID_SOCKET),m_bAutoClosed(true),m_hThread(INVALID_HANDLE_VALUE)
	{

		if (InitSockEnv() == FALSE)
		{
			MessageBox(NULL, _T("无法初始化套接字环境，请检测网络设置！"), _T("初始化错误！"), MB_OK | MB_ICONERROR);
			exit(0);
		}
		m_buffer.resize(BUFFER_SIZE);
		memset(m_buffer.data(), 0, BUFFER_SIZE);
		struct
		{
			UINT message;
			MSGFUNC func;
		}funcs[] = {
			{WM_SEND_PACKET,&CClientSocket::SendPack},
			{0,NULL}
		};
		for (int i = 0; funcs[i].message != 0; i++)
		{
			if (m_mapFunc.insert(std::pair<UINT, MSGFUNC>(funcs[i].message, funcs[i].func)).second == false)
			{
				TRACE("插入失败");
			}
		}
	}
	CClientSocket& operator=(const CClientSocket& ss){}
	CClientSocket(const CClientSocket& ss)
	{
		m_hThread = INVALID_HANDLE_VALUE;
		m_bAutoClosed = ss.m_bAutoClosed;
		m_nIp = ss.m_nIp;
		m_nPort = ss.m_nPort;
		m_sock = ss.m_sock;
		std::map<UINT, CClientSocket::MSGFUNC>::const_iterator it = ss.m_mapFunc.begin();
		for (; it != ss.m_mapFunc.end(); it++)
		{
			m_mapFunc.insert(std::pair<UINT, MSGFUNC>(it->first, it->second));
		}
	}
	static unsigned __stdcall threadEntry(void* arg);
	//void threadFunc();
	void threadFunc2();
	BOOL InitSockEnv()
	{
		WSADATA data;
		if (WSAStartup(MAKEWORD(1, 1), &data) != 0)//返回值处理
		{
			return FALSE;
		}
		return TRUE;
	}
	~CClientSocket() {
		closesocket(m_sock);
		m_sock = INVALID_SOCKET;
		WSACleanup();
	}
	static void releaseInstance()
	{
		if (m_instance != NULL)
		{
			CClientSocket* tmp = m_instance;
			m_instance = NULL;
			delete tmp;
		}
	}
	static CClientSocket* m_instance;
	class CHelper
	{
	public:
		CHelper()
		{
			CClientSocket::getInstance();
		}
		~CHelper()
		{
			CClientSocket::releaseInstance();
		}
	};
	static CHelper m_helper;
};

extern CClientSocket server;
