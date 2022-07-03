// CWatchDialog.cpp: 实现文件
//

#include "pch.h"
#include "RemoteClient.h"
#include "afxdialogex.h"
#include "CWatchDialog.h"
#include "RemoteClientDlg.h"

// CWatchDialog 对话框

IMPLEMENT_DYNAMIC(CWatchDialog, CDialog)

CWatchDialog::CWatchDialog(CWnd* pParent /*=nullptr*/)
	: CDialog(IDD_DLG_WATCH, pParent)
{

}

CWatchDialog::~CWatchDialog()
{
}

void CWatchDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_WATCH, m_picture);
}


BEGIN_MESSAGE_MAP(CWatchDialog, CDialog)
	ON_WM_TIMER()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_RBUTTONDBLCLK()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_STN_CLICKED(IDC_WATCH, &CWatchDialog::OnStnClickedWatch)
END_MESSAGE_MAP()


// CWatchDialog 消息处理程序


CPoint CWatchDialog::UserPoinToRemoteScreenPoint( CPoint& point,bool isScreen)
{
	CRect clientRect;
	if(isScreen)ScreenToClient(&point);
	
	m_picture.GetWindowRect(clientRect);
	return CPoint(point.x * m_nObjWidth/ clientRect.Width(), point.y * m_n_ObjHeight / clientRect.Height());
}

BOOL CWatchDialog::OnInitDialog()
{
	CDialog::OnInitDialog();

	// TODO:  在此添加额外的初始化
	SetTimer(0, 30, NULL);
	return TRUE;  // return TRUE unless you set the focus to a control
				  // 异常: OCX 属性页应返回 FALSE
}


void CWatchDialog::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	if (nIDEvent==0)
	{
		CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent();
		if (pParent->isFull())
		{
			CRect rect;
			m_picture.GetWindowRect(rect);
			if (m_nObjWidth == -1)
			{
				m_nObjWidth = pParent->getImage().GetWidth();
			}
			if (m_n_ObjHeight == -1)
			{
				m_n_ObjHeight = pParent->getImage().GetHeight();
			}
			pParent->getImage().StretchBlt(
				m_picture.GetDC()->GetSafeHdc(), 0, 0, rect.Width(), rect.Height(), SRCCOPY);
			m_picture.InvalidateRect(NULL);
			pParent->getImage().Destroy();
			pParent->SetImageStatus();
		}
	}
	CDialog::OnTimer(nIDEvent);
}


void CWatchDialog::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	if ((m_nObjWidth != -1) && (m_n_ObjHeight != -1))
	{
		CPoint remote = UserPoinToRemoteScreenPoint(point);//坐标转换
		MOUSEEV event;
		event.ptXY = remote;
		event.nButton = 0;
		event.nAction = 1;
		CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent();
		pParent->SendMessage(WM_SEND_PACKET, 5 << 1 | 1, (WPARAM) & event);
	}
	CDialog::OnLButtonDblClk(nFlags, point);
}


void CWatchDialog::OnLButtonDown(UINT nFlags, CPoint point)
{
	if ((m_nObjWidth != -1) && (m_n_ObjHeight != -1))
	{
		CPoint remote = UserPoinToRemoteScreenPoint(point);//坐标转换
		MOUSEEV event;
		event.ptXY = remote;
		event.nButton = 0;
		event.nAction = 2;

		CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent();
		pParent->SendMessage(WM_SEND_PACKET, 5 << 1 | 1, (WPARAM) & event);
	}
		CDialog::OnLButtonDown(nFlags, point);
}


void CWatchDialog::OnLButtonUp(UINT nFlags, CPoint point)
{
	//// TODO: 在此添加消息处理程序代码和/或调用默认值
	if ((m_nObjWidth != -1) && (m_n_ObjHeight != -1))
	{
		CPoint remote = UserPoinToRemoteScreenPoint(point);//坐标转换
		MOUSEEV event;
		event.ptXY = remote;
		event.nButton = 0;
		event.nAction = 3;
		CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent();
		pParent->SendMessage(WM_SEND_PACKET, 5 << 1 | 1, (WPARAM) & event);
	}
	CDialog::OnLButtonUp(nFlags, point);
}


void CWatchDialog::OnRButtonDblClk(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	if ((m_nObjWidth != -1) && (m_n_ObjHeight != -1))
	{
		CPoint remote = UserPoinToRemoteScreenPoint(point);//坐标转换
		MOUSEEV event;
		event.ptXY = remote;
		event.nButton = 1;
		event.nAction = 1;
		CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent();
		pParent->SendMessage(WM_SEND_PACKET, 5 << 1 | 1, (WPARAM) & event);
	}
	CDialog::OnRButtonDblClk(nFlags, point);
}


void CWatchDialog::OnRButtonDown(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	if ((m_nObjWidth != -1) && (m_n_ObjHeight != -1))
	{
		CPoint remote = UserPoinToRemoteScreenPoint(point);//坐标转换
		MOUSEEV event;
		event.ptXY = remote;
		event.nButton = 1;
		event.nAction = 2;
		CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent();
		pParent->SendMessage(WM_SEND_PACKET, 5 << 1 | 1, (WPARAM) & event);
	}
	CDialog::OnRButtonDown(nFlags, point);
}


void CWatchDialog::OnRButtonUp(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	if ((m_nObjWidth != -1) && (m_n_ObjHeight != -1))
	{
		CPoint remote = UserPoinToRemoteScreenPoint(point);//坐标转换
		MOUSEEV event;
		event.ptXY = remote;
		event.nButton = 1;
		event.nAction = 3;
		CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent();
		pParent->SendMessage(WM_SEND_PACKET, 5 << 1 | 1, (WPARAM) & event);
	}
	CDialog::OnRButtonUp(nFlags, point);
}


void CWatchDialog::OnMouseMove(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	if ((m_nObjWidth != -1) && (m_n_ObjHeight != -1))
	{
		CPoint remote = UserPoinToRemoteScreenPoint(point);//坐标转换
		MOUSEEV event;
		event.ptXY = remote;
		event.nButton = 8;
		event.nAction = 0;
		//CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent();
		//pParent->SendMessage(WM_SEND_PACKET, 5 << 1 | 1, (WPARAM) & event);
	}
	CDialog::OnMouseMove(nFlags, point);
}


void CWatchDialog::OnStnClickedWatch()
{
	if ((m_nObjWidth != -1) && (m_n_ObjHeight != -1))
	{
		// TODO: 在此添加控件通知处理程序代码
		CPoint point;
		GetCursorPos(&point);
		// TODO: 在此添加消息处理程序代码和/或调用默认值
		CPoint remote = UserPoinToRemoteScreenPoint(point, true);//坐标转换
		MOUSEEV event;
		event.ptXY = remote;
		event.nButton = 0;
		event.nAction = 0;
		CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent();
		pParent->SendMessage(WM_SEND_PACKET, 5 << 1 | 1, (WPARAM) & event);
	}
}


void CWatchDialog::OnOK()
{
	// TODO: 在此添加专用代码和/或调用基类

	//CDialog::OnOK();
}
