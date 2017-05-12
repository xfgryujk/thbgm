
// ThbgmDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "thbgm.h"
#include "ThbgmDlg.h"
using namespace thbgm;
#include <thread>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CThbgmDlg 对话框



CThbgmDlg::CThbgmDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CThbgmDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

#pragma region MFC
void CThbgmDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT1, m_fmtFileEdit);
	DDX_Control(pDX, IDC_EDIT2, m_bgmFileEdit);
	DDX_Control(pDX, IDC_EDIT6, m_cmtFileEdit);
	DDX_Control(pDX, IDC_LIST1, m_bgmsList);
	DDX_Control(pDX, IDC_EDIT3, m_newFileEdit);
	DDX_Control(pDX, IDC_EDIT4, m_loopPointEdit);
	DDX_Control(pDX, IDC_CHECK1, m_loopPointSecCheck);
}

BEGIN_MESSAGE_MAP(CThbgmDlg, CDialog)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_EN_CHANGE(IDC_EDIT1, &CThbgmDlg::OnEnChangeEdit1)
	ON_BN_CLICKED(IDC_BUTTON1, &CThbgmDlg::OnBnClickedButton1)
	ON_BN_CLICKED(IDC_BUTTON2, &CThbgmDlg::OnBnClickedButton2)
	ON_BN_CLICKED(IDC_BUTTON5, &CThbgmDlg::OnBnClickedButton5)
	ON_BN_CLICKED(IDC_BUTTON3, &CThbgmDlg::OnBnClickedButton3)
	ON_LBN_SELCHANGE(IDC_LIST1, &CThbgmDlg::OnLbnSelchangeList1)
	ON_BN_CLICKED(IDC_BUTTON4, &CThbgmDlg::OnBnClickedButton4)
	ON_BN_CLICKED(IDC_CHECK1, &CThbgmDlg::OnBnClickedCheck1)
	ON_BN_CLICKED(IDC_BUTTON6, &CThbgmDlg::OnBnClickedButton6)
	ON_WM_DESTROY()
END_MESSAGE_MAP()


// CThbgmDlg 消息处理程序

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CThbgmDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CThbgmDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}
#pragma endregion

BOOL CThbgmDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	if (!thbgm::Init(GetSafeHwnd()))
	{
		AfxMessageBox(IDS_FAILED_TO_INIT_BASS, MB_ICONERROR);
		return FALSE;
	}

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CThbgmDlg::OnDestroy()
{
	CDialog::OnDestroy();

	thbgm::Uninit();
}


void CThbgmDlg::OnEnChangeEdit1()
{
	CString fmtFile;
	m_fmtFileEdit.GetWindowText(fmtFile);
	int pos = fmtFile.ReverseFind(_T('\\'));
	if (pos == -1)
		return;
	m_cmtFileEdit.SetWindowText(fmtFile.Left(pos + 1) + _T("musiccmt.txt"));
}

// Browse FMT file
void CThbgmDlg::OnBnClickedButton1()
{
	BrowseFile(m_fmtFileEdit, _T("fmt"), _T("FMT file (*.fmt;*.pos)|*.fmt;*.pos||"));
}

// Browse BGM file
void CThbgmDlg::OnBnClickedButton2()
{
	BrowseFile(m_bgmFileEdit, _T("dat"), _T("thbgm (*.dat;*.wav)|*.dat;*.wav||"));
}

// Browse CMT file
void CThbgmDlg::OnBnClickedButton5()
{
	BrowseFile(m_cmtFileEdit, _T("txt"), _T("musiccmt.txt (*.txt)|*.txt||"));
}

void CThbgmDlg::BrowseFile(CEdit& edit, LPCTSTR defaultExtension, LPCTSTR filter)
{
	CFileDialog dlg(TRUE, defaultExtension, NULL, 0, filter, this);
	if (dlg.DoModal() == IDOK)
		edit.SetWindowText(dlg.GetPathName());
}

// Parse
void CThbgmDlg::OnBnClickedButton3()
{
	m_lastSel = LB_ERR;
	m_bgmsList.ResetContent();

	CString fmtFile, bgmFile, cmtFile;
	m_fmtFileEdit.GetWindowText(fmtFile);
	m_bgmFileEdit.GetWindowText(bgmFile);
	m_cmtFileEdit.GetWindowText(cmtFile);
	m_thbgm = THBgm::Create((LPCWSTR)fmtFile, (LPCWSTR)bgmFile, (LPCWSTR)cmtFile);
	if (m_thbgm == nullptr || m_thbgm->m_bgms.empty())
	{
		m_thbgm = nullptr;
		AfxMessageBox(IDS_FAILED_TO_PARSE, MB_ICONERROR);
		return;
	}

	for (const auto& i : m_thbgm->m_bgms)
		m_bgmsList.AddString(i.displayName.c_str());
	m_bgmsList.SetCurSel(0);
	OnLbnSelchangeList1();
}


// Save and show BGM
void CThbgmDlg::OnLbnSelchangeList1()
{
	if (m_lastSel != LB_ERR && m_thbgm != nullptr)
		SaveBgm(m_thbgm->m_bgms[m_lastSel]);
	m_lastSel = m_bgmsList.GetCurSel();
	if (m_lastSel != LB_ERR && m_thbgm != nullptr)
		ShowBgm(m_thbgm->m_bgms[m_lastSel]);
}

void CThbgmDlg::ShowBgm(const Bgm& bgm)
{
	CString buffer;

	m_newFileEdit.SetWindowText(bgm.newFileName.c_str());

	m_loopPointSecCheck.SetCheck(FALSE);
	buffer.Format(_T("%u"), bgm.newLoopPoint);
	m_loopPointEdit.SetWindowText(buffer);
}

void CThbgmDlg::SaveBgm(Bgm& bgm)
{
	CString buffer;
	double dBuffer;
	
	m_newFileEdit.GetWindowText(buffer);
	bgm.newFileName = buffer;

	m_loopPointEdit.GetWindowText(buffer);
	dBuffer = _ttof(buffer);
	if (m_loopPointSecCheck.GetCheck())
		dBuffer *= 44100 * 2 * 2;
	bgm.newLoopPoint = (DWORD)dBuffer;
}

// Browse new file
void CThbgmDlg::OnBnClickedButton4()
{
	BrowseFile(m_newFileEdit, _T("mp3"), _T("audio files (*.mp3;*.ogg;*.wav)|*.mp3;*.ogg;*.wav|all files (*.*)|*.*||"));
}

// Convert the unit of loop point
void CThbgmDlg::OnBnClickedCheck1()
{
	CString buffer;
	m_loopPointEdit.GetWindowText(buffer);
	double dBuffer = _ttof(buffer);

	if (m_loopPointSecCheck.GetCheck())
		buffer.Format(_T("%lf"), dBuffer / (44100 * 2 * 2)); // Byte to second
	else 
		buffer.Format(_T("%u"), DWORD(dBuffer * (44100 * 2 * 2))); // Second to byte

	m_loopPointEdit.SetWindowText(buffer);
}

// Save
void CThbgmDlg::OnBnClickedButton6()
{
	if (m_thbgm == nullptr)
		return;
	int index = m_bgmsList.GetCurSel();
	if (index != LB_ERR)
		SaveBgm(m_thbgm->m_bgms[index]);

	BROWSEINFOW bi;
	ZeroMemory(&bi, sizeof(bi));
	bi.lpszTitle = _T("Output directory:");
	bi.ulFlags = BIF_STATUSTEXT;
	LPITEMIDLIST pidlSel = SHBrowseForFolderW(&bi);
	if (pidlSel == NULL)
		return;

	std::wstring outputDir(MAX_PATH, L'\0');
	SHGetPathFromIDListW(pidlSel, &outputDir.front());
	outputDir.resize(wcslen(outputDir.c_str()));

	EnableWindow(FALSE);
	std::thread([this, outputDir]{
		if (m_thbgm->Save(outputDir))
			AfxMessageBox(IDS_SUCCEEDED);
		else
			AfxMessageBox(IDS_FAILED_TO_SAVE, MB_ICONERROR);
		EnableWindow(TRUE);
	}).detach();
}
