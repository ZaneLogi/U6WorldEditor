#pragma once
#include "afxwin.h"
#include <string>
#include "Script.h"

#include "MapManager.h"

// CConverseDlg dialog

class CConverseDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CConverseDlg)

public:
	CConverseDlg(MapManager* mm, CWnd* pParent = NULL);   // standard constructor
	virtual ~CConverseDlg();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_CONVERSE };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()
public:
    afx_msg void OnBnClickedOk();
    afx_msg void OnBnClickedCancel();
    CEdit m_input;
    CEdit m_output;

    MapManager& m_map_manager;
    ScriptInterpreter m_interpreter;
    std::string m_text;
    CListBox m_lbNPCs;
    afx_msg void OnSelchangeNpc();
};
