
// odin_log.h : PROJECT_NAME Ӧ�ó������ͷ�ļ�
//

#pragma once

#ifndef __AFXWIN_H__
	#error "�ڰ������ļ�֮ǰ������stdafx.h�������� PCH �ļ�"
#endif

#include "resource.h"		// ������


// Codin_logApp: 
// �йش����ʵ�֣������ odin_log.cpp
//

class Codin_logApp : public CWinApp
{
public:
	Codin_logApp();

// ��д
public:
	virtual BOOL InitInstance();

// ʵ��

	DECLARE_MESSAGE_MAP()
};

extern Codin_logApp theApp;