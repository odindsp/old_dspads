
// AppMonitorDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "AppMonitor.h"
#include "AppMonitorDlg.h"
#include "afxdialogex.h"
#include "url_endecode.h"
#include <string.h>
using namespace std;

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 对话框数据
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CAppMonitorDlg 对话框



CAppMonitorDlg::CAppMonitorDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CAppMonitorDlg::IDD, pParent)
	, m_imp_name(_T(""))
	, m_clk_name(_T(""))
	, m_appid(_T(""))
	, m_advid(_T(""))
	, m_mapid(_T(""))
	, m_curl(_T(""))
	, m_imp_url(_T(""))
	, m_clk_url(_T(""))
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CAppMonitorDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT_IMP_NAME, m_imp_name);
	DDX_Text(pDX, IDC_EDIT_CLK_NAME, m_clk_name);
	DDX_Text(pDX, IDC_EDIT_APPID2, m_appid);
	DDX_Text(pDX, IDC_EDIT_ADVID, m_advid);
	DDX_Text(pDX, IDC_EDIT_MAPID, m_mapid);
	DDX_Text(pDX, IDC_EDIT_CURL, m_curl);
	DDX_Text(pDX, IDC_EDIT_IMP, m_imp_url);
	DDX_Text(pDX, IDC_EDIT_CLK, m_clk_url);
}

BEGIN_MESSAGE_MAP(CAppMonitorDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON_CANCEL, &CAppMonitorDlg::OnBnClickedButtonCancel)
	ON_BN_CLICKED(IDC_BUTTON_OK, &CAppMonitorDlg::OnBnClickedButtonOk)
END_MESSAGE_MAP()


// CAppMonitorDlg 消息处理程序

BOOL CAppMonitorDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO:  在此添加额外的初始化代码
	m_imp_name = _T("http://ip.pxene.com/ic");
	m_clk_name = _T("http://cl.pxene.com/ic");
	UpdateData(FALSE);
	// 设置初始值

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CAppMonitorDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CAppMonitorDlg::OnPaint()
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
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CAppMonitorDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CAppMonitorDlg::OnBnClickedButtonCancel()
{
	m_advid = "";
	m_appid = "";
	m_mapid = "";
	m_curl = "";
	m_clk_url = "";
	m_imp_url = "";
	UpdateData(FALSE);
}


void CAppMonitorDlg::OnBnClickedButtonOk()
{
	UpdateData(TRUE);
	if (m_appid == "")
	{
		MessageBox(_T("请输入appid"));
		return;
	}

	if (m_advid == "")
	{
		MessageBox(_T("请输入advid"));
		return;
	}

	if (m_curl == "")
	{
		MessageBox(_T("请输入curl"));
		return;
	}

	if (m_mapid == "")
	{
		// create uuid to replace mapid
		char buffer[64] = { 0 };
		GUID guid;
		if (CoCreateGuid(&guid))
		{
			MessageBox(_T("create uuid error"));
			return;
		}
		_snprintf_s(buffer, sizeof(buffer),
			"%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
			guid.Data1, guid.Data2, guid.Data3,
			guid.Data4[0], guid.Data4[1], guid.Data4[2],
			guid.Data4[3], guid.Data4[4], guid.Data4[5],
			guid.Data4[6], guid.Data4[7]);
		m_mapid = buffer;
		MessageBox(m_mapid);
	}

	int new_len = 0;
//	char buffer[128];
////	memset(buffer, 0, 128);
//	strncpy(buffer, LPCTSTR(m_curl), m_curl.GetLength());
	int len = WideCharToMultiByte(CP_ACP, 0, m_curl, m_curl.GetLength(), NULL, 0, NULL, NULL);
	char * pFileName = new char[len + 1];
	WideCharToMultiByte(CP_ACP, 0, m_curl, m_curl.GetLength(), pFileName, len, NULL, NULL);
	pFileName[len] = '\0'; //多字节字符以'\0'结束

	char *encode = urlencode(pFileName, len, &new_len);
	/*for (int i = 0; i < new_len; i++)
	{
		char c = encode[i];
	
	}*/

	m_imp_url = m_imp_name + _T("?appsid=") + m_appid + _T("&adv=") + m_advid + _T("&mapid=") + m_mapid + _T("&mtype=m");
	m_clk_url = m_clk_name + _T("?appsid=") + m_appid + _T("&adv=") + m_advid + _T("&mapid=") + m_mapid + _T("&mtype=c") + _T("&curl=") + CString(encode);
	free(encode);
	UpdateData(FALSE);
	if (pFileName != NULL)
		delete [] pFileName;
}
