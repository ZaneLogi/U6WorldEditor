
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
#include "DibUtils.h"
#include "ColorPalette.h"
#include "TileImage.h"
#include "TileManager.h"
#include "MapManager.h"
#include "U6Objects.h"

#pragma comment(lib,"vfw32.lib")

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

/*
Operation:
1. move the map around
drag the map using the mouse left button

2. move the object
select the object: the ctrl key + the mouse left button
move the object: the ctrl key + the mouse left button after an object is selected.

3. mouse right button click
use ladders
*/


HDRAWDIB gDD;
Configuration   gConfig;
DibSection      gScreen;
MapManager      gMapManager;
Obj*            gSelectedObj = nullptr;

inline int normalize(int value, int boundary)
{
    value %= boundary;
    if (value < 0)
        value += boundary;
    return value;
}


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

    // EXPERIMENT:
    // save game data
    //gMapManager.obj_manager.save_objblks("d:\\test\\test");
    /*
    *actor.strength = 99;
    *actor.dexterity = 99;
    *actor.intelligence = 99;
    *actor.hp = 99;
    gMapManager.obj_manager.save_actors("d:\\test\\test\\OBJLIST");
    */

    for (int i = 0; i < 256; i++)
    {
        auto name = gMapManager.script.get_npc_name(i);
        TRACE("%3d: %s\n", i, name.c_str());
    }
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
    ON_WM_RBUTTONDOWN()
    ON_WM_SIZE()
    ON_WM_ERASEBKGND()
    ON_UPDATE_COMMAND_UI(ID_INDICATOR_MAP_ORIGIN, CChildView::OnUpdateMapOriginText)
    ON_COMMAND_RANGE(ID_GAME_U6, ID_GAME_MARTIAN, CChildView::OnGameType)
    ON_COMMAND_RANGE(ID_JUMPTO_BEGIN, ID_JUMPTO_END, CChildView::OnJumpTo)
    ON_COMMAND_RANGE(ID_VIEW_SURFACE, ID_VIEW_DUNGEON5, CChildView::OnViewZ)
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

    CRect rcClient;
    GetClientRect(&rcClient);

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

    for (int i = 0; i < 256; i++)
    {
        int x = (i % 16) * 8 + 640 - 128;
        int y = (i / 16) * 8 + 128;

        gMapManager.text.draw_char(gScreen, i, x, y, 0x48);
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

    if (gSelectedObj)
    {
        int screen_x = xoffset + rcClient.Width() - 32;
        int screen_y = yoffset;
        auto p = gScreen.bits(yoffset) + screen_x;
        FillRect(p, 32, 32, 0, 1, gScreen.ypitch());
        gMapManager.tile_manager.draw(gScreen, screen_x + 16, screen_y + 16, gSelectedObj->obj_number, gSelectedObj->obj_frame, false);
        gMapManager.tile_manager.draw(gScreen, screen_x + 16, screen_y + 16, gSelectedObj->obj_number, gSelectedObj->obj_frame, true);
    }

    DrawDibDraw(gDD, dc.GetSafeHdc(), -xoffset, -yoffset, width, height,
        (BITMAPINFOHEADER*)&gScreen.bitmap_info()->bmiHeader,
        gScreen.bits(), 0, 0, width, height, NULL);

    int cur_tile_x = (m_mouse_last_point.x + m_map_ox) / 16;
    int cur_tile_y = (m_mouse_last_point.y + m_map_oy) / 16;
    if (m_map_z == 0)
    {
        cur_tile_x %= 1024;
        cur_tile_y %= 1024;
    }
    else
    {
        cur_tile_x %= 256;
        cur_tile_y %= 256;
    }

    Obj* obj = gMapManager.obj_manager.get_obj(cur_tile_x, cur_tile_y, m_map_z);
    std::string obj_desc;
    std::string actor_name;
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

        if (obj->type() == Obj::Type::ACTOR)
        {
            actor_name = gMapManager.script.get_npc_name(((Actor*)obj)->id);
        }
    }

    CString s;
    s.Format(_T("%d, %d, %hs\n%hs"), cur_tile_x, cur_tile_y, obj_desc.c_str(), actor_name.c_str() );
    dc.SelectStockObject(SYSTEM_FIXED_FONT);
    dc.SelectStockObject(BLACK_PEN);
    CRect rc(0, 0, 640, 640);
    dc.DrawText(s, &rc, DT_LEFT | DT_EXTERNALLEADING | DT_WORDBREAK);

#endif



}

void CChildView::MoveToTile(int xtile, int ytile, int z)
{
    assert(z == 0 ? (0 <= xtile && xtile < 1024) : (0 <= xtile && xtile < 256));
    assert(z == 0 ? (0 <= ytile && ytile < 1024) : (0 <= ytile && ytile < 256));

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

    m_map_ox = normalize(m_map_ox, world_pixels);
    m_map_oy = normalize(m_map_oy, world_pixels);
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
        int width =  ((cx + 15) & (~0x0f)) + 16 + 16; // ((cx + 15) / 16) * 16 + 16 (for screen buffer) + 16 (for double-width objects)
        int height = ((cy + 15) & (~0x0f)) + 16 + 16;
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
    if (GetKeyState(VK_CONTROL) & 0x8000)
    {
        int cur_tile_x = (point.x + m_map_ox) / 16;
        int cur_tile_y = (point.y + m_map_oy) / 16;
        int world_tiles = (m_map_z == 0 ? 1024 : 256);
        cur_tile_x = normalize(cur_tile_x, world_tiles);
        cur_tile_y = normalize(cur_tile_y, world_tiles);

        if (!gSelectedObj)
            gSelectedObj = gMapManager.obj_manager.get_obj(cur_tile_x, cur_tile_y, m_map_z);
        else
        {
            gMapManager.obj_manager.move_obj_to(*gSelectedObj, cur_tile_x, cur_tile_y, m_map_z);
            gSelectedObj = nullptr;
        }
    }
    else
    {
        m_mouse_caputred = true;
        m_mouse_last_point = point;
        SetCapture();
    }
}

void CChildView::OnLButtonUp(UINT nFlags, CPoint point)
{
    if (m_mouse_caputred)
    {
        m_mouse_caputred = false;
        ReleaseCapture();
    }
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

        m_map_ox = normalize(m_map_ox, boundary);
        m_map_oy = normalize(m_map_oy, boundary);

        m_mouse_last_point = point;
        Update();
    }

    m_mouse_last_point = point;
}

struct { int x; int y; int z; int q; }
SE_PLATE[] =
{
    { 182, 456, 0, 1 },
    { 184, 457, 0, 2 },
    { 186, 457, 0, 3 },
    { 187, 455, 0, 4 },
    { 182, 454, 0, 5 },
    { 187, 453, 0, 6 },
    { 183, 452, 0, 7 },
    { 206, 619, 0, 8 },
    { 369, 604, 0, 9 },
    { 689, 775, 0, 10 },
    { 619, 602, 0, 11 },
    { 103, 615, 0, 12 },
    { 534, 286, 0, 13 },
    { 376, 173, 0, 14 },

    { 255,   7, 3, 1 },
    { 197,  55, 3, 1 },
    { 245,   4, 3, 2 },
    { 200,  38, 3, 2 },
    { 169, 174, 3, 3 },
    { 170, 202, 3, 3 },
    { 143,  23, 3, 4 },
    { 144, 129, 3, 4 },
    { 146, 129, 3, 5 },
    { 229,  38, 3, 5 },
    { 244, 129, 3, 6 },
    { 146, 131, 3, 6 },
    { 226, 228, 3, 7 },
    { 146, 133, 3, 7 },
    { 144, 133, 3, 8 },
    { 147, 212, 3, 8 },
    { 142, 133, 3, 9 },
    { 68, 197, 3, 9 },
    { 255,   2, 3, 10 },
    { 119, 223, 3, 10 },
    { 85,  38, 3, 11 },
    { 142, 129, 3, 11 },
};

struct { int x; int y; int z; }
SE_CAVE[] =
{
    { 108, 605, 0 },{ 212, 189, 2 }, // SAKKHRA
    { 124, 597, 0 },{ 228, 181, 2 },
    { 140, 589, 0 },{ 244, 173, 2 },
    { 685, 373, 0 },{ 13, 245, 2 }, // URALI
    { 821, 349, 0 },{ 149, 237, 2 },
    { 986, 412, 0 },{ 74, 196, 2 },
    { 956, 235, 0 },{ 20, 218, 2 },
    { 845, 653, 0 },{ 141,  61, 2 }, // HAAKUR
    { 861, 661, 0 },{ 159,  69, 2 },
    { 877, 653, 0 },{ 173,  61, 2 },
    { 893, 661, 0 },{ 189,  69, 2 },
    { 905, 748, 0 },{ 209,  92, 2 }, // LAVA
    { 929, 844, 0 },{ 177, 132, 2 }, // WEB
    { 293,  67, 0 },{ 45, 131, 2 }, // near Barako
    { 272,  68, 0 },{ 25, 132, 2 },
    { 497,  76, 0 },{ 81, 148, 2 }, // fritz

    { 192, 460, 0 },{ 157, 137, 3 }, // hidden city Kotl
    { 243, 204, 1 },{ 234,  61, 2 }, // myrmidexdron hole
};

void CChildView::OnRButtonDown(UINT nFlags, CPoint point)
{
    int cur_tile_x = (point.x + m_map_ox) / 16;
    int cur_tile_y = (point.y + m_map_oy) / 16;
    int world_tiles = (m_map_z == 0 ? 1024 : 256);
    cur_tile_x = normalize(cur_tile_x, world_tiles);
    cur_tile_y = normalize(cur_tile_y, world_tiles);

    const int TILE_U6_DUNGEON = 1196;
    const int TILE_U6_CAVE = 1198;

    auto obj = gMapManager.obj_manager.get_obj(cur_tile_x, cur_tile_y, m_map_z);
    if (obj)
    {
        auto tile_info = gMapManager.tile_manager.get_info(obj->obj_number, obj->obj_frame);
        auto tile_index = tile_info.index;

        auto game_type = gConfig.get_property("game_type");

        if (gConfig.get_property("game_type") == "u6" &&
            (obj->obj_number == OBJ_U6_LADDER ||
                tile_index == TILE_U6_CAVE ||
                tile_index == TILE_U6_CAVE + 1 ||
                tile_index == TILE_U6_DUNGEON ||
                tile_index == TILE_U6_DUNGEON + 1))
        {
            if (obj->obj_frame == 0
                || (obj->z == 0 && (tile_index == TILE_U6_CAVE
                    || tile_index == TILE_U6_CAVE + 1 || tile_index == TILE_U6_DUNGEON
                    || tile_index == TILE_U6_DUNGEON + 1))) // DOWN
            {
                if (obj->z == 0) //handle the transition from the surface to the first dungeon level
                {
                    // note
                    // surface => 8 x 16 = 128 chunks for one direction
                    // dungeon => 32 chunks for one direction
                    // 128 / 32 = 4 for one direction => ratio 4 : 1
                    // 4 x 4 surface chunks map to one dungeon chunk
                    int nxtile = (obj->x & 0x07) | ((obj->x >> 2) & 0xF8);
                    int nytile = (obj->y & 0x07) | ((obj->y >> 2) & 0xF8);
                    MoveToTile(nxtile, nytile, 1);
                }
                else //dungeon ladders line up so we simply drop straight down
                {
                    MoveToTile(obj->x, obj->y, obj->z + 1);
                }
            }
            else // UP
            {
                if (obj->z == 1) //we use obj->quality to tell us which surface chunk to come up in.
                {
                    int nxtile = obj->x / 8 * 8 * 4 + ((obj->quality & 0x03) * 8) + (obj->x - obj->x / 8 * 8);
                    int nytile = obj->y / 8 * 8 * 4 + (((obj->quality >> 2) & 0x03) * 8) + (obj->y - obj->y / 8 * 8);
                    MoveToTile(nxtile, nytile, 0);
                }
                else
                {
                    MoveToTile(obj->x, obj->y, obj->z - 1);
                }
            }
            return;
        }

        if (game_type == "se")
        {
            if (obj->obj_number == 68)
            { // floor plate
                for (int i = 0; i < sizeof(SE_PLATE) / sizeof(SE_PLATE[0]); i++)
                {
                    if (obj->x == SE_PLATE[i].x && obj->y == SE_PLATE[i].y && obj->z == SE_PLATE[i].z
                        && obj->quality == SE_PLATE[i].q)
                    {
                        int jumpIndex = 0;
                        if (obj->z == 0)
                        {
                            jumpIndex = i + 7;
                            jumpIndex %= 14;
                        }
                        else
                        {
                            jumpIndex = i;
                            int n = jumpIndex % 2;
                            jumpIndex -= n;
                            n = 1 - n;
                            jumpIndex += n;
                        }

                        MoveToTile(SE_PLATE[jumpIndex].x, SE_PLATE[jumpIndex].y, SE_PLATE[jumpIndex].z);
                        break;
                    }
                }
            }
            else // SE_CAVE
            {
                for (int i = 0; i < sizeof(SE_CAVE) / sizeof(SE_CAVE[0]); i++)
                {
                    if (obj->x == SE_CAVE[i].x && obj->y == SE_CAVE[i].y && obj->z == SE_CAVE[i].z)
                    {
                        int r = i % 2;
                        i -= r;
                        i += 1 - r;
                        MoveToTile(SE_CAVE[i].x, SE_CAVE[i].y, SE_CAVE[i].z);
                        break;
                    }
                }
            }
        }
    }
}

void CChildView::OnUpdateMapOriginText(CCmdUI* pCmdUI)
{
    CString s;
    s.Format(_T("%d:%d:%d"), m_map_ox, m_map_oy, m_map_z);
    pCmdUI->SetText(s);
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

    gSelectedObj = nullptr;

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

void CChildView::OnViewZ(UINT id)
{
    int z = id - ID_VIEW_SURFACE;
    if (z == 0)
    {
        MoveToTile(511, 511, 0);
    }
    else
    {
        MoveToTile(127, 127, z);
    }
}


