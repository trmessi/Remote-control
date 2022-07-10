
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

void CClientSocket::threadEntry(void* arg)
{
	CClientSocket* thiz = (CClientSocket*)arg;
	thiz->threadFunc();
}

void CClientSocket::threadFunc()
{
	
	std::string strBuffer;
	strBuffer.resize(BUFFER_SIZE);
	char* pBuffer =(char*) strBuffer.c_str();
	int index = 0;
	InitSocket();//������
	while (m_sock!=INVALID_SOCKET)
	{
		
		if (m_lstSend.size() > 0)
		{
			m_lock.lock();
			CPacket& head = m_lstSend.front();
			m_lock.unlock();
			if (Send(head) == false)
			{
				
				TRACE("����ʧ��\r\n");
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
						m_mapAutoClosed.erase(it0);
						break;
					}
				} while (it0->second == false);
			}
			m_lock.lock();
			m_lstSend.pop_front();
			m_lock.unlock();
			if (InitSocket() == false)//ddadafwwawdwad
			{
				InitSocket();//������
			}
			
		}

		Sleep(1);
	}
	CloseSocket();
}
