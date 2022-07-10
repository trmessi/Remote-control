
#include "pch.h"
#include "ClientSocket.h"
CClientSocket* CClientSocket::m_instance = NULL;
CClientSocket::CHelper CClientSocket::m_helper;
CClientSocket* pclient = CClientSocket::getInstance();
std::string GetMyErrInfo(int wsaErrCode)
{
	std::string ret;
	LPVOID lpMsgBuf = NULL;
	FormatMessage(
		FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
		NULL,
		wsaErrCode,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPSTR)&lpMsgBuf, 0, NULL
	);
	ret = (char*)lpMsgBuf;
	LocalFree(lpMsgBuf);
	return ret;
}

bool CClientSocket::SendPacket(HWND hWnd, const CPacket& pack, bool isAutoClosed,WPARAM wParam)
{
	if (m_hThread == INVALID_HANDLE_VALUE)
	{
		m_hThread = (HANDLE)_beginthreadex(NULL, 0, &CClientSocket::threadEntry, this, 0, &m_nThreadID);
	}
	UINT nMode = isAutoClosed ? CSM_AUTOCLOSE : 0;
	std::string strOut;
	pack.Data(strOut);
	return PostThreadMessage(m_nThreadID, WM_SEND_PACKET,(WPARAM) new PACKET_DATA(strOut.c_str(), strOut.size(),nMode,wParam), (LPARAM)hWnd);
	
}

void CClientSocket::SendPack(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	PACKET_DATA data = *(PACKET_DATA*)wParam;
	delete(PACKET_DATA*)wParam;
	HWND hwnd = (HWND)lParam;
	if (InitSocket() == true)
	{
		
		int ret = send(m_sock, (char*)data.strData.c_str(), (int)data.strData.size(), 0);
		if (ret>0)
		{
			size_t index = 0;
			std::string strBuffer;
			strBuffer.resize(BUFFER_SIZE);
			char* pBuffer = (char*)strBuffer.c_str();
			while (m_sock != INVALID_SOCKET)
			{


				int length = recv(m_sock,pBuffer+index ,BUFFER_SIZE-index , 0);
				if (length > 0||index>0)
				{
					index += (size_t)length;
					size_t nLen = index;
					CPacket pack((BYTE*)pBuffer, nLen);
					if (nLen > 0)
					{
						::SendMessage(hwnd, WM_SEND_ACK, (WPARAM)new CPacket(pack), data.wParam);
						if (data.nMode & CSM_AUTOCLOSE)
						{
							CloseSocket();
							return;
						}
					}
					index -= nLen;
					memmove(pBuffer, pBuffer + index, nLen);
				}
				else
				{//TODO:对方关闭套接字或网络异常
					CloseSocket();
					::SendMessage(hwnd, WM_SEND_ACK, NULL, 1);
				}
			}
			
		} 
		else
		{
			CloseSocket();
			::SendMessage(hwnd, WM_SEND_ACK, NULL, -1);
		}
	}
	else
	{
		::SendMessage(hwnd, WM_SEND_ACK, NULL, -2);
	}
}

unsigned CClientSocket::threadEntry(void* arg)
{
	CClientSocket* thiz = (CClientSocket*)arg;
	thiz->threadFunc2();
	_endthreadex(0);
	return 0;

}

/*
void CClientSocket::threadFunc()
{
	
	std::string strBuffer;
	strBuffer.resize(BUFFER_SIZE);
	char* pBuffer =(char*) strBuffer.c_str();
	int index = 0;
	InitSocket();//有问题
	while (m_sock!=INVALID_SOCKET)
	{
		
		if (m_lstSend.size() > 0)
		{
			m_lock.lock();
			CPacket& head = m_lstSend.front();
			m_lock.unlock();
			if (Send(head) == false)
			{
				
				TRACE("发送失败\r\n");
				continue;
			}
			std::map<HANDLE, std::list<CPacket>&>::iterator it;
			it = m_mapAck.find(head.hEvent);
			if (it != m_mapAck.end())
			{
				std::map<HANDLE, bool>::iterator it0 = m_mapAutoClosed.find(head.hEvent);
				do
				{
					int length = recv(m_sock, pBuffer + index, BUFFER_SIZE - index, 0);
					if ((length > 0) || (index > 0))
					{
						index += length;
						size_t size = (size_t)index;
						CPacket pack((BYTE*)pBuffer, size);
						if (size > 0)
						{
							pack.hEvent = head.hEvent;
							it->second.push_back(pack);

							memmove(pBuffer, pBuffer + size, index - size);
							index -= size;
							if (it0->second)
							{
								SetEvent(head.hEvent);
								break;
							}
						}
						continue;
					}
					else if (length <= 0 && index <= 0)
					{
						CloseSocket();
						SetEvent(head.hEvent);
						if (it0 != m_mapAutoClosed.end())
						{
							
						}
						else TRACE("异常退出");
						break;
					}
				} while (it0->second == false);
			}
			m_lock.lock();
			m_lstSend.pop_front();
			m_mapAutoClosed.erase(head.hEvent);
			m_lock.unlock();
			if (InitSocket() == false)//ddadafwwawdwad
			{
				InitSocket();//有问题
			}
			
		}

		Sleep(1);
	}
	CloseSocket();
}*/

void CClientSocket::threadFunc2()
{
	MSG msg;
	while (::GetMessage(&msg,NULL,0,0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
		if (m_mapFunc.find(msg.message) != m_mapFunc.end())
		{
			(this->*m_mapFunc[msg.message])(msg.message, msg.wParam, msg.lParam);
			
		}
	}
}
