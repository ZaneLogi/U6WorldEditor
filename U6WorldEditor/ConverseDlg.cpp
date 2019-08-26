// ConverseDlg.cpp : implementation file
//

#include "stdafx.h"
#include "U6WorldEditor.h"
#include "ConverseDlg.h"
#include "afxdialogex.h"


// CConverseDlg dialog

IMPLEMENT_DYNAMIC(CConverseDlg, CDialogEx)

CConverseDlg::CConverseDlg(MapManager* mm, CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_CONVERSE, pParent), m_map_manager(*mm)
{
    m_selected_npc_id = -1;
}

CConverseDlg::~CConverseDlg()
{
}

void CConverseDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_EDIT2, m_input);
    DDX_Control(pDX, IDC_OUTPUT, m_output);
    DDX_Control(pDX, IDC_NPC, m_lbNPCs);
}


BEGIN_MESSAGE_MAP(CConverseDlg, CDialogEx)
    ON_BN_CLICKED(IDOK, &CConverseDlg::OnBnClickedOk)
    ON_BN_CLICKED(IDCANCEL, &CConverseDlg::OnBnClickedCancel)
    ON_LBN_SELCHANGE(IDC_NPC, &CConverseDlg::OnSelchangeNpc)
    ON_BN_CLICKED(IDC_FIND_NPC, &CConverseDlg::OnBnClickedFindNpc)
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
    m_interpreter.load(m_map_manager.script.get_script(2));
    m_interpreter.run(std::string(), m_text);

    CString s;
    s.Format(_T("\n%hs\n"), m_text.c_str());
    m_output.SetSel(-1);
    m_output.ReplaceSel(s);


    for (int i = 0; i < 256; i++)
    {
        auto actor = m_map_manager.obj_manager.get_actor(i);
        auto npc_name = m_map_manager.script.get_npc_name(i);
        s.Format(_T("%3d %-12hs 0x%02x"), i, npc_name.c_str(), *actor.flags);
        m_lbNPCs.AddString(s);
    }

    auto ansi_fixed_font = CFont::FromHandle((HFONT)::GetStockObject(ANSI_FIXED_FONT));

    m_lbNPCs.SetFont(ansi_fixed_font);
    m_output.SetFont(ansi_fixed_font);

    m_input.SetFocus();
    return FALSE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}


void CConverseDlg::OnSelchangeNpc()
{
    int nSel = m_lbNPCs.GetCurSel();
    auto npc_script = m_map_manager.script.get_script(nSel);
    auto npc_name = m_map_manager.script.get_npc_name(nSel);

    m_selected_npc_id = -1;
    m_text.clear();
    if (!npc_script.empty())
    {
        m_selected_npc_id = nSel;
        m_interpreter.load(npc_script);

#define SAVE_RAW 0
#define FORMAT_SCRIPT 1
#define SAVE_FORMATTED 1

#if SAVE_RAW
        auto bin_filename = format_string("d:\\%03d_%s.bin", nSel, npc_name.c_str());
        std::ofstream f(bin_filename.c_str(), std::ios_base::out | std::ios_base::binary);
        f.write((char*)npc_script.data(), npc_script.size());
        f.close();
#endif

#if FORMAT_SCRIPT
        auto formatted = m_interpreter.format_script();
#endif

#if FORMAT_SCRIPT && SAVE_FORMATTED
        auto formatted_filename = format_string("d:\\%03d_%s_formatted.txt", nSel, npc_name.c_str());
        std::ofstream f_formatted(formatted_filename.c_str(), std::ios_base::out | std::ios_base::binary);
        f_formatted.write(formatted.c_str(), formatted.length());
        f_formatted.close();
#endif

        m_interpreter.run(std::string(), m_text);
    }

    CString s;
    s.Format(_T("\n%hs\n"), m_text.c_str());
    m_output.SetSel(0xffff0000);
    m_output.ReplaceSel(s);
}


void CConverseDlg::OnBnClickedFindNpc()
{
    EndDialog(IDC_FIND_NPC);
}
