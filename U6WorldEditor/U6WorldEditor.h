
// U6WorldEditor.h : main header file for the U6WorldEditor application
//
#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"       // main symbols


// CU6WorldEditorApp:
// See U6WorldEditor.cpp for the implementation of this class
//

class CU6WorldEditorApp : public CWinApp
{
public:
	CU6WorldEditorApp();


// Overrides
public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();

// Implementation

public:
	afx_msg void OnAppAbout();
	DECLARE_MESSAGE_MAP()
    virtual BOOL OnIdle(LONG lCount);
};

extern CU6WorldEditorApp theApp;
