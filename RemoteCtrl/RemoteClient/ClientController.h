#pragma once
#include "ClientSocket.h"
#include "CWatchDialog.h"
#include "StatusDlg.h"
#include "RemoteClientDlg.h"
#include "Resource.h"
#include <map>
#define WM_SEND_PACKET (WM_USER+1)//发送包数据
#define WM_SEND_DATA (WM_USER+2)//发送数据
#define WM_SEND_STATUS (WM_USER+3)//展示状态
#define  WM_SEND_WATCH (WM_USER+4)//远程监控
#define  WM_SEND_MESSAGE (WM_USER+0x1000)
class CClientController
{
public:
	static CClientController* getInstance();
	//启动
	int Invoke(CWnd*& pMainWnd);
	int InitController();
	LRESULT SendMessage(MSG msg);
protected:
	CClientController():m_statusDlg(&m_remoteDlg),m_watchDlg(&m_remoteDlg)
	{
		m_hThread = INVALID_HANDLE_VALUE;
		m_nThreadID = -1;

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
	unsigned m_nThreadID;
	static CClientController* m_instance;
	class CHelper
	{
	public:
		CHelper()
		{
			CClientController::getInstance();
		}
		~CHelper()
		{
			CClientController::releaseInstance();
		}
	};
	static CHelper m_helper;
};

