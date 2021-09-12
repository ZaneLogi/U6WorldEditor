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
    ON_MESSAGE(WM_DPICHANGED, &CConverseDlg::OnDpiChanged)
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


    using fnGetDpiForWindow = UINT (WINAPI*)(HWND);

    fnGetDpiForWindow myGetDpiForWindow = nullptr;

    auto hinstLib = LoadLibrary(TEXT("user32.dll"));
    if (hinstLib != nullptr)
    {
        myGetDpiForWindow = (fnGetDpiForWindow)GetProcAddress(hinstLib, "GetDpiForWindow");
    }

    if (myGetDpiForWindow != nullptr)
    {
        int iDpi = myGetDpiForWindow(GetSafeHwnd());

        CClientDC dc(this);

        static const int points_per_inch = 72;
        int points = 10;
        int pixels_per_inch = GetDeviceCaps(dc.GetSafeHdc(), LOGPIXELSY);
        int pixels_height = -MulDiv(points, pixels_per_inch, points_per_inch);

        m_font.CreateFont(
            pixels_height, 0, // size
            0, 0, // normal orientation
            FW_NORMAL,   // normal weight--e.g., bold would be FW_BOLD
            false, false, false, // not italic, underlined or strike out
            DEFAULT_CHARSET,
            OUT_OUTLINE_PRECIS, // select only outline (not bitmap) fonts
            CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY,
            VARIABLE_PITCH | FF_SWISS,
            _T("Courier"));

        m_lbNPCs.SetFont(&m_font);
        m_output.SetFont(&m_font);
    }

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

LRESULT CConverseDlg::OnDpiChanged(WPARAM wParam, LPARAM lParam)
{
    return 0;
}