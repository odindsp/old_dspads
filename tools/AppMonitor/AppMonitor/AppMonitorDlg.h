
// AppMonitorDlg.h : 头文件
//

#pragma once


// CAppMonitorDlg 对话框
class CAppMonitorDlg : public CDialogEx
{
// 构造
public:
	CAppMonitorDlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
	enum { IDD = IDD_APPMONITOR_DIALOG };

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
