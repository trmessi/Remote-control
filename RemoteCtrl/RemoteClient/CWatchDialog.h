﻿#pragma once
#include "afxdialogex.h"


// CWatchDialog 对话框

class CWatchDialog : public CDialog
{
	DECLARE_DYNAMIC(CWatchDialog)

public:
	CWatchDialog(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CWatchDialog();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DLG_WATCH };
#endif
public:
	int m_nObjWidth;
	int m_nObjHeight;
	CImage m_image;
protected:
	
	
	DECLARE_MESSAGE_MAP()
public:
	bool m_isFull;//true表示有缓存数据
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持
	void SetImageStatus(bool isFull = false)
	{
		m_isFull = isFull;
	}
	bool isFull()const
	{
		return m_isFull;
	}
	CImage& getImage()
	{
		return m_image;
	}
	CPoint UserPoinToRemoteScreenPoint(CPoint& point,bool isScreen=false);
	virtual BOOL OnInitDialog();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	CStatic m_picture;
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnStnClickedWatch();
	virtual void OnOK();
	afx_msg void OnBnClickedBtnLock();
	afx_msg void OnBnClickedBtnUnlock();
};
