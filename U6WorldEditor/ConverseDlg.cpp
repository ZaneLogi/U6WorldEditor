// ConverseDlg.cpp : implementation file
//

#include "stdafx.h"
#include "U6WorldEditor.h"
#include "ConverseDlg.h"
#include "afxdialogex.h"


// CConverseDlg dialog

IMPLEMENT_DYNAMIC(CConverseDlg, CDialogEx)

CConverseDlg::CConverseDlg(const std::string& script, CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_CONVERSE, pParent), m_interpreter(script)
{
}

CConverseDlg::~CConverseDlg()
{
}

void CConverseDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_EDIT2, m_input);
    DDX_Control(pDX, IDC_OUTPUT, m_output);
}


BEGIN_MESSAGE_MAP(CConverseDlg, CDialogEx)
    ON_BN_CLICKED(IDOK, &CConverseDlg::OnBnClickedOk)
    ON_BN_CLICKED(IDCANCEL, &CConverseDlg::OnBnClickedCancel)
END_MESSAGE_MAP()


// CConverseDlg message handlers


void CConverseDlg::OnBnClickedOk()
{
    char input[256];
    GetWindowTextA(m_input.GetSafeHwnd(), input, 256);

    CString s;
    s.Format(_T(":%hs\r\n"), input);
    int l = m_output.GetWindowTextLength();
    m_output.SetSel(l, l);
    m_output.ReplaceSel(s);

    m_input.SetWindowText(_T(""));


    m_interpreter.run(std::string(input), m_text);

    auto size = m_text.length();
    size_t index = 0;
    size_t next;
    do
    {
        next = m_text.find('\n', index);
        if (next == std::string::npos)
        {
            next = size;
        }

        s.Format(_T("%hs\r\n"), m_text.substr(index, next - index).c_str());
        l = m_output.GetWindowTextLength();
        m_output.SetSel(l, l);
        m_output.ReplaceSel(s);

        index = next + 1;
    } while (index < size);
}


void CConverseDlg::OnBnClickedCancel()
{
    // TODO: Add your control notification handler code here
    CDialogEx::OnCancel();
}


BOOL CConverseDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    // TODO:  Add extra initialization here

    m_interpreter.run(std::string(), m_text);

    CString s;
    s.Format(_T("\n%hs\n"), m_text.c_str());
    m_output.SetSel(-1);
    m_output.ReplaceSel(s);

    m_input.SetFocus();

    return FALSE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}
