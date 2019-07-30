
// ChildView.cpp : implementation of the CChildView class
//

#include "stdafx.h"
#include <fstream>
#include <sstream>
#include <regex>
#include <map>
#include <Vfw.h>
#include "U6WorldEditor.h"
#include "ChildView.h"
#include "LZW.h"
#include "DibSection.h"
#include "ColorPalette.h"
#include "TileImage.h"
#include "TileManager.h"
#include "MapManager.h"

#pragma comment(lib,"vfw32.lib")

#ifdef _DEBUG
#define new DEBUG_NEW
#endif



HDRAWDIB gDD;
Configuration   gConfig;
DibSection      gScreen;
MapManager      gMapManager;

// CChildView

CChildView::CChildView()
{
    gConfig.load("init.cfg");
    gMapManager.init(gConfig);

    gScreen.create(640, 640, 8, 256, gMapManager.color_palette.color_entries());

    gDD = DrawDibOpen();

    m_map_ox = 128 * 16;
    m_map_oy = 128 * 16;
    m_map_z = 0;

    auto actor = gMapManager.obj_manager.get_actor(1);
    MoveToTile(actor.x, actor.y, actor.z);

    m_mouse_caputred = false;
}

CChildView::~CChildView()
{
    DrawDibClose(gDD);
}


BEGIN_MESSAGE_MAP(CChildView, CWnd)
    ON_WM_PAINT()
    ON_WM_CREATE()
    ON_WM_LBUTTONDOWN()
    ON_WM_LBUTTONUP()
    ON_WM_MOUSEMOVE()
    ON_WM_SIZE()
    ON_WM_ERASEBKGND()
    ON_COMMAND_RANGE(ID_GAME_U6, ID_GAME_MARTIAN, CChildView::OnGameType)
    ON_COMMAND_RANGE(ID_JUMPTO_BEGIN, ID_JUMPTO_END, CChildView::OnJumpTo)
END_MESSAGE_MAP()



// CChildView message handlers

BOOL CChildView::PreCreateWindow(CREATESTRUCT& cs)
{
    if (!CWnd::PreCreateWindow(cs))
        return FALSE;

    cs.dwExStyle |= WS_EX_CLIENTEDGE;
    cs.style &= ~WS_BORDER;
    cs.lpszClass = AfxRegisterWndClass(CS_HREDRAW|CS_VREDRAW|CS_DBLCLKS,
        ::LoadCursor(NULL, IDC_ARROW), reinterpret_cast<HBRUSH>(COLOR_WINDOW+1), NULL);

    return TRUE;
}

void CChildView::OnPaint()
{
    CPaintDC dc(this); // device context for painting

    // TODO: Add your message handler code here


    // Do not call CWnd::OnPaint() for painting messages
}



int CChildView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CWnd::OnCreate(lpCreateStruct) == -1)
        return -1;

    // TODO:  Add your specialized creation code here


    return 0;
}

void CChildView::Update()
{
    CClientDC dc(this);

    auto bitmap_info = gScreen.bitmap_info();
    int width = bitmap_info->bmiHeader.biWidth;
    int height = bitmap_info->bmiHeader.biHeight;

    gMapManager.update(gScreen);

#if 0
    gMapManager.color_palette.draw(gScreen, 640 - 128, 0);

    const int START = 0;
    const int END = START + 1024;
    const int Y = (START / 32) * 16;
    for (int i = START; i < END; i++)
    {
        int x = (i % 32) * 16;
        int y = (i / 32) * 16 - Y;
        gMapManager.tile_image.draw(gScreen, x, y, i);
        //gMapManager.tile_manager.draw(gScreen, x, y, i);
    }

    DrawDibDraw(gDD, dc.GetSafeHdc(), 0, 0, width, height,
        (BITMAPINFOHEADER*)&gScreen.bitmap_info()->bmiHeader,
        gScreen.bits(), 0, 0, width, height, NULL);

#endif

#if 1
    int map_tile_xstart = m_map_ox / 16;
    int map_tile_ystart = m_map_oy / 16;

    gMapManager.draw(gScreen, map_tile_xstart, map_tile_ystart, m_map_z);

    int xoffset = m_map_ox % 16;
    int yoffset = m_map_oy % 16;

    DrawDibDraw(gDD, dc.GetSafeHdc(), -xoffset, -yoffset, width, height,
        (BITMAPINFOHEADER*)&gScreen.bitmap_info()->bmiHeader,
        gScreen.bits(), 0, 0, width, height, NULL);

    int cur_tile_x = (m_mouse_last_point.x + m_map_ox) / 16;
    int cur_tile_y = (m_mouse_last_point.y + m_map_oy) / 16;

    Obj* obj = gMapManager.obj_manager.get_obj(cur_tile_x, cur_tile_y, m_map_z);
    std::string obj_desc;
    if (obj)
    {
        obj_desc = gMapManager.tile_manager.get_description(obj->obj_number, obj->obj_frame, obj->quantity);
        if (!obj->obj_list.empty())
        {
            obj_desc += "\n[\n";
            auto itr = obj->obj_list.begin();
            auto end = obj->obj_list.end();
            for (; itr != end; ++itr)
            {
                std::string name = gMapManager.tile_manager.get_description(itr->obj_number, itr->obj_frame, itr->quantity);
                obj_desc += name + "\n";
            }
            obj_desc += ']';
        }
    }

    CString s;
    s.Format(_T("%d, %d - %d, %d, %hs"), m_map_ox, m_map_oy, cur_tile_x, cur_tile_y, obj_desc.c_str() );
    dc.SelectStockObject(SYSTEM_FIXED_FONT);
    dc.SelectStockObject(BLACK_PEN);
    CRect rc(0, 0, 640, 640);
    dc.DrawText(s, &rc, DT_LEFT | DT_EXTERNALLEADING | DT_WORDBREAK);

#endif



}

void CChildView::MoveToTile(int xtile, int ytile, int z)
{
    int world_pixels;
    if (z == 0)
    {
        world_pixels = 1024 * 16;
    }
    else
    {
        world_pixels = 256 * 16;
    }

    int xcenter = xtile * 16 + 8;
    int ycenter = ytile * 16 + 8;

    int view_width = gScreen.width();
    int view_height = gScreen.height();

    m_map_ox = xcenter - view_width / 2;
    m_map_oy = ycenter - view_height / 2;
    m_map_z = z;
}


BOOL CChildView::OnEraseBkgnd(CDC* pDC)
{
    // TODO: Add your message handler code here and/or call default
    // return CWnd::OnEraseBkgnd(pDC);
    return TRUE;
}

void CChildView::OnSize(UINT nType, int cx, int cy)
{
    CWnd::OnSize(nType, cx, cy);

    // Add your message handler code here
    if (cx != 0 && cy != 0)
    {
        int width =  ((cx + 15) & (~0x0f)); // ((cx + 15) / 16) * 16
        int height = ((cy + 15) & (~0x0f));
        if (width < 640)
            width = 640;
        if (height < 640)
            height = 640;
        gScreen.create(width, height, 8, 256, gMapManager.color_palette.color_entries());
        Update();
    }
}

void CChildView::OnLButtonDown(UINT nFlags, CPoint point)
{
    m_mouse_caputred = true;
    m_mouse_last_point = point;
    SetCapture();

}


void CChildView::OnLButtonUp(UINT nFlags, CPoint point)
{
    m_mouse_caputred = false;
    ReleaseCapture();
}


void CChildView::OnMouseMove(UINT nFlags, CPoint point)
{
    const int GROUND_PIXELS = 1024 * 16;
    const int DUNGEON_PIXELS = 256 * 16;
    int boundary = m_map_z == 0 ? GROUND_PIXELS : DUNGEON_PIXELS;


    if (m_mouse_caputred)
    {
        m_map_ox += m_mouse_last_point.x - point.x;
        m_map_oy += m_mouse_last_point.y - point.y;
        if (m_map_ox < 0)
            m_map_ox += boundary;
        else if (m_map_ox >= boundary)
            m_map_ox -= boundary;

        if (m_map_oy < 0)
            m_map_oy += boundary;
        else if (m_map_oy >= boundary)
            m_map_oy -= boundary;

        m_mouse_last_point = point;
        Update();
    }

    m_mouse_last_point = point;
}

void CChildView::OnGameType(UINT id)
{
    switch (id) {
    case ID_GAME_U6:
        gConfig.set_property("game_type", "u6");
        break;
    case ID_GAME_SAVAGE:
        gConfig.set_property("game_type", "se");
        break;
    case ID_GAME_MARTIAN:
        gConfig.set_property("game_type", "md");
        break;
    }
    gMapManager.init(gConfig);

    m_map_ox = 128 * 16;
    m_map_oy = 128 * 16;
    m_map_z = 0;

    auto actor = gMapManager.obj_manager.get_actor(1);
    MoveToTile(actor.x, actor.y, actor.z);

    int view_width = gScreen.width();
    int view_height = gScreen.height();
    gScreen.create(view_width, view_height, 8, 256, gMapManager.color_palette.color_entries());

    Update();
}

typedef struct _JUMPTO
{
    int id; int x; int y; int z;
} JUMPTO;

static JUMPTO jmp_to_tbl[] =
{
    { ID_JUMPTO_CASTLE_OF_LORD_BRITISH, 307, 352, 0 },
    { ID_JUMPTO_ISLEOFTHEAVATAR,        923, 876, 0 },
    { ID_JUMPTO_SHRINEOFCONTROL,         68,  45, 5 },
    { ID_JUMPTO_SHRINEOFPASSION,        188,  45, 5 },
    { ID_JUMPTO_SHRINEOFDILIGENCE,      108, 221, 5 },
    { ID_JUMPTO_THEGARGOYLEALTAR,       128,  86, 5 },
    { ID_JUMPTO_BRITAIN,                435, 395, 0 },
    { ID_JUMPTO_JHELOM,                 147, 883, 0 },
    { ID_JUMPTO_MINOC,                  667,  67, 0 },
    { ID_JUMPTO_MOONGLOW,               899, 499, 0 },
    { ID_JUMPTO_NEWMAGINCIA,            739, 699, 0 },
    { ID_JUMPTO_SKARABRAE,               75, 507, 0 },
    { ID_JUMPTO_TRINSIC,                387, 787, 0 },
    { ID_JUMPTO_YEW,                    227, 131, 0 },
    { ID_JUMPTO_SHRINE_OF_COMPASSION,   503, 358, 0 },
    { ID_JUMPTO_SHRINE_OF_VALOR,        159, 942, 0 },
    { ID_JUMPTO_SHRINE_OF_SACRIFICE,    831, 166, 0 },
    { ID_JUMPTO_SHRINE_OF_HONESTY,      935, 262, 0 },
    { ID_JUMPTO_SHRINE_OF_HUMILITY,     919, 934, 0 },
    { ID_JUMPTO_SHRINE_OF_SPIRITUALITY,  23,  22, 1 },
    { ID_JUMPTO_SHRINE_OF_HONOR,        327, 822, 0 },
    { ID_JUMPTO_SHRINE_OF_JUSTICE,      295,  38, 0 },

    { ID_JUMPTO_SAVAGE_TELEPORT,        184, 454, 0 },

    { ID_JUMPTO_SAVAGE_BARAKO,          341, 211, 0 },
    { ID_JUMPTO_SAVAGE_BARRAB,          243, 635, 0 },
    { ID_JUMPTO_SAVAGE_DISQUIQUI,       384, 555, 0 },
    { ID_JUMPTO_SAVAGE_HAAKUR,          142,  39, 2 },
    { ID_JUMPTO_SAVAGE_JUKARI,          683, 819, 0 },
    { ID_JUMPTO_SAVAGE_KURAK,           411, 283, 0 },
    { ID_JUMPTO_SAVAGE_NAHUATLA,        547, 530, 0 },
    { ID_JUMPTO_SAVAGE_PINDIRO,         579, 114, 0 },
    { ID_JUMPTO_SAVAGE_SAKKHRA,         203, 162, 2 },
    { ID_JUMPTO_SAVAGE_URALI,           996, 338, 0 },
    { ID_JUMPTO_SAVAGE_YOLARU,          587, 323, 0 },

    { ID_JUMPTO_SAVAGE_PLATE_BARRAB,    206, 619, 0 }, // 182, 456, 0 - (0,4)
    { ID_JUMPTO_SAVAGE_PLATE_DISQUIQUI, 369, 604, 0 }, // 184, 457, 0 - (2,5)
    { ID_JUMPTO_SAVAGE_PLATE_JUKARI,    689, 775, 0 }, // 186, 457, 0 - (4,5)
    { ID_JUMPTO_SAVAGE_PLATE_TICHTICATL,619, 602, 0 }, // 187, 455, 0 - (5,3)
    { ID_JUMPTO_SAVAGE_PLATE_SAKKHRA,   103, 615, 0 }, // 182, 454, 0 - (0,2)
    { ID_JUMPTO_SAVAGE_PLATE_YOLARU,    534, 286, 0 }, // 187, 453, 0 - (5,1)
    { ID_JUMPTO_SAVAGE_PLATE_BARAKO,    376, 173, 0 }, // 183, 452, 0 - (1,0)
};

void CChildView::OnJumpTo(UINT id)
{
    for (int i = 0; i < sizeof(jmp_to_tbl) / sizeof(jmp_to_tbl[0]); i++)
    {
        if (id == jmp_to_tbl[i].id)
        {
            MoveToTile(jmp_to_tbl[i].x, jmp_to_tbl[i].y, jmp_to_tbl[i].z);
            return;
        }
    }
}