
// intelligent_priceDlg.cpp : ʵ���ļ�
#include "stdafx.h"
#include "intelligent_price.h"
#include "intelligent_priceDlg.h"
#include "afxdialogex.h"
#include "resource.h"
#include "util.h"
#include "errcode.h"

using namespace std;

char *redis_ip = "", *redis_ip_r = "";
int	redis_port = 0, redis_port_r = 0;

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

TCHAR *sz_adx[100] =
{
	_T("tanx"),
	_T("amax"),
	_T("inmobi"),
	_T("iwifi"),
	_T("letv"),
	_T("toutiao"),
	_T("zplay"),
	_T("baidu"),
	_T("sohu"),
	_T("adwalker"),
	_T("iqiyi"),
	_T("goyoo"),
	_T("iflytek"),
	_T("adview")

};

int update_flag = 0, u_adx = 0;
char *get_groupid_info = NULL;

DWORD WINAPI threadUpdate(LPVOID lpParam);
/*DWORD WINAPI threadtime(LPVOID lpParam);*/

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


// Cintelligent_priceDlg �Ի���



Cintelligent_priceDlg::Cintelligent_priceDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(Cintelligent_priceDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void Cintelligent_priceDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT1, m_groupid);
	DDX_Control(pDX, IDC_COMBO1, m_combo_adx);
	DDX_Control(pDX, IDC_CHECK1, m_intelligent_check);
	DDX_Control(pDX, IDC_EDIT3, m_ip);
	DDX_Control(pDX, IDC_EDIT2, m_port);
	DDX_Control(pDX, IDC_BUTTON1, m_connect);
	DDX_Control(pDX, IDC_BUTTON2, m_disconnect);
	DDX_Control(pDX, IDC_LIST3, m_list_control);
	DDX_Control(pDX, IDC_INTERVAL, m_interval);
	DDX_Control(pDX, IDC_MIN_PRICE, m_minprice);
	DDX_Control(pDX, IDC_MAX_PRICE, m_maxprice);
	DDX_Control(pDX, IDC_CONFIRM, m_confirm);
	DDX_Control(pDX, IDC_MODIFY, m_modify);
	DDX_Control(pDX, IDC_EDIT4, m_ip_r);
	DDX_Control(pDX, IDC_EDIT5, m_port_r);
	DDX_Control(pDX, IDC_EDIT6, m_valid_day);
}

BEGIN_MESSAGE_MAP(Cintelligent_priceDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_EN_CHANGE(IDC_EDIT1, &Cintelligent_priceDlg::OnEnChangeEdit1)
	ON_CBN_SELCHANGE(IDC_COMBO1, &Cintelligent_priceDlg::OnCbnSelchangeCombo1)
	ON_BN_CLICKED(IDC_CHECK1, &Cintelligent_priceDlg::OnBnClickedCheck1)
	ON_BN_CLICKED(IDC_BUTTON1, &Cintelligent_priceDlg::OnBnClickedButton1)
	ON_BN_CLICKED(IDC_BUTTON2, &Cintelligent_priceDlg::OnBnClickedButton2)
	ON_BN_CLICKED(IDC_BUTTON4, &Cintelligent_priceDlg::OnBnClickedButton4)
	ON_BN_CLICKED(IDC_CONFIRM, &Cintelligent_priceDlg::OnBnClickedConfirm)
	ON_BN_CLICKED(IDC_MODIFY, &Cintelligent_priceDlg::OnBnClickedModify)
END_MESSAGE_MAP()


// Cintelligent_priceDlg ��Ϣ�������

BOOL Cintelligent_priceDlg::OnInitDialog()
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
	int index = 0;
	char szmaster_ip[MINI_LENGTH] = "", szmaster_port[MINI_LENGTH] = "",
		szslave_ip[MINI_LENGTH] = "", szslave_port[MINI_LENGTH] = "", szgroupid[MIN_LENGTH] = "";
	CString cmaster_ip, cmaster_port, cslave_ip, cslave_port, cgroupid;
	TCHAR *strInfo[2][5] = { { _T("mapid1"), _T("640 x 100"), _T("10"), _T("5.2"), _T("20") },	\
		{ _T("mapid2"), _T("320 x 50"), _T("20"), _T("10"), _T("30") } };
	TCHAR *strColumnInfo[5] = { _T("����"), _T("�ߴ�"), _T("���۳��� Ԫ/CPM"), _T("�ɽ��� Ԫ/CPM"), _T("���ʱ�� min") };
	CRect Rect;
	IniReadStrByNameA(FILENAME_INI, SECTION_INFO, "master_ip", szmaster_ip);
	IniReadStrByNameA(FILENAME_INI, SECTION_INFO, "master_port", szmaster_port);
	IniReadStrByNameA(FILENAME_INI, SECTION_INFO, "slave_ip", szslave_ip);
	IniReadStrByNameA(FILENAME_INI, SECTION_INFO, "slave_port", szslave_port);
	IniReadStrByNameA(FILENAME_INI, SECTION_INFO, "groupid", szgroupid);

	if ((strlen(szmaster_ip) == 0) || (strlen(szmaster_ip) == 0) || (strlen(szslave_port) == 0) || (strlen(szslave_ip) == 0) ||
		(strlen(szslave_ip) == 0))
	{
		cmaster_ip = _T("111.235.158.31");
		cmaster_port = _T("7379");
		cslave_ip = _T("111.235.158.26");
		cslave_port = _T("7380");
		cgroupid = _T("");
	}
	else
	{
		cmaster_ip = szmaster_ip;
		cmaster_port = szmaster_port;
		cslave_ip = szslave_ip;
		cslave_port = szslave_port;
		cgroupid = szgroupid;
	}

	for (index; index <= sizeof(sz_adx) / sizeof(char *); index++)
		m_combo_adx.AddString(sz_adx[index]);

	//list Control
	
	m_list_control.GetClientRect(&Rect);
	m_list_control.ModifyStyle(LVS_ICON | LVS_SMALLICON | LVS_LIST, LVS_REPORT);
	m_list_control.SetExtendedStyle(LVS_EX_FULLROWSELECT);

	m_list_control.InsertColumn(0, (LPCTSTR)strColumnInfo[0], LVCFMT_LEFT, Rect.Width() * 2 / 6, 0);
	for (int n = 1; n < 5; n++)
		m_list_control.InsertColumn(n, (LPCTSTR)strColumnInfo[n], LVCFMT_LEFT, Rect.Width() / 6, n);

	
	for (int i = 0; i < 2; i++)
	{
		m_list_control.InsertItem(i, strInfo[i][0]);
		for (int j = 1; j < 5; j++)
		{
			m_list_control.SetItemText(i, j, strInfo[i][j]);
		}
	}

	m_interval.SetWindowText(_T("30"));//Default 30 min
	m_maxprice.SetWindowText(_T("1.2"));
	m_minprice.SetWindowText(_T("0.5"));
	m_valid_day.SetWindowText(_T("20"));//day
	m_groupid.SetWindowTextW(cgroupid);
	m_ip.SetWindowText(cmaster_ip);
	m_port.SetWindowText(cmaster_port);
	m_ip_r.SetWindowTextW(cslave_ip);
	m_port_r.SetWindowTextW(cslave_port);
	m_modify.EnableWindow(FALSE);

	hThreadUpdate = CreateThread(NULL, 0, threadUpdate, this, 0, NULL);
	//hThreadtime = CreateThread(NULL, 0, threadtime, this, 0, NULL);

	return TRUE;  // ���ǽ��������õ��ؼ������򷵻� TRUE
}

void Cintelligent_priceDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else if ((nID & 0xFFF0) == SC_CLOSE)
	{
		//���redis����
		redis_deconnect();
		CDialog::OnClose();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// �����Ի��������С����ť������Ҫ����Ĵ���
//  �����Ƹ�ͼ�ꡣ  ����ʹ���ĵ�/��ͼģ�͵� MFC Ӧ�ó���
//  �⽫�ɿ���Զ���ɡ�

void Cintelligent_priceDlg::OnPaint()
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
HCURSOR Cintelligent_priceDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void Cintelligent_priceDlg::OnEnChangeEdit1()
{
	// TODO:  ����ÿؼ��� RICHEDIT �ؼ���������
	// ���ʹ�֪ͨ��������д CDialogEx::OnInitDialog()
	// ���������� CRichEditCtrl().SetEventMask()��
	// ͬʱ�� ENM_CHANGE ��־�������㵽�����С�

	// TODO:  �ڴ���ӿؼ�֪ͨ����������
}

void Cintelligent_priceDlg::OnCbnSelchangeCombo1()
{
	// TODO:  �ڴ���ӿؼ�֪ͨ����������
	/* ѡ��ADXƽ̨ */
	CString str_groupid, str_adx, str_status, str_interval, str_minprice, str_maxprice, str_time;
	int bCheck1 = -1, binterval = 30;
	double bminratio = 0.5, bmaxratio = 1.2, valid_time = 20;
	int errcode = E_SUCCESS;

	if (GetDlgItemText(IDC_STATIC1, str_status))
	{
		if (str_status == _T("������"))
		{
			int nSel = 0;
			float left_day = 0.0;
			INTELLIGENT_INFO intelligent_info;
			CString x_interval, x_minratio, x_maxratio, x_valid_time;
			char szinterval[MINI_LENGTH] = "", szminratio[MINI_LENGTH] = "", 
				szmaxratio[MINI_LENGTH] = "", szvalidtime[MINI_LENGTH] = "";

			m_valid_day.GetWindowText(str_time);
			m_groupid.GetWindowText(str_groupid);
			nSel = m_combo_adx.GetCurSel();
			m_combo_adx.GetLBText(nSel, str_adx);

			if (str_groupid.GetLength() == 0)
			{
				MessageBox(_T("��ĿIDΪ��"), _T("��ʾ!"));
				m_intelligent_check.SetCheck(FALSE);
				return;
			}
			
			intelligent_info.adx = nSel + 1;
			intelligent_info.flag = bCheck1;
			intelligent_info.interval = binterval;
			intelligent_info.minratio = bminratio;
			intelligent_info.maxratio = bmaxratio;
			intelligent_info.valid_time = valid_time;

			IniWriteStrByNameA(FILENAME_INI, SECTION_INFO, "groupid", KS_UNICODE_to_UTF8(str_groupid.GetBuffer()));

			errcode = redis_operion(str_groupid.GetBuffer(), intelligent_info);

			switch (errcode)
			{
			case E_REDIS_CONNECT_FAILD:
				MessageBox(_T("����������ʧ��!"), _T("��ʾ"));
				break;
			case E_REDIS_EXECUTE_FAILED:
				MessageBox(_T("Redisִ��ʧ��!"), _T("��ʾ"));
				break;
			default:
				break;
			}

			if (intelligent_info.flag == 1)
				m_intelligent_check.SetCheck(TRUE);
			else
				m_intelligent_check.SetCheck(FALSE);

			sprintf(szinterval, "%d", intelligent_info.interval);
			sprintf(szminratio, "%f", intelligent_info.minratio);
			sprintf(szmaxratio, "%f", intelligent_info.maxratio);
			sprintf(szvalidtime, "%f", intelligent_info.valid_time);

			x_interval = szinterval;
			x_minratio = szminratio;
			x_maxratio = szmaxratio;
			x_valid_time = szvalidtime;
			m_interval.SetWindowTextW(x_interval);
			m_minprice.SetWindowTextW(x_minratio);
			m_maxprice.SetWindowTextW(x_maxratio);
			m_valid_day.SetWindowTextW(x_valid_time);
		}
		else
			MessageBox(_T("�������ӷ�����!"), _T("��ʾ"));
	}
	else
		MessageBox(_T("�Ƿ�����!"), _T("��ʾ"));
	
}

void Cintelligent_priceDlg::OnBnClickedCheck1()
{
	// TODO:  �ڴ���ӿؼ�֪ͨ����������	
}


void Cintelligent_priceDlg::OnBnClickedButton1()
{
	// TODO:  �ڴ���ӿؼ�֪ͨ����������
	/* Redis���Ӱ�ť */
	int errcode = E_SUCCESS;
	CString csmaster_ip, csmaster_port, csslave_ip, csslave_port;
	char *master_ip = NULL, *master_port = NULL, *slave_ip = NULL, *slave_port = NULL;

	m_ip.GetWindowText(csmaster_ip);
	m_port.GetWindowText(csmaster_port);
	m_ip_r.GetWindowTextW(csslave_ip);
	m_port_r.GetWindowText(csslave_port);

	if ((csmaster_ip.GetLength() == 0) || (csmaster_port.GetLength() == 0) || (csslave_ip.GetLength() == 0) || (csslave_port.GetLength() == 0))
	{
		MessageBox(_T("���������������ַ�Ͷ˿ڣ�"), _T("��ʾ"));
		m_intelligent_check.SetCheck(FALSE);
		return;
	}
	update_flag = 1;

	redis_ip = KS_UNICODE_to_UTF8(csmaster_ip.GetBuffer());
	redis_port = _wtoi(csmaster_port.GetBuffer());
	redis_ip_r = KS_UNICODE_to_UTF8(csslave_ip.GetBuffer());
	redis_port_r = _wtoi(csslave_port.GetBuffer());

	IniWriteStrByNameA(FILENAME_INI, SECTION_INFO, "master_ip", redis_ip);
	IniWriteStrByNameA(FILENAME_INI, SECTION_INFO, "master_port", KS_UNICODE_to_UTF8(csmaster_port.GetBuffer()));
	IniWriteStrByNameA(FILENAME_INI, SECTION_INFO, "slave_ip", redis_ip_r);
	IniWriteStrByNameA(FILENAME_INI, SECTION_INFO, "slave_port", KS_UNICODE_to_UTF8(csslave_port.GetBuffer()));

	errcode = redis_connecting();
	if (errcode == E_SUCCESS)
	{
		m_ip.EnableWindow(FALSE);
		m_port.EnableWindow(FALSE);
		m_ip_r.EnableWindow(FALSE);
		m_port_r.EnableWindow(FALSE);
		m_connect.EnableWindow(FALSE);
		m_disconnect.EnableWindow(TRUE);
		GetDlgItem(IDC_STATIC1)->SetWindowText(_T("������"));
	}
}


void Cintelligent_priceDlg::OnBnClickedButton2()
{
	// TODO:  �ڴ���ӿؼ�֪ͨ����������
	/* Redis�Ͽ���ť */
	int errcode = E_SUCCESS;

	update_flag = 0;
	errcode = redis_deconnect();
	if (errcode == E_SUCCESS)
	{
		m_ip.EnableWindow(TRUE);
		m_port.EnableWindow(TRUE);
		m_ip_r.EnableWindow(TRUE);
		m_port_r.EnableWindow(TRUE);
		m_connect.EnableWindow(TRUE);
		m_disconnect.EnableWindow(FALSE);
		GetDlgItem(IDC_STATIC1)->SetWindowText(_T("�ѶϿ�"));
	}
}


void Cintelligent_priceDlg::OnBnClickedButton4()
{
	// TODO:  �ڴ���ӿؼ�֪ͨ����������
	/* ˢ�°�ť */
	
}


void Cintelligent_priceDlg::OnBnClickedConfirm()
{
	// TODO:  �ڴ���ӿؼ�֪ͨ����������
	/* ���ܳ����㷨���� */
	CString str_groupid, str_adx, str_status, str_interval, str_minprice, str_maxprice, str_time;
	TCHAR *groupid = NULL, *interval = NULL, *minprice = NULL, *maxprice = NULL;
	int bCheck1 = 0, binterval = 30;
	double bminratio = 0, bmaxratio = 0;
	int errcode = E_SUCCESS;

	if (GetDlgItemText(IDC_STATIC1, str_status))
	{
		if (str_status == _T("������"))
		{
			int nSel = 0;
			INTELLIGENT_INFO intelligent_info;

			m_groupid.GetWindowTextW(str_groupid);
			m_interval.GetWindowTextW(str_interval);
			m_minprice.GetWindowTextW(str_minprice);
			m_maxprice.GetWindowTextW(str_maxprice);
			m_valid_day.GetWindowTextW(str_time);

			nSel = m_combo_adx.GetCurSel();
			m_combo_adx.GetLBText(nSel, str_adx);

			if ((str_groupid.GetLength() == 0) || (str_interval.GetLength() == 0) || (str_minprice.GetLength() == 0) || (str_maxprice.GetLength() == 0))
			{
				MessageBox(_T("��ĿID��ʱ����޻�۸�����Ϊ��"), _T("��ʾ"));
				m_intelligent_check.SetCheck(FALSE);
				return;
			}

			get_groupid_info = KS_UNICODE_to_UTF8(str_groupid.GetBuffer());//
			u_adx = nSel + 1;
			update_flag = 1;

			if (BST_CHECKED == IsDlgButtonChecked(IDC_INTELLIGENT)) //MessageBox(_T("Intelligent  is checked!."), _T("Notice!"));
				bCheck1 = 1;
			else
				bCheck1 = 0;

			intelligent_info.adx = nSel + 1;
			intelligent_info.flag = bCheck1;
			intelligent_info.interval = _wtoi(str_interval.GetBuffer());
			intelligent_info.minratio = _wtof(str_minprice.GetBuffer());
			intelligent_info.maxratio = _wtof(str_maxprice.GetBuffer());//���ӱ�������������
			intelligent_info.valid_time = _wtoi(str_time.GetBuffer());
			if ((intelligent_info.interval < 30) || (intelligent_info.interval >= 60) ||
				(intelligent_info.minratio < 0.3) || (intelligent_info.minratio > 1.0) ||
				(intelligent_info.maxratio < 1.0) || (intelligent_info.maxratio > 1.2))
			{
				MessageBox(_T("�������,�۸����䷶ΧΪ0.3~1.2,ʱ����޷�ΧΪ30~60."), _T("��ʾ"));
				m_intelligent_check.SetCheck(FALSE);
				return;
			}

			errcode = redis_operion(str_groupid.GetBuffer(), intelligent_info);

			switch (errcode)
			{
			case E_REDIS_CONNECT_FAILD:
				MessageBox(_T("����������ʧ��"), _T("��ʾ"));
				break;
			case E_REDIS_EXECUTE_FAILED:
				MessageBox(_T("Redisִ��ʧ��"), _T("��ʾ"));
				break;
			case E_SUCCESS:
				MessageBox(_T("�����ɹ�"), _T("��ʾ"));
				break;
			default:
				break;
			}

			m_intelligent_check.EnableWindow(FALSE);
			m_valid_day.EnableWindow(FALSE);
			m_combo_adx.EnableWindow(FALSE);
			m_groupid.EnableWindow(FALSE);
			m_maxprice.EnableWindow(FALSE);
			m_minprice.EnableWindow(FALSE);
			m_interval.EnableWindow(FALSE);
			m_confirm.EnableWindow(FALSE);
			m_modify.EnableWindow(TRUE);
		}
		else
			MessageBox(_T("�������ӷ�����."), _T("��ʾ"));
	}
	else
		MessageBox(_T("�Ƿ�����!"), _T("��ʾ"));

	return;
}


void Cintelligent_priceDlg::OnBnClickedModify()
{
	// TODO:  �ڴ���ӿؼ�֪ͨ����������
	get_groupid_info = NULL;//
	update_flag = 0;
	u_adx = 0;
	m_valid_day.EnableWindow(TRUE);
	m_intelligent_check.EnableWindow(TRUE);
	m_combo_adx.EnableWindow(TRUE);
	m_groupid.EnableWindow(TRUE);
	m_maxprice.EnableWindow(TRUE);
	m_minprice.EnableWindow(TRUE);
	m_interval.EnableWindow(TRUE);
	m_modify.EnableWindow(FALSE);
	m_confirm.EnableWindow(TRUE);
}

DWORD WINAPI threadUpdate(LPVOID lpParam)
{
	int errcode = E_SUCCESS;
	Cintelligent_priceDlg* intelligent_ui = (Cintelligent_priceDlg*)lpParam;
	
	CListCtrl *update_list;
	CEdit *update_time;
	
	update_list = (CListCtrl *)intelligent_ui->GetDlgItem(IDC_LIST3);
	update_time = (CEdit *)intelligent_ui->GetDlgItem(IDC_EDIT6);

	while (1)
	{
		if ((update_flag) && (get_groupid_info != NULL) && (u_adx != 0))
		{
			int asize = 0, cpm_ratio = 0;
			double left_time = 20;
			char szleft_time[MIN_LENGTH] = "";
			vector <AVER_PRICE> averprice;
			update_list->DeleteAllItems();

			errcode = get_aver_price(get_groupid_info, u_adx, averprice, left_time);

			switch (u_adx)
			{
			case 2:
			case 10:
			case 14:
				cpm_ratio = 10000;
				break;
			case 3:
			case 13:
				cpm_ratio = 1;
				break;
			default:
				cpm_ratio = 100;
				break;
			}

			sprintf_s(szleft_time, "%f", left_time);
			update_time->SetWindowTextW(KS_UTF8_to_UNICODE(szleft_time));
			
			for (int i = 0; i < averprice.size(); i++)
			{
				char outprice[20] = "", finish_time[20] = "", realprice[20] = "";

				sprintf(outprice, "%f", averprice[i].out_price);
				sprintf(realprice, "%f", averprice[i].real_price / cpm_ratio );
				if (averprice[i].finish_time == -1)
					sprintf(finish_time, "");
				else
					sprintf(finish_time, "%d", averprice[i].finish_time);

				update_list->InsertItem(i, KS_UTF8_to_UNICODE(averprice[i].mapid));
				update_list->SetItemText(i, 1, KS_UTF8_to_UNICODE(averprice[i].size));
				update_list->SetItemText(i, 2, KS_UTF8_to_UNICODE(outprice));
				update_list->SetItemText(i, 3, KS_UTF8_to_UNICODE(realprice));
				update_list->SetItemText(i, 4, KS_UTF8_to_UNICODE(finish_time));
			}

		}
		
		Sleep(3000);
	}
}


// DWORD WINAPI threadtime(LPVOID lpParam)
// {
// 	int errcode = E_SUCCESS;
// 	Cintelligent_priceDlg* intelligent_ui = (Cintelligent_priceDlg*)lpParam;
// 	CEdit *update_time, *groupid;
// 	update_time = (CEdit *)intelligent_ui->GetDlgItem(IDC_EDIT6);
// 	groupid = (CEdit *)intelligent_ui->GetDlgItem(IDC_GROUPID);
// 	while (1)
// 	{
// 		CString cgroupid;
// 		groupid->GetWindowTextW(cgroupid);
// 		if ((cgroupid.GetLength() != 0) && (update_flag))
// 		{
// 			double left_time = 20.0;
// 			char szleft_time[MIN_LENGTH] = "";
// 			left_time = get_keys_left_time(KS_UNICODE_to_UTF8(cgroupid.GetBuffer()));
// 
// 			sprintf_s(szleft_time, "%f", left_time);
// 			update_time->SetWindowTextW(KS_UTF8_to_UNICODE(szleft_time));
// 		}
// 
// 		Sleep(1000);
// 	}
// }