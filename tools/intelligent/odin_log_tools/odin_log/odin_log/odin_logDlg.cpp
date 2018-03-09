
// odin_logDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "odin_log.h"
#include "odin_logDlg.h"
#include "afxdialogex.h"
#include "errcode.h"
#include "util.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

TCHAR *redis_ip;
int redis_port;
TCHAR *sz_time[100] = { _T("Print_time"), _T("No print time") };
TCHAR *sz_err[100] = { _T("Err_creative"), _T("Err_groupid"), _T("Err") };
TCHAR *sz_adx[100] = { _T("Tanx"), _T("Amax"), _T("Inmobi"), _T("Iwifi"), _T("Letv"),
_T("Toutiao"), _T("Zplay"), _T("Baidu"), _T("Sohu"), _T("Adwalker"), _T("Iqiyi"), _T("Goyoo"),
_T("Iflytek"), _T("Adview"), _T("H2"), _T("google"), _T("baiduVideo"), _T("Meizu"), _T("16wifi"),
_T("Moji"), _T("Gdt"), _T("Lefee"), _T("Autohome"), _T("MoMo"), _T("homeTV") };

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


// Codin_logDlg 对话框



Codin_logDlg::Codin_logDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(Codin_logDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void Codin_logDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_IP, m_ip);
	DDX_Control(pDX, IDC_PORT, m_port);
	DDX_Control(pDX, IDC_CONNECT, m_connect);
	DDX_Control(pDX, IDC_DISCONNECT, m_disconnect);
	DDX_Control(pDX, IDC_STATUS, m_status);
	DDX_Control(pDX, IDC_ADX, m_adx);
	DDX_Control(pDX, IDC_ERR, m_err);
	DDX_Control(pDX, IDC_TIME, m_time);
	DDX_Control(pDX, IDOK, m_comfirm);
	DDX_Control(pDX, IDCANCEL, m_modify);
}

BEGIN_MESSAGE_MAP(Codin_logDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_CBN_SELCHANGE(IDC_ADX, &Codin_logDlg::OnCbnSelchangeAdx)
	ON_BN_CLICKED(IDOK, &Codin_logDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, &Codin_logDlg::OnBnClickedCancel)
	ON_BN_CLICKED(IDC_CONNECT, &Codin_logDlg::OnBnClickedConnect)
	ON_BN_CLICKED(IDC_DISCONNECT, &Codin_logDlg::OnBnClickedDisconnect)
END_MESSAGE_MAP()


// Codin_logDlg 消息处理程序

BOOL Codin_logDlg::OnInitDialog()
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
	int index = 0;
	CString r_ip, port;
	char szip[20] = "", szport[20] = "";

	IniReadStrByNameA(FILENAME_INI, SECTION_IP, "ip", szip);
	IniReadStrByNameA(FILENAME_INI, SECTION_PORT, "port", szport);

	if ((szip == "") || (szport == ""))
	{
		r_ip = _T("111.235.158.31");
		port = _T("7379");
	}
	else
	{
		r_ip = szip;
		port = szport;
	}

	m_ip.SetWindowText(r_ip);
	m_port.SetWindowText(port);

	for (index; index <= sizeof(sz_adx) / sizeof(char *); index++)
		m_adx.AddString(sz_adx[index]);

	m_err.AddString(_T("0"));
	m_err.AddString(_T("1"));
	m_err.AddString(_T("2"));

	m_time.AddString(_T("0"));
	m_time.AddString(_T("1"));

	m_adx.SetCurSel(0);
	m_err.SetCurSel(2);
	m_time.SetCurSel(0);

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void Codin_logDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else if ((nID & 0xFFF0) == SC_CLOSE)
	{
		//清除redis连接
		CString str_status;
		if (GetDlgItemText(IDC_STATUS, str_status))
		{
			if (str_status == _T("已连接"))
			{
				MessageBox(_T("请断开服务器连接."), _T("提示"));
				return;
			}
		}

		CDialogEx::OnCancel();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void Codin_logDlg::OnPaint()
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
HCURSOR Codin_logDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void Codin_logDlg::OnCbnSelchangeAdx()
{
	// TODO:  在此添加控件通知处理程序代码
	/* 选择adx的时候会自动读取该adx对应的错误日志等级和是否打印时间 */
	int errcode = E_SUCCESS;
	CString str_status;

	if (GetDlgItemText(IDC_STATUS, str_status))
	{
		if (str_status == _T("已连接"))
		{
			int adx = 0, err_level = 2, print_time = 0, 
				nSel = 0, flag = -1;
			CString str_adx, str_err, str_time;
			

			nSel = m_adx.GetCurSel();
			adx = nSel + 1;

			errcode = redis_operion(adx, flag, err_level, print_time);
			switch (errcode)
			{
			case E_REDIS_CONNECT_FAILD:
				MessageBox(_T("Redis连接失败!"), _T("提示"));
				break;
			case E_REDIS_EXECUTE_FAILED:
				MessageBox(_T("Redis执行失败!"), _T("提示"));
				break;
			default:
				break;
			}

			m_err.SetCurSel(err_level);
			m_time.SetCurSel(print_time);
		}
		else
			MessageBox(_T("请先连接服务器!"), _T("提示"));
	}
	else
		MessageBox(_T("非法错误!"), _T("提示"));
}

void Codin_logDlg::OnBnClickedOk()
{
	// TODO:  在此添加控件通知处理程序代码
	//CDialogEx::OnOK();
	/* 确定修改 */
	int errcode = E_SUCCESS;
	CString str_status;

	if (GetDlgItemText(IDC_STATUS, str_status))
	{
		if (str_status == _T("已连接"))
		{
			int adx = 0, err_level = 2, print_time = 0,
				nSel = 0, flag = 0;
			CString str_err, str_time;

			m_err.GetWindowText(str_err);
			m_time.GetWindowText(str_time);
			err_level = _wtoi(str_err.GetBuffer());
			print_time = _wtoi(str_time.GetBuffer());

			if ((err_level > 2) || (err_level < 0) || (print_time > 1) || (print_time < 0))
			{
				MessageBox(_T("日志等级或打印时间输入错误!"), _T("提示"));
				return;
			}

			nSel = m_adx.GetCurSel();
			adx = nSel + 1;
		/*	nSel = m_err.GetCurSel();
			err_level = nSel;

			nSel = m_time.GetCurSel();
			print_time = nSel;
			*/
			errcode = redis_operion(adx, flag, err_level, print_time);
			switch (errcode)
			{
			case E_REDIS_CONNECT_FAILD:
				MessageBox(_T("Redis连接失败!"), _T("提示"));
				break;
			case E_REDIS_EXECUTE_FAILED:
				MessageBox(_T("Redis执行失败!"), _T("提示"));
				break;
			case E_SUCCESS:
				MessageBox(_T("操作成功."), _T("提示"));
			default:
				break;
			}

			/*m_err.EnableWindow(FALSE);
			m_adx.EnableWindow(FALSE);
			m_time.EnableWindow(FALSE);
			m_comfirm.EnableWindow(FALSE);
			m_modify.EnableWindow(TRUE);*/
		}
		else
			MessageBox(_T("请先连接服务器!"), _T("提示"));
	}
	else
		MessageBox(_T("非法错误!"), _T("提示"));
}


void Codin_logDlg::OnBnClickedCancel()
{
	// TODO:  在此添加控件通知处理程序代码
	CDialogEx::OnCancel();
	/*m_err.EnableWindow(TRUE);
	m_adx.EnableWindow(TRUE);
	m_time.EnableWindow(TRUE);
	m_comfirm.EnableWindow(TRUE);
	m_modify.EnableWindow(FALSE);*/
}


void Codin_logDlg::OnBnClickedConnect()
{
	// TODO:  在此添加控件通知处理程序代码
	/* Redis连接按钮 */
	CString str_ip, str_port;
	int errcode = E_SUCCESS;
	char *ip_addr = NULL, *port_value = NULL;

	m_ip.GetWindowText(str_ip);
	m_port.GetWindowText(str_port);

	if (str_ip.GetLength() == 0)
	{
		MessageBox(_T("请先输入IP"), _T("提示"));
		return;
	}

	if (str_port.GetLength() == 0)
	{
		MessageBox(_T("请输入端口"), _T("提示"));
		return;
	}

	redis_ip = str_ip.GetBuffer();
	redis_port = _wtoi(str_port.GetBuffer());

	ip_addr = KS_UNICODE_to_UTF8(redis_ip);
	port_value = KS_UNICODE_to_UTF8(str_port.GetBuffer());
	IniWriteStrByNameA(FILENAME_INI, SECTION_IP, "ip", ip_addr);
	IniWriteStrByNameA(FILENAME_INI, SECTION_PORT, "port", port_value);
	errcode = redis_connecting();
	if (errcode == E_REDIS_CONNECT_FAILD)
		MessageBox(_T("服务器连接失败!"), _T("提示"));

	if (errcode == E_SUCCESS)
	{
		m_ip.EnableWindow(FALSE);
		m_port.EnableWindow(FALSE);
		m_connect.EnableWindow(FALSE);
		m_disconnect.EnableWindow(TRUE);
		GetDlgItem(IDC_STATUS)->SetWindowText(_T("已连接"));
	}
}


void Codin_logDlg::OnBnClickedDisconnect()
{
	// TODO:  在此添加控件通知处理程序代码
	/* Redis断开按钮 */
	int errcode = E_SUCCESS;

	errcode = redis_deconnect();
	if (errcode == E_SUCCESS)
	{
		m_ip.EnableWindow(TRUE);
		m_port.EnableWindow(TRUE);
		m_connect.EnableWindow(TRUE);
		m_disconnect.EnableWindow(FALSE);
		GetDlgItem(IDC_STATUS)->SetWindowText(_T("已断开"));
	}
}


void Codin_logDlg::OnBnClickedButton1()
{
	// TODO:  在此添加控件通知处理程序代码
}
