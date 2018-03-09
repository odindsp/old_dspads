
// AppMonitorDlg.cpp : ʵ���ļ�
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


// ����Ӧ�ó��򡰹��ڡ��˵���� CAboutDlg �Ի���

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// �Ի�������
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��

// ʵ��
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


// CAppMonitorDlg �Ի���



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


// CAppMonitorDlg ��Ϣ�������

BOOL CAppMonitorDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// ��������...���˵�����ӵ�ϵͳ�˵��С�

	// IDM_ABOUTBOX ������ϵͳ���Χ�ڡ�
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

	// ���ô˶Ի����ͼ�ꡣ  ��Ӧ�ó��������ڲ��ǶԻ���ʱ����ܽ��Զ�
	//  ִ�д˲���
	SetIcon(m_hIcon, TRUE);			// ���ô�ͼ��
	SetIcon(m_hIcon, FALSE);		// ����Сͼ��

	// TODO:  �ڴ���Ӷ���ĳ�ʼ������
	m_imp_name = _T("http://ip.pxene.com/ic");
	m_clk_name = _T("http://cl.pxene.com/ic");
	UpdateData(FALSE);
	// ���ó�ʼֵ

	return TRUE;  // ���ǽ��������õ��ؼ������򷵻� TRUE
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

// �����Ի��������С����ť������Ҫ����Ĵ���
//  �����Ƹ�ͼ�ꡣ  ����ʹ���ĵ�/��ͼģ�͵� MFC Ӧ�ó���
//  �⽫�ɿ���Զ���ɡ�

void CAppMonitorDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // ���ڻ��Ƶ��豸������

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// ʹͼ���ڹ����������о���
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// ����ͼ��
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//���û��϶���С������ʱϵͳ���ô˺���ȡ�ù��
//��ʾ��
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
		MessageBox(_T("������appid"));
		return;
	}

	if (m_advid == "")
	{
		MessageBox(_T("������advid"));
		return;
	}

	if (m_curl == "")
	{
		MessageBox(_T("������curl"));
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
	pFileName[len] = '\0'; //���ֽ��ַ���'\0'����

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
