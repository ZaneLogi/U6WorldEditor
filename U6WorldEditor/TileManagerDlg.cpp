// TileManagerDlg.cpp : implementation file
//

#include "stdafx.h"
#include "U6WorldEditor.h"
#include "TileManagerDlg.h"
#include "afxdialogex.h"


// CTileManagerDlg dialog

IMPLEMENT_DYNAMIC(CTileManagerDlg, CDialogEx)

CTileManagerDlg::CTileManagerDlg(MapManager* mm, CWnd* pParent /*=NULL*/)
	: m_map_manager(*mm), CDialogEx(IDD_TILE_MANAGER, pParent)
{

}

CTileManagerDlg::~CTileManagerDlg()
{
}

void CTileManagerDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_LIST1, m_lcTiles);
}


BEGIN_MESSAGE_MAP(CTileManagerDlg, CDialogEx)
    ON_WM_MOVE()
    ON_WM_SIZE()
    ON_WM_GETMINMAXINFO()
END_MESSAGE_MAP()


// CTileManagerDlg message handlers


BOOL CTileManagerDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    // TODO:  Add extra initialization here
    BEGIN_OBJ_MAP(CTileManagerDlg);
    OBJ_DEFINE_SCALEABLE(IDC_LIST1);
    OBJ_DEFINE_BOTTOM_RIGHT(IDOK);
    OBJ_DEFINE_BOTTOM_RIGHT(IDCANCEL);
    END_OBJ_MAP();

    m_lcTiles.SetExtendedStyle(LVS_EX_AUTOSIZECOLUMNS | LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

    m_lcTiles.InsertColumn(0, _T("NO"));
    m_lcTiles.InsertColumn(1, _T("Image"));
    m_lcTiles.InsertColumn(2, _T("Description"));
    m_lcTiles.InsertColumn(3, _T("Flag 1"));
    m_lcTiles.InsertColumn(4, _T("Flag 2"));
    m_lcTiles.InsertColumn(5, _T("Flag 3"));

    auto& tile_manager = m_map_manager.tile_manager;

    for (int i = 0; i < 2048; i++)
    {
        m_lcTiles.InsertItem(i, _T(""));
        m_lcTiles.SetItemTextEx(i, 0, _T("%d"), i);
        m_lcTiles.SetItemTextEx(i, 1, _T("XXXX"));

        auto desc = tile_manager.get_description(i);
        m_lcTiles.SetItemTextEx(i, 2, _T("%hs"), desc.c_str());

        auto ti = tile_manager.get_info(i);
        m_lcTiles.SetItemTextEx(i, 3, _T("0x%02X"), ti.flag1);
        m_lcTiles.SetItemTextEx(i, 4, _T("0x%02X"), ti.flag2);
        m_lcTiles.SetItemTextEx(i, 5, _T("0x%02X"), ti.flag3);
    }

    // code below is used to make the row height larger
    //m_ilTiles.Create(16, 32, ILC_COLOR4, 10, 10);
    //m_lcTiles.SetImageList(&m_ilTiles, LVSIL_SMALL);

    for (int i = 0; i < m_lcTiles.GetHeaderCtrl()->GetItemCount(); i++)
    {
        m_lcTiles.SetColumnWidth(i, LVSCW_AUTOSIZE_USEHEADER);
    }

    auto& tiles_dib = m_lcTiles.m_tiles_dib;
    tiles_dib.create(32 * 16, 64 * 16, 8, 256, m_map_manager.color_palette.color_entries());
    for (int i = 0; i < 64 * 16; i++)
    {
        auto image_bits = tiles_dib.bits(i);
        memset(image_bits, 255, 32 * 16);
    }

    auto& tile_image = m_map_manager.tile_image;
    for (int i = 0; i < 2048; i++)
    {
        int x = (i % 32) * 16;
        int y = (i / 32) * 16;
        tile_image.draw(tiles_dib, x, y, i);
    }


    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}

void CTileManagerDlg::OnMove(int x, int y)
{
    SAVE_WINDOW_PLACEMENT();
}

void CTileManagerDlg::OnSize(UINT nType, int cx, int cy)
{
    __super::OnSize(nType, cx, cy); // need to change to CDialogEx if the base is CDialogEx
    UPDATE_OBJ_POSITION(cx, cy);
    SAVE_WINDOW_PLACEMENT();
}

void CTileManagerDlg::OnGetMinMaxInfo(MINMAXINFO* lpMMI)
{
    UPDATE_MINMAX_INFO(lpMMI);
    __super::OnGetMinMaxInfo(lpMMI); // need to change to CDialogEx if the base is CDialogEx
}
