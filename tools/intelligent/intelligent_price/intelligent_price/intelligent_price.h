
// intelligent_price.h : PROJECT_NAME Ӧ�ó������ͷ�ļ�
//

#pragma once

#ifndef __AFXWIN_H__
	#error "�ڰ������ļ�֮ǰ������stdafx.h�������� PCH �ļ�"
#endif

#include "resource.h"		// ������


// Cintelligent_priceApp: 
// �йش����ʵ�֣������ intelligent_price.cpp
//

class Cintelligent_priceApp : public CWinApp
{
public:
	Cintelligent_priceApp();

// ��д
public:
	virtual BOOL InitInstance();

// ʵ��

	DECLARE_MESSAGE_MAP()
};

extern Cintelligent_priceApp theApp;