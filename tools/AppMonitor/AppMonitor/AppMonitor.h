
// AppMonitor.h : PROJECT_NAME Ӧ�ó������ͷ�ļ�
//

#pragma once

#ifndef __AFXWIN_H__
	#error "�ڰ������ļ�֮ǰ������stdafx.h�������� PCH �ļ�"
#endif

#include "resource.h"		// ������


// CAppMonitorApp: 
// �йش����ʵ�֣������ AppMonitor.cpp
//

class CAppMonitorApp : public CWinApp
{
public:
	CAppMonitorApp();

// ��д
public:
	virtual BOOL InitInstance();

// ʵ��

	DECLARE_MESSAGE_MAP()
};

extern CAppMonitorApp theApp;