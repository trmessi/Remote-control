﻿// RemoteCtrl.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include "framework.h"
#include "RemoteCtrl.h"
#include"ServerSocket.h"
#include<direct.h>
#include <atlimage.h>
#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// 唯一的应用程序对象

CWinApp theApp;

using namespace std;

void Dump(BYTE* pData, size_t nSize)
{
    std::string strOut;
    for (size_t i = 0; i < nSize; i++)
    {
        char buf[8] = "";
        if (i > 0 && (i % 16 == 0))strOut += "\n";
        snprintf(buf, sizeof(buf), "%02X ", pData[i] & 0xFF);
        strOut += buf;
    }
    strOut += "\n";
    OutputDebugStringA(strOut.c_str());
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
    Dump((BYTE*) pack.Data(), pack.Size());
    CServerSocket::getInstance()->Send(pack);
    return 0;
}

#include <io.h>
#include <list>


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
    if ((hfind= _findfirst("*", &fdata) )== -1)
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
    } while (!_findnext(hfind,&fdata));
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
    errno_t err=fopen_s(&pFile,strPath.c_str(), "rb");
    if (err!=0)
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
    BitBlt(screen.GetDC(), 0, 0, 1920, 1080, hScreen, 0, 0, SRCCOPY);
    ReleaseDC(NULL, hScreen);
    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, 0);
    if (hMem == NULL)return -1;
    IStream* pStream = NULL;
    HRESULT ret = CreateStreamOnHGlobal(hMem, TRUE, &pStream);
    if (ret == S_OK)
    {
        screen.Save(pStream, Gdiplus::ImageFormatPNG);
        LARGE_INTEGER bg={ 0 };
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

#include "LockDialog.h"
CLockDialog dlg;
unsigned threadid = 0;
unsigned _stdcall threadLockDlg(void* arg)
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
	dlg.SetWindowPos(&dlg.wndTopMost, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
	//限制鼠标活动范围
	ShowCursor(false);
	//隐藏任务栏
	::ShowWindow(FindWindow(_T("Shell_TrayWnd"), NULL), SW_HIDE);
	//dlg.GetWindowRect(rect);
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
	
	ShowCursor(true);
    ::ShowWindow(FindWindow(_T("Shell_TrayWnd"), NULL), SW_SHOW);
    dlg.DestroyWindow();
    _endthreadex(0);
    return 0;
}

int LockMachine()
{
    if ((dlg.m_hWnd == NULL) || (dlg.m_hWnd == INVALID_HANDLE_VALUE))
    {
        //_beginthread(threadLockDlg, 0, NULL);
        _beginthreadex(NULL, 0, threadLockDlg, NULL, 0, &threadid);
    }
	CPacket pack(7, NULL, 0);
	CServerSocket::getInstance()->Send(pack);
	return 0;
}

int UnLockMachine()
{
    PostThreadMessage(threadid, WM_KEYDOWN, 0x41, 0);
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
	bool ret= CServerSocket::getInstance()->Send(pack);
	return 0;
}

int ExcuteCommand(int nCmd)
{
    int ret = 0;
	switch (nCmd)
	{
	case 1://查看磁盘分区
        ret=MakeDriverInfo();
		break;
	case 2://查看指定目录下的文件
        ret = MakeDirectoryInfo();
		break;
	case 3://打开文件
        ret = RunFile();
		break;
	case 4://下载文件
        ret = DownloadFile();
		break;
	case 5:
        ret = MouseEvent();//鼠标操作
		break;
	case 6://发送屏幕内容==>发送屏幕截图
        ret = SendScreen();
		break;
	case 7://锁机
        ret = LockMachine();
		
		break;
	case 8://解锁
        ret = UnLockMachine();
		break;
    case 9://删除文件
        ret = DeleteLocalFile();
    case 1981:
        ret = TestConnect();
        break;
	}
    return ret;
}




int main()
{
    int nRetCode = 0;
    
    HMODULE hModule = ::GetModuleHandle(nullptr);

    if (hModule != nullptr)
    {
        // 初始化 MFC 并在失败时显示错误
        if (!AfxWinInit(hModule, nullptr, ::GetCommandLine(), 0))
        {
            // TODO: 在此处为应用程序的行为编写代码。
            wprintf(L"错误: MFC 初始化失败\n");
            nRetCode = 1;
        }
        else
        {
            // TODO: 在此处为应用程序的行为编写代码。
            CServerSocket* pserver= CServerSocket::getInstance();
            int count= 0;
            if (pserver->InitSocket() == false)
            {
                MessageBox(NULL, _T("网络初始化异常，未能成功初始化，请检查网络连接状态"),_T("网络初始化失败"), MB_OK | MB_ICONERROR);
                exit(0);
            }
            while (CServerSocket::getInstance() != NULL)
            {
                if (pserver->AcceptClient() == false)
                {
                    if (count > 3)
                    {
                        MessageBox(NULL, _T("多次无法正常接入用户，结束程序！"), _T("接入用户失败！"), MB_OK| MB_ICONERROR);
                        exit(0);
                    }
                    MessageBox(NULL, _T("无法正常接入用户，自动重试！"), _T("接入用户失败！"), MB_OK | MB_ICONERROR);
                    count++;
                }
                int ret = pserver->DealCommand();
                if (ret > 0)
                {
                    ret= ExcuteCommand(pserver->GetPacket().sCmd);
                    if (ret != 0)
                    {
                        TRACE("执行命令失败,5d ret=%d\r\n", pserver->GetPacket().sCmd, ret);
                    }
                    pserver->CloseClient();
                }
            }
        }
    }
    else
    {
        // TODO: 更改错误代码以符合需要
        wprintf(L"错误: GetModuleHandle 失败\n");
        nRetCode = 1;
    }

    return nRetCode;
}
