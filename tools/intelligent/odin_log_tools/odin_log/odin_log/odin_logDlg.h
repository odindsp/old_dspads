
// odin_logDlg.h : 头文件
//

#pragma once
#include "afxwin.h"


// Codin_logDlg 对话框
class Codin_logDlg : public CDialogEx
{
// 构造
public:
	Codin_logDlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
	enum { IDD = IDD_ODIN_LOG_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
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
