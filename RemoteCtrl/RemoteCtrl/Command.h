#pragma once
#include <map>
#include"TrTool.h"
#include<direct.h>
#include <atlimage.h>
#include <io.h>
#include <list>
#include "LockDialog.h"

#include "pch.h"
#include "framework.h"
#include "RemoteCtrl.h"
#include"ServerSocket.h"

class CCommand
{
public:
	CCommand();
	~CCommand() {};
	int  ExcuteCommand(int nCmd);
	
protected:
	typedef int(CCommand::* CMDFUNC)();
	std::map<int, CMDFUNC>m_mapFunction;

	CLockDialog dlg;
	unsigned threadid ;
protected:
	static unsigned _stdcall threadLockDlg(void* arg)
	{
		CCommand* thiz = (CCommand*)arg;
		_endthreadex(0);
		return 0;
	}

	void threadLockDlgMain()
	{
		dlg.Create(IDD_DIALOG_INFO, NULL);
		dlg.ShowWindow(SW_SHOW);
		//窗口置顶
		CRect rect;
		//窗口覆盖全屏
		rect.left = 0;
		rect.top = 0;
		rect.right = GetSystemMetrics(SM_CXFULLSCREEN);
		rect.bottom = GetSystemMetrics(SM_CYFULLSCREEN);
		rect.bottom = LONG(rect.bottom * 1.05);
		dlg.MoveWindow(rect);
		CWnd* pText = dlg.GetDlgItem(IDC_STATIC);
		if (pText)
		{
			CRect rtText;
			pText->GetWindowRect(rtText);
			int nWidth = rtText.Width() / 2;
			int x = (rect.right - nWidth) / 2;
			int nHeight = rtText.Height() / 2;
			int y = (rect.bottom - nHeight) / 2;
			pText->MoveWindow(x, y, rtText.Width(), rtText.Height());
		}
		dlg.SetWindowPos(&dlg.wndTopMost, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
		//限制鼠标活动范围
		ShowCursor(false);
		//隐藏任务栏
		::ShowWindow(FindWindow(_T("Shell_TrayWnd"), NULL), SW_HIDE);
		dlg.GetWindowRect(rect);
		//固定鼠标位置
		rect.left = 0;
		rect.top = 0;
		rect.right = 1;
		rect.bottom = 1;
		ClipCursor(rect);
		MSG msg;
		while (GetMessage(&msg, NULL, 0, 0))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			if (msg.message == WM_KEYDOWN)
			{
				TRACE("msg:%08X wparam:%80x lparam:%08x\r\n", msg.message, msg.wParam, msg.lParam);
				if (msg.wParam == 0x41)//按下a建 ，ESC（1B）退出
				{
					break;
				}
			}
		}
		ClipCursor(NULL);
		ShowCursor(true);
		::ShowWindow(::FindWindow(_T("Shell_TrayWnd"), NULL), SW_SHOW);
		dlg.DestroyWindow();
	}

	int MakeDriverInfo()
	{//1->A 2->B 3->C 
		std::string result;
		for (int i = 1; i <= 26; i++)
		{
			if (_chdrive(i) == 0)
			{
				if (result.size() > 0)
					result += ',';
				result += 'A' + i - 1;
			}

		}
		CPacket pack(1, (BYTE*)result.c_str(), result.size());
		CTrTool::Dump((BYTE*)pack.Data(), pack.Size());
		CServerSocket::getInstance()->Send(pack);
		return 0;
	}
	int MakeDirectoryInfo()
	{
		//std::list<FILEINFO>listFileInfos;
		std::string strPath;
		if (CServerSocket::getInstance()->GetFilePath(strPath) == false)
		{
			OutputDebugString(_T("当前命令不是获取文件列表,命令解析错误！！"));
			return -1;
		}
		if (_chdir(strPath.c_str()) != 0)
		{
			FILEINFO finfo;
			finfo.HasNext = FALSE;
			//listFileInfos.push_back(finfo);
			CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));
			CServerSocket::getInstance()->Send(pack);
			OutputDebugString(_T("没用权限，访问目录！"));
			return -2;
		}
		_finddata_t fdata;
		int hfind = 0;
		if ((hfind = _findfirst("*", &fdata)) == -1)
		{
			OutputDebugString(_T("没有找到任何文件！"));
			FILEINFO finfo;
			finfo.HasNext = FALSE;
			CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));
			CServerSocket::getInstance()->Send(pack);
			return -3;
		}
		do
		{
			FILEINFO finfo;
			finfo.IsDirectory = (fdata.attrib & _A_SUBDIR) != 0;
			memcpy(finfo.szFileName, fdata.name, strlen(fdata.name));
			//listFileInfos.push_back(finfo);
			CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));
			CServerSocket::getInstance()->Send(pack);
		} while (!_findnext(hfind, &fdata));
		//发送信息到控制端
		FILEINFO finfo;
		finfo.HasNext = FALSE;
		CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));
		CServerSocket::getInstance()->Send(pack);
		return 0;
	}

	int RunFile()
	{
		std::string strPath;
		CServerSocket::getInstance()->GetFilePath(strPath);
		ShellExecuteA(NULL, NULL, strPath.c_str(), NULL, NULL, SW_SHOWNORMAL);
		CPacket pack(3, NULL, 0);
		CServerSocket::getInstance()->Send(pack);
		return 0;
	}

	int DownloadFile()
	{
		std::string strPath;
		CServerSocket::getInstance()->GetFilePath(strPath);
		long long data = 0;
		FILE* pFile = NULL;
		errno_t err = fopen_s(&pFile, strPath.c_str(), "rb");
		if (err != 0)
		{
			CPacket pack(4, (BYTE*)&data, 8);
			CServerSocket::getInstance()->Send(pack);
			return -1;
		}
		if (pFile != NULL)
		{
			fseek(pFile, 0, SEEK_END);
			data = _ftelli64(pFile);
			CPacket head(4, (BYTE*)&data, 8);
			CServerSocket::getInstance()->Send(head);
			fseek(pFile, 0, SEEK_SET);
			char buffer[1024] = "";
			size_t rlen = 0;
			do
			{
				rlen = fread(buffer, 1, 1024, pFile);
				CPacket pack(4, (BYTE*)buffer, rlen);
				CServerSocket::getInstance()->Send(pack);
			} while (rlen >= 1024);
			fclose(pFile);
		}
		CPacket pack(4, NULL, 0);
		CServerSocket::getInstance()->Send(pack);
		return 0;
	}

	int MouseEvent()
	{
		MOUSEEV mouse;
		if (CServerSocket::getInstance()->GetMouseEvent(mouse))
		{

			DWORD nFlags = 0;

			switch (mouse.nButton)
			{
			case 0:
				nFlags = 1;//左键
				break;
			case 1:
				nFlags = 2;//右键
				break;
			case 2:
				nFlags = 4;//中键
				break;
			case 4:
				nFlags = 8;//没有按键
				break;

			}
			if (nFlags != 8)SetCursorPos(mouse.ptXY.x, mouse.ptXY.y);
			switch (mouse.nAction)
			{
			case 0://单击
				nFlags |= 0x10;
				break;
			case 1://双击
				nFlags |= 0x20;
				break;
			case 2://按下
				nFlags |= 0x40;
				break;
			case 3://放开
				nFlags |= 0x80;
				break;
			default:
				break;
			}
			switch (nFlags)
			{
			case 0x21://左键双击
				mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
				mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());

			case 0x11://左键单击
				mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
				mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
				break;
			case 0x41://左键按下
				mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
				break;
			case 0x81://左键放开
				mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
				break;
				break;
			case 0x22://右键双击
				mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
				mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
			case 0x12://右键单机
				mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
				mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
				break;
			case 0x42://右键按下
				mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
				break;
			case 0x82://右键放开
				mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
				break;
			case 0x24://中键双击
				mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
				mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
			case 0x14://中键单击
				mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
				mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
				break;
			case 0x44://中键按下
				mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
				break;
			case 0x84://中键放开
				mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
				break;
			case 0x08://单纯鼠标移动
				mouse_event(MOUSEEVENTF_MOVE, mouse.ptXY.x, mouse.ptXY.y, 0, GetMessageExtraInfo());
				break;
			}
			CPacket pack(4, NULL, 0);
			CServerSocket::getInstance()->Send(pack);
		}
		else
		{
			OutputDebugString(_T("获取鼠标操作参数失败！！"));
			return -1;
		}
		return 0;
	}

	int SendScreen()
	{
		CImage screen;
		HDC hScreen = ::GetDC(NULL);
		int nBitPerPixel = GetDeviceCaps(hScreen, BITSPIXEL);
		int nWidth = GetDeviceCaps(hScreen, HORZRES);
		int nHeight = GetDeviceCaps(hScreen, VERTRES);
		screen.Create(nWidth, nHeight, nBitPerPixel);
		BitBlt(screen.GetDC(), 0, 0, nWidth, nHeight, hScreen, 0, 0, SRCCOPY);
		ReleaseDC(NULL, hScreen);
		HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, 0);
		if (hMem == NULL)return -1;
		IStream* pStream = NULL;
		HRESULT ret = CreateStreamOnHGlobal(hMem, TRUE, &pStream);
		if (ret == S_OK)
		{
			screen.Save(pStream, Gdiplus::ImageFormatPNG);
			LARGE_INTEGER bg = { 0 };
			pStream->Seek(bg, STREAM_SEEK_SET, NULL);
			PBYTE pData = (PBYTE)GlobalLock(hMem);
			SIZE_T nSize = GlobalSize(hMem);
			CPacket pack(6, pData, nSize);
			CServerSocket::getInstance()->Send(pack);
			GlobalUnlock(hMem);
		}
		pStream->Release();
		GlobalFree(hMem);
		screen.ReleaseDC();

		return 0;
	}



	int LockMachine()
	{
		if ((dlg.m_hWnd == NULL) || (dlg.m_hWnd == INVALID_HANDLE_VALUE))
		{
			//_beginthread(threadLockDlg, 0, NULL);
			_beginthreadex(NULL, 0, &CCommand::threadLockDlg, this, 0, &threadid);
		}
		CPacket pack(7, NULL, 0);
		CServerSocket::getInstance()->Send(pack);
		return 0;
	}

	int UnLockMachine()
	{
		PostThreadMessage(threadid, WM_KEYDOWN, 0x41, 0);
		CPacket pack(8, NULL, 0);
		CServerSocket::getInstance()->Send(pack);
		return 0;
	}

	int TestConnect()
	{
		CPacket pack(1981, NULL, 0);
		CServerSocket::getInstance()->Send(pack);
		return 0;
	}

	int DeleteLocalFile()
	{
		std::string strPath;
		CServerSocket::getInstance()->GetFilePath(strPath);
		TCHAR sPath[MAX_PATH] = _T("");
		//mbstowcs(sPath, strPath.c_str(), strPath.size());d中文容易乱码
		MultiByteToWideChar(CP_ACP, 0, strPath.c_str(), strPath.size(), sPath,
			sizeof(sPath) / sizeof(TCHAR));
		DeleteFile(sPath);
		CPacket pack(9, NULL, 0);
		bool ret = CServerSocket::getInstance()->Send(pack);
		return 0;
	}

};

