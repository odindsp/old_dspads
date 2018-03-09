
// AppMonitorDlg.h : ͷ�ļ�
//

#pragma once


// CAppMonitorDlg �Ի���
class CAppMonitorDlg : public CDialogEx
{
// ����
public:
	CAppMonitorDlg(CWnd* pParent = NULL);	// ��׼���캯��

// �Ի�������
	enum { IDD = IDD_APPMONITOR_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV ֧��


// ʵ��
protected:
	HICON m_hIcon;

	// ���ɵ���Ϣӳ�亯��
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	CString m_imp_name;
	CString m_clk_name;
	CString m_appid;
	CString m_advid;
	CString m_mapid;
	CString m_curl;
	CString m_imp_url;
	CString m_clk_url;
	afx_msg void OnBnClickedButtonCancel();
	afx_msg void OnBnClickedButtonOk();
};
