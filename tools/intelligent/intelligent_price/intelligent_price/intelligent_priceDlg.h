
// intelligent_priceDlg.h : 头文件
//

#pragma once
#include "afxwin.h"
#include "afxcmn.h"


// Cintelligent_priceDlg 对话框
class Cintelligent_priceDlg : public CDialogEx
{
// 构造
public:
	Cintelligent_priceDlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
	enum { IDD = IDD_INTELLIGENT_PRICE_DIALOG };

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
	CEdit m_groupid;
	CComboBox m_combo_adx;
	afx_msg void OnEnChangeEdit1();
	afx_msg void OnCbnSelchangeCombo1();
	afx_msg void OnBnClickedCheck1();
	afx_msg void OnBnClickedConfirm();
	afx_msg void OnBnClickedModify();
	afx_msg void OnBnClickedButton1();
	afx_msg void OnBnClickedButton2();
	afx_msg void OnBnClickedButton4();
	afx_msg void OnClose();
	CButton m_intelligent_check;
	CEdit m_ip;
	CEdit m_port;
	CButton m_connect;
	CButton m_disconnect;
	CListCtrl m_list_control;
	CEdit m_interval;
	CEdit m_minprice;
	CEdit m_maxprice;
	CButton m_confirm;
	CButton m_modify;
	HANDLE hThreadUpdate;
	HANDLE hThreadtime;
	CEdit m_ip_r;
	CEdit m_port_r;
	CEdit m_valid_day;
};
