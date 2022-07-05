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
#include"Packet.h"

class CCommand
{
public:
	CCommand();
	~CCommand() {};
	int  ExcuteCommand(int nCmd,std::list<CPacket>& lstPack,CPacket& inPacket);
	static void RunCommand(void* arg, int status,std::list<CPacket>&lstPacket, CPacket& inPacket)
	{
		CCommand* thiz = (CCommand*)arg;
		if (status > 0)
		{
			int ret = thiz->ExcuteCommand(status,lstPacket,inPacket);
			if ( ret!= 0)
			{
				
					TRACE("执行命令失败:%d  ret=%d\r\n", status,ret);
				
			}
		}
		else
		{
			MessageBox(NULL, _T("无法正常接入用户，自动重试！"), _T("接入用户失败！"), MB_OK | MB_ICONERROR);
		}
	}
protected:
	typedef int(CCommand::* CMDFUNC)(std::list<CPacket>&, CPacket&);
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

	int MakeDriverInfo(std::list<CPacket>& listPacket, CPacket& inPacket)
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
		listPacket.push_back(CPacket(1, (BYTE*)result.c_str(), result.size()));
	
		return 0;
	}
	int MakeDirectoryInfo(std::list<CPacket>& lstPacket, CPacket& inPacket)
	{
		//std::list<FILEINFO>listFileInfos;
		std::string strPath;
		strPath = inPacket.strData;
		
		if (_chdir(strPath.c_str()) != 0)
		{
			FILEINFO finfo;
			finfo.HasNext = FALSE;
			//listFileInfos.push_back(finfo);
			lstPacket.push_back(CPacket(2, (BYTE*)&finfo, sizeof(finfo)));
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
			lstPacket.push_back(CPacket(2, (BYTE*)&finfo, sizeof(finfo)));
			return -3;
		}
		do
		{
			FILEINFO finfo;
			finfo.IsDirectory = (fdata.attrib & _A_SUBDIR) != 0;
			memcpy(finfo.szFileName, fdata.name, strlen(fdata.name));
			//listFileInfos.push_back(finfo);
			lstPacket.push_back(CPacket(2, (BYTE*)&finfo, sizeof(finfo)));
			
		} while (!_findnext(hfind, &fdata));
		//发送信息到控制端
		FILEINFO finfo;
		finfo.HasNext = FALSE;
		TRACE("文件 %d \r\n", fdata.name);
		lstPacket.push_back(CPacket(2, (BYTE*)&finfo, sizeof(finfo)));
		return 0;
	}

	int RunFile(std::list<CPacket>& lstPacket, CPacket& inPacket)
	{
		std::string strPath;
		strPath = inPacket.strData;
		ShellExecuteA(NULL, NULL, strPath.c_str(), NULL, NULL, SW_SHOWNORMAL);
		lstPacket.push_back(CPacket(3, NULL, 0));
	
		return 0;
	}

	int DownloadFile(std::list<CPacket>& listPacket, CPacket& inPacket)
	{
		std::string strPath;
		strPath = inPacket.strData;
		long long data = 0;
		FILE* pFile = NULL;
		errno_t err = fopen_s(&pFile, strPath.c_str(), "rb");
		if (err != 0)
		{
			listPacket.push_back(CPacket(4, (BYTE*)&data, 8));
			
			return -1;
		}
		if (pFile != NULL)
		{
			fseek(pFile, 0, SEEK_END);
			data = _ftelli64(pFile);
			listPacket.push_back(CPacket(4, (BYTE*)&data, 8));
			
			fseek(pFile, 0, SEEK_SET);
			char buffer[1024] = "";
			size_t rlen = 0;
			do
			{
				rlen = fread(buffer, 1, 1024, pFile);
				listPacket.push_back(CPacket(4, (BYTE*)buffer, rlen));
				
			} while (rlen >= 1024);
			fclose(pFile);
		}
		//代码不一样，可能有问题
		listPacket.push_back(CPacket(4, NULL, 0));
		
		return 0;
	}

	int MouseEvent(std::list<CPacket>& lstPacket, CPacket& inPacket)
	{
		MOUSEEV mouse;
		memcpy(&mouse, inPacket.strData.c_str(), sizeof(MOUSEEV));
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
			lstPacket.push_back(CPacket(5, NULL, 0));
			
		
		
		return 0;
	}

	int SendScreen(std::list<CPacket>& listPacket,CPacket& inPacket)
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
			listPacket.push_back(CPacket(6, pData, nSize));
			
			GlobalUnlock(hMem);
		}
		pStream->Release();
		GlobalFree(hMem);
		screen.ReleaseDC();

		return 0;
	}



	int LockMachine(std::list<CPacket>& listPacket, CPacket& inPacket)
	{
		if ((dlg.m_hWnd == NULL) || (dlg.m_hWnd == INVALID_HANDLE_VALUE))
		{
			//_beginthread(threadLockDlg, 0, NULL);
			_beginthreadex(NULL, 0, &CCommand::threadLockDlg, this, 0, &threadid);
		}
		listPacket.push_back(CPacket(7, NULL, 0));
		
		return 0;
	}

	int UnLockMachine(std::list<CPacket>& listPacket, CPacket& inPacket)
	{
		PostThreadMessage(threadid, WM_KEYDOWN, 0x41, 0);
		listPacket.push_back(CPacket(8, NULL, 0));
		
		return 0;
	}

	int TestConnect(std::list<CPacket>& listPacket, CPacket& inPacket)
	{
		listPacket.push_back(CPacket(1981, NULL, 0));
		
		return 0;
	}

	int DeleteLocalFile(std::list<CPacket>& listPacket, CPacket& inPacket)
	{
		std::string strPath;
		strPath = inPacket.strData;
		TCHAR sPath[MAX_PATH] = _T("");
		//mbstowcs(sPath, strPath.c_str(), strPath.size());d中文容易乱码
		MultiByteToWideChar(CP_ACP, 0, strPath.c_str(), strPath.size(), sPath,
			sizeof(sPath) / sizeof(TCHAR));
		DeleteFile(sPath);
		listPacket.push_back(CPacket(9, NULL, 0));
		
		return 0;
	}

};

