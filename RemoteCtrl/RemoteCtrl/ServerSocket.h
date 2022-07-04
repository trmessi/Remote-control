#pragma once
#include "pch.h"
#include "framework.h"
#include <list>
#include "Packet.h"





typedef void (*SOCKET_CALLBACK)(void* arg,int status,std::list<CPacket>&,CPacket&);

class CServerSocket
{
public:
	static CServerSocket* getInstance()
	{
		if (m_instance == NULL)//静态函数没有this指针，无法访问成员变量
		{
			m_instance = new CServerSocket();
		}
		return m_instance;
	}
	

	int Run(SOCKET_CALLBACK callback, void* arg,short prot=9527)
	{
		
		bool ret = InitSocket(prot);
		if (ret == false)return -1;
		std::list<CPacket>lstPackets;
		m_callback = callback;
		m_arg = arg;
		int count = 0;
		while (true)
		{
			if (AcceptClient() == false)
			{
				if (count >= 3)
				{
					return -2;
				}
				count++;
			}
			int ret1 = DealCommand();
			if (ret1 > 0)
			{
				m_callback(m_arg, ret1,lstPackets,m_packet);
				if (lstPackets.size() > 0)
				{
					Send(lstPackets.front());
					lstPackets.pop_front();
				}
			}
			CloseClient();
		}
		
	}
//protected:
	bool InitSocket(short port)
	{

		//校验
		if (m_sock == -1)
		{
			return false;
		}
		sockaddr_in serv_adr;
		memset(&serv_adr, 0, sizeof(serv_adr));
		serv_adr.sin_family = AF_INET;
		serv_adr.sin_addr.s_addr = INADDR_ANY;
		serv_adr.sin_port = htons(port);
		if (bind(m_sock, (sockaddr*)&serv_adr, sizeof(serv_adr)) == -1)
		{
			return false;//do
		}
		if (listen(m_sock, 1) == -1)
		{
			return false;
		}

		return true;
	}

	bool AcceptClient()
	{
		sockaddr_in client_addr;
		int cli_sz = sizeof(client_addr);
		m_client = accept(m_sock, (sockaddr*)&client_addr, &cli_sz);
		
		if (m_client == -1)
		{
			return false;
		}
		return true;
		
		
	}
#define BUFFER_SIZE 4096
	int DealCommand()
	{
		if (m_client == -1)
		{
			return -1;
		}
		//char buffer[1024] = "";
		char* buffer = new char[BUFFER_SIZE];//接收包的缓冲区
		if (buffer == NULL)
		{
			TRACE("内存不足\r\n");
			return -2;
		}
		memset(buffer, 0, BUFFER_SIZE);
		size_t index = 0;
		while (true)
		{
			size_t len = recv(m_client, buffer+index, BUFFER_SIZE -index, 0);
			if (len <= 0)
			{
				delete[]buffer;
				return -1;
			}
			index += len;
			len = index;
			m_packet= CPacket((BYTE*)buffer, len);
			if (len > 0)
			{
				memmove(buffer, buffer + len, BUFFER_SIZE -len);
				index -= len;
				delete[]buffer;
				return m_packet.sCmd;
			}
		}
		delete[]buffer;
		return -1;
	}
	bool Send(const char* pData, int nsize)
	{
		if (m_client == -1)
		{
			return false;
		}
		return send(m_client, pData, nsize, 0) > 0;
	}
	bool Send(CPacket& pack)
	{
		if (m_client == -1)return false;
		return send(m_client, pack.Data(), pack.Size(), 0) > 0;
	}
	bool GetFilePath(std::string& strPath)
	{
		if ((m_packet.sCmd == 2)||(m_packet.sCmd==3) || (m_packet.sCmd == 4)|| (m_packet.sCmd == 9))
		{
			strPath = m_packet.strData;
			return true;
		}
		return false;
	}
	bool GetMouseEvent( MOUSEEV& mouse)
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
	void CloseClient()
	{
		if (m_client != INVALID_SOCKET)
		{
			closesocket(m_client);
			m_client = INVALID_SOCKET;
		}
	}
private:
	SOCKET_CALLBACK m_callback;
	void* m_arg;
	SOCKET m_sock;
	SOCKET m_client;
	CPacket m_packet;
	CServerSocket() {
		m_client =INVALID_SOCKET;
		
		if (InitSockEnv() == FALSE)
		{
			MessageBox(NULL, _T("无法初始化套接字环境，请检测网络设置！"), _T("初始化错误！"), MB_OK | MB_ICONERROR);
			exit(0);
		}
		m_sock = socket(PF_INET, SOCK_STREAM, 0);
	}
	CServerSocket& operator=(const CServerSocket& ss) {}
	CServerSocket(const CServerSocket&ss) 
	{
		m_sock = ss.m_sock;
		m_client = ss.m_client;
	}
	BOOL InitSockEnv()
	{
		WSADATA data;
		if (WSAStartup(MAKEWORD(1, 1), &data) != 0)//返回值处理
		{
			return FALSE;
		}
		return TRUE;
	}
	~CServerSocket() {
		closesocket(m_sock);
		WSACleanup();
	}
	static void releaseInstance()
	{
		if (m_instance != NULL)
		{
			CServerSocket* tmp = m_instance;
			m_instance = NULL;
			delete tmp;
		}
	}
	static CServerSocket* m_instance;
	class CHelper
	{
	public:
		CHelper()
		{
			CServerSocket::getInstance();
		}
		~CHelper()
		{
			CServerSocket::releaseInstance();
		}
	};
	static CHelper m_helper;
};

extern CServerSocket server;
