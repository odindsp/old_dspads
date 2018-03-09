
// odin_logDlg.h : ͷ�ļ�
//

#pragma once
#include "afxwin.h"


// Codin_logDlg �Ի���
class Codin_logDlg : public CDialogEx
{
// ����
public:
	Codin_logDlg(CWnd* pParent = NULL);	// ��׼���캯��

// �Ի�������
	enum { IDD = IDD_ODIN_LOG_DIALOG };

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
	afx_msg void OnCbnSelchangeCombo1();
	afx_msg void OnCbnSelchangeAdx();
	afx_msg void OnEnChangeEdit1();
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedCancel();
	CEdit m_ip;
	CEdit m_port;
	CButton m_connect;
	CButton m_disconnect;
	CStatic m_status;
	CComboBox m_adx;
	CComboBox m_err;
	CComboBox m_time;
	CButton m_comfirm;
	CButton m_modify;
	afx_msg void OnBnClickedConnect();
	afx_msg void OnBnClickedDisconnect();
	CButton m_modify2;
	afx_msg void OnBnClickedButton1();
};
