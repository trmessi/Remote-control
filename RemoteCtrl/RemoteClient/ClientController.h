#pragma once
#include "ClientSocket.h"
#include "CWatchDialog.h"
#include "StatusDlg.h"
#include "RemoteClientDlg.h"
#include "Resource.h"
#include <map>
#include "TrTool.h"
#define WM_SEND_PACKET (WM_USER+1)//���Ͱ�����
#define WM_SEND_DATA (WM_USER+2)//��������
#define WM_SEND_STATUS (WM_USER+3)//չʾ״̬
#define  WM_SEND_WATCH (WM_USER+4)//Զ�̼��
#define  WM_SEND_MESSAGE (WM_USER+0x1000)
class CClientController
{
public:
	static CClientController* getInstance();
	//����
	int Invoke(CWnd*& pMainWnd);
	int InitController();
	LRESULT SendMessage(MSG msg);
	//���������������ַ
	void UpdateAddress(int nIp, int nPort)
	{
		CClientSocket::getInstance()->MyUpdateAddress(nIp, nPort);
	}
	int DealCommand()
	{
		return CClientSocket::getInstance()->DealCommand();
	}
	void CloseSocket()
	{
		CClientSocket::getInstance()->CloseSocket();
	}
	int SendPacket(const CPacket& pack)
	{
		CClientSocket* pClient = CClientSocket::getInstance();
		if (pClient->InitSocket() == false)return false;
		pClient->Send(pack);
	}
	//1.�鿴���̷���
	//2.�鿴ָ��Ŀ¼�µ��ļ�
	//3.���ļ�
	//4.�����ļ�
	// 5.������
	// 6.������Ļ����
	// 7.����
	// 8.����
	//9.ɾ���ļ�
	// 1981.��������
	//����ֵ������ţ�С��0�����
	int SendCommandPacket(int nCmd, bool bAutoClose = true, BYTE* pData = NULL, size_t nLength = 0);
	
	int GetImage(CImage& image)
	{
		CClientSocket* pClient = CClientSocket::getInstance();
		return CTrTool::Bytes2Image(image, pClient->GetPacket().strData);
		
	}
	int DownFile(CString strPath);

	void StrarWatchScreen();
protected:
	void threadDownloadFile();
	static void threadEntryDownload(void* arg);

	 void   threadWatchScreen();
	static void threadEntryWatchScreen(void* arg);
	CClientController():m_statusDlg(&m_remoteDlg),m_watchDlg(&m_remoteDlg)
	{
		m_hThread = INVALID_HANDLE_VALUE;
		m_hThreadDownload= INVALID_HANDLE_VALUE;
		m_hTreadWatch =INVALID_HANDLE_VALUE;
		m_nThreadID = -1;
		m_isClosed = true;
	}
	~CClientController()
	{
		WaitForSingleObject(m_hThread, 100);
	}
	void threadFunc();
	static unsigned __stdcall threadEntry(void* arg);
	static void releaseInstance()
	{
		if (m_instance != NULL)
		{
			delete m_instance;
			m_instance = NULL;
		}
	}
	LRESULT (OnSendPack)(UINT nMsg, WPARAM wParam, LPARAM lParam);
	LRESULT(OnSendData)(UINT nMsg, WPARAM wParam, LPARAM lParam);
	LRESULT(OnShowStatus)(UINT nMsg, WPARAM wParam, LPARAM lParam);
	LRESULT(OnShowWatcher)(UINT nMsg, WPARAM wParam, LPARAM lParam);
private:
	typedef struct MsgInfo
	{

		MSG msg;
		LRESULT result;
		
		MsgInfo(MSG m)
		{
			result = 0;
			memcpy(&msg, &m, sizeof(MSG));
			
		}
		MsgInfo()
		{
			result = 0;
			memset(&msg, 0, sizeof(MSG));
			
		}
		MsgInfo(const MsgInfo&m)
		{
			result = m.result;
			memcpy(&msg, &m.msg, sizeof(MSG));
			
		}
		MsgInfo& operator=(const MsgInfo& m)
		{
			if (this != &m)
			{
				result = m.result;
				memcpy(&msg, &m.msg, sizeof(MSG));
				
			}
			return *this;
		}
		
	}MSGINFO;
	typedef LRESULT(CClientController::* MSGFUNC)(UINT nMsg, WPARAM wParam, LPARAM lParam);
	static std::map<UINT, MSGFUNC> m_mapFunc;



	CWatchDialog m_watchDlg;
	CRemoteClientDlg m_remoteDlg;
	CStatusDlg m_statusDlg;
	HANDLE m_hThread;
	HANDLE m_hThreadDownload;
	HANDLE m_hTreadWatch;
	CString m_strRemote;//�����ļ�Զ��·��
	CString m_strLocal;//����·��
	bool m_isClosed;
	unsigned m_nThreadID;
	static CClientController* m_instance;
	class CHelper
	{
	public:
		CHelper()
		{
			//
		}
		~CHelper()
		{
			CClientController::releaseInstance();
		}
	};
	static CHelper m_helper;
};

