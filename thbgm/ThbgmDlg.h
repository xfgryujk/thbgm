
// ThbgmDlg.h : 头文件
//

#pragma once
#include "afxwin.h"
#include <libthbgm.h>


// CThbgmDlg 对话框
class CThbgmDlg : public CDialog
{
// 构造
public:
	CThbgmDlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
	enum { IDD = IDD_THBGM_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnDestroy();
	afx_msg void OnEnChangeEdit1();
	afx_msg void OnBnClickedButton1();
	afx_msg void OnBnClickedButton2();
	afx_msg void OnBnClickedButton5();
	afx_msg void OnBnClickedButton3();
	afx_msg void OnLbnSelchangeList1();
	afx_msg void OnBnClickedButton4();
	afx_msg void OnBnClickedCheck1();
	afx_msg void OnBnClickedButton6();

protected:
	void BrowseFile(CEdit& edit, LPCTSTR defaultExtension, LPCTSTR filter);
	void ShowBgm(const thbgm::Bgm& bgm);
	void SaveBgm(thbgm::Bgm& bgm);


public:
	CEdit m_fmtFileEdit;
	CEdit m_bgmFileEdit;
	CEdit m_cmtFileEdit;
	CListBox m_bgmsList;
	CEdit m_newFileEdit;
	CEdit m_loopPointEdit;
	CButton m_loopPointSecCheck;

protected:
	std::unique_ptr<thbgm::THBgm> m_thbgm;

	int m_lastSel = LB_ERR;
};
