
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

#include "ConverseDlg.h"
#include "TileManagerDlg.h"

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

HANDLE          ghDosBox = nullptr;
const uint8_t*  gNamePtr = nullptr;
const uint8_t*  gPosPtr = nullptr;
const uint8_t*  gHpPtr = nullptr;
const uint8_t*  gLvlPtr = nullptr;
const uint8_t*  gStrPtr = nullptr;

BOOL EnablePrivilege(BOOL enable)
{
    HANDLE hToken = NULL;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY | TOKEN_READ, &hToken))
        return FALSE;

    LUID luid;
    if (!LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &luid))
        return FALSE;

    TOKEN_PRIVILEGES tp = {};
    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    tp.Privileges[0].Attributes = enable ? SE_PRIVILEGE_ENABLED : 0;
    if (!AdjustTokenPrivileges(hToken, FALSE, &tp, 0, NULL, NULL))
        return FALSE;

    CloseHandle(hToken);

    return TRUE;
}

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
    // if needed, uncomment the line below
    //EnablePrivilege(true);

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

    // EXPERIMENT:
    /*auto script = gMapManager.script.get_script(2);
    std::ofstream file("d:\\script002.bin", std::ios_base::out | std::ios_base::binary);
    file.write((const char*)script.data(), script.size());
    file.close();
    */

    for (int i = 0; i < 256; i++)
    {
        auto name = gMapManager.script.get_npc_name(i);
        TRACE("%3d: %s\n", i, name.c_str());
    }

    // EXPERIMENT:
    // save npc's formatted script
    /*
    ScriptInterpreter i;
    i.load(gMapManager.script.get_script(7));
    auto formatted = i.format_script();
    std::ofstream f("d:\\script.txt", std::ios_base::out | std::ios_base::binary);
    f.write(formatted.c_str(), formatted.length());
    f.close();
    */
}

CChildView::~CChildView()
{
    DrawDibClose(gDD);

    if (ghDosBox)
    {
        CloseHandle(ghDosBox);
        ghDosBox = nullptr;
    }
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
    ON_COMMAND(ID_HACK_CONVERSE, &CChildView::OnHackConverse)
    ON_COMMAND(ID_HACK_TILEMANAGER, &CChildView::OnHackTilemanager)
    ON_COMMAND(ID_HACK_HOOKDOSBOX, &CChildView::OnHackHookdosbox)
    ON_UPDATE_COMMAND_UI(ID_HACK_HOOKDOSBOX, &CChildView::OnUpdateHackHookdosbox)
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

    {
        //
        // dosbox hack
        //
        if (ghDosBox != nullptr && gPosPtr != nullptr)
        {
            std::vector<uint8_t> buffer;
            buffer.resize(3 * 256);
            SIZE_T bytes_read;
            ReadProcessMemory(ghDosBox, gPosPtr, buffer.data(), buffer.size(), &bytes_read);
            if (bytes_read == buffer.size())
            {
                auto p = buffer.data();
                for (int i = 0; i < 256; i++)
                {
                    int b1 = *p;
                    int b2 = *(p + 1);
                    int b3 = *(p + 2);
                    int x = (((b2 & 0x03) << 8) | b1);                  // 10bits = 0 - 1023
                    int y = (((b3 & 0x0f) << 6) | ((b2 & 0xfc) >> 2));  // 10bits = 0 - 1023
                    int z = ((b3 & 0xf0) >> 4);                         // 4bits = 0 - 15
                    if (z >= 0 && z <= 5)
                    {
                        gMapManager.obj_manager.set_actor_position(i, p);
                    }
                    p += 3;
                }

                auto main_actor = gMapManager.obj_manager.get_actor(1);
                if (main_actor.z == 0)
                {
                    MoveToTile(main_actor.x % 1024, main_actor.y % 1024, main_actor.z);
                }
                else
                {
                    MoveToTile(main_actor.x %256, main_actor.y % 256, main_actor.z);
                }
            }
        }

        if (ghDosBox != nullptr && gLvlPtr != nullptr && gStrPtr != nullptr)
        {
            auto game_type = gConfig.get_property("game_type");
            if (game_type == "u6")
            {
                uint8_t hp;
                uint8_t level;
                SIZE_T bytes_read, bytes_written;
                ReadProcessMemory(ghDosBox, gHpPtr, &hp, 1, &bytes_read);
                ReadProcessMemory(ghDosBox, gLvlPtr, &level, 1, &bytes_read);
                int max_hp = level * 30;
                if (max_hp > 255)
                    max_hp = 255;
                if (hp < max_hp)
                {
                    hp = (uint8_t)max_hp;

                    BOOL b;
                    unsigned long oldProtect;
                    // need PROCESS_VM_OPERATION
                    b = VirtualProtectEx(ghDosBox, (void*)gHpPtr, 1, PAGE_EXECUTE_READWRITE, &oldProtect);
                    // need PROCESS_VM_WRITE
                    b = WriteProcessMemory(ghDosBox, (void*)gHpPtr, &hp, 1, &bytes_written);
                    b = VirtualProtectEx(ghDosBox, (void*)gHpPtr, 1, oldProtect, &oldProtect);
                }
            }
        }
    }

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
        //gMapManager.tile_image.draw(gScreen, x, y, i); // no animation
        gMapManager.tile_manager.draw(gScreen, x, y, i); // with animation
    }

    // draw the character font
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
    CString obj_flags;
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

        auto& ti = obj->tile_info;
        obj_flags.Format(_T("%d, %02x-%02x-%02x"), obj->obj_number, ti.flag1, ti.flag2, ti.flag3);
    }

    CString s;
    s.Format(_T("%d, %d, %hs\n%hs\n%s"), cur_tile_x, cur_tile_y, obj_desc.c_str(), actor_name.c_str(), obj_flags );
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
    { 956, 235, 0 },{ 20, 219, 2 },
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
        auto& tile_info = obj->tile_info;
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

    { ID_JUMPTO_SAVAGE_PLATE_BARRAB,    206, 619, 0 }, // 182, 456, 0 - (0,4) in the big plate
    { ID_JUMPTO_SAVAGE_PLATE_DISQUIQUI, 369, 604, 0 }, // 184, 457, 0 - (2,5)
    { ID_JUMPTO_SAVAGE_PLATE_JUKARI,    689, 775, 0 }, // 186, 457, 0 - (4,5)
    { ID_JUMPTO_SAVAGE_PLATE_TICHTICATL,619, 602, 0 }, // 187, 455, 0 - (5,3)
    { ID_JUMPTO_SAVAGE_PLATE_SAKKHRA,   103, 615, 0 }, // 182, 454, 0 - (0,2)
    { ID_JUMPTO_SAVAGE_PLATE_YOLARU,    534, 286, 0 }, // 187, 453, 0 - (5,1)
    { ID_JUMPTO_SAVAGE_PLATE_BARAKO,    376, 173, 0 }, // 183, 452, 0 - (1,0)

    { ID_CITY_ELYSIUM20N114E,           0, 0, 0 }, // 20N 114E
    { ID_CITY_HELLAS27S77E,             0, 0, 0 }, // 27S 77E
    { ID_CITY_OLYMPUS10N110W,           707, 500, 0 }, // 10N 110W
    { ID_CITY_ARGYRE30S107W,            0, 0, 0 }, // 30S 107W
    { ID_MARTIAN_LANDINGSITE27S146E,    413, 628, 0 },
    { ID_MARTIAN_EARLIERLANDING28S153W, 597, 636, 0 },
    { ID_MARTIAN_ARSIAMONS,             658, 565, 0 },
    { ID_MARTIAN_NOCTIS,                722, 597, 0 },
    { ID_MARTIAN_COOTER,                795, 563, 0 },
    { ID_MARTIAN_SYRTISMAJOR10N71E,     0, 0, 0},
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




void CChildView::OnHackConverse()
{
    CConverseDlg dlg(&gMapManager);
    if (IDC_FIND_NPC == dlg.DoModal() && dlg.m_selected_npc_id != -1)
    {
        auto actor = gMapManager.obj_manager.get_actor(dlg.m_selected_npc_id);
        if (actor.z <= 5)
        {
            MoveToTile(actor.x, actor.y, actor.z);
        }
        else
        {
            AfxMessageBox(_T("NPC is not on the map!!!"), MB_OK);
        }
    }
}

void CChildView::OnHackTilemanager()
{
    CTileManagerDlg dlg(&gMapManager);
    dlg.DoModal();
}








#include <psapi.h>

DWORD FindProcessId(const std::string& process_name)
{
    int nCurIndex = 0;
    DWORD aProcesses[1024];
    DWORD cbNeeded;

    // Get the list of process identifiers.
    if (!EnumProcesses(aProcesses, sizeof(aProcesses), &cbNeeded))
    {
        return 0;
    }

    // Calculate how many process identifiers were returned.
    DWORD cProcesses = cbNeeded / sizeof(DWORD);

    DWORD found_pid = 0;
    for (DWORD pi = 0; pi < cProcesses && found_pid == 0; pi++)
    {
        HANDLE hProcess;

        auto pid = aProcesses[pi];
        if (pid == 0)
            continue;

        // Get a handle to the process.
        hProcess = OpenProcess(PROCESS_QUERY_INFORMATION |
            PROCESS_VM_READ,
            FALSE, pid);
        if (NULL == hProcess)
        {
            continue;
        }

        char path[MAX_PATH];
        DWORD path_length = MAX_PATH;
        if (QueryFullProcessImageNameA(hProcess, 0, path, &path_length) && path_length >= process_name.length())
        {
            if (0 == _stricmp(process_name.c_str(), &path[path_length - process_name.length()]))
            {
                found_pid = pid;
            }
        }

        // Release the handle to the process.
        CloseHandle(hProcess);
    }

    return found_pid;
}

std::vector<uint8_t> get_pattern(const std::string& data, const std::string& type)
{
    char* endptr;
    std::vector<uint8_t> pattern;
    if (type == "int8")
    {
        int8_t value = (int8_t)strtol(data.c_str(), &endptr, 0);
        pattern.push_back(*(uint8_t*)&value);
    }
    else if (type == "uint8")
    {
        uint8_t value = (uint8_t)strtoul(data.c_str(), &endptr, 0);
        pattern.push_back(value);
    }
    else if (type == "int16")
    {
        int16_t value = (int16_t)strtol(data.c_str(), &endptr, 0);
        pattern.push_back(((uint8_t*)&value)[0]);
        pattern.push_back(((uint8_t*)&value)[1]);
    }
    else if (type == "uint16")
    {
        uint16_t value = (uint16_t)strtoul(data.c_str(), &endptr, 0);
        pattern.push_back(((uint8_t*)&value)[0]);
        pattern.push_back(((uint8_t*)&value)[1]);
    }
    else if (type == "int32")
    {
        int32_t value = (int32_t)strtol(data.c_str(), &endptr, 0);
        pattern.push_back(((uint8_t*)&value)[0]);
        pattern.push_back(((uint8_t*)&value)[1]);
        pattern.push_back(((uint8_t*)&value)[2]);
        pattern.push_back(((uint8_t*)&value)[3]);
    }
    else if (type == "uint32")
    {
        uint32_t value = (uint32_t)strtoul(data.c_str(), &endptr, 0);
        pattern.push_back(((uint8_t*)&value)[0]);
        pattern.push_back(((uint8_t*)&value)[1]);
        pattern.push_back(((uint8_t*)&value)[2]);
        pattern.push_back(((uint8_t*)&value)[3]);
    }
    else if (type == "data")
    {
        for (int i = 0; i < data.length(); i += 2)
        {
            uint8_t value = (uint8_t)strtoul(data.substr(i, 2).c_str(), &endptr, 16);
            pattern.push_back(value);
        }
    }
    else
    {
        // treat it as string
        std::for_each(data.begin(), data.end(), [&pattern](auto c) { pattern.push_back(*(uint8_t*)&c); });
    }

    return pattern;
}

template <class InIter1, class InIter2>
std::vector<uint8_t*> find_all(uint8_t *base, InIter1 buf_start, InIter1 buf_end, InIter2 pat_start, InIter2 pat_end)
{
    std::vector<uint8_t*> out;

    for (InIter1 pos = buf_start;
        buf_end != (pos = std::search(pos, buf_end, pat_start, pat_end));
        ++pos)
    {
        out.push_back(base + (pos - buf_start));
    }

    return out;
}

std::vector<uint8_t*> InitSearch(HANDLE hProcess, const std::string& init_value, const std::string& type)
{
    std::vector<uint8_t*> ret;

    auto pattern = get_pattern(init_value, type);

    SYSTEM_INFO sys_info;
    GetSystemInfo(&sys_info);

    auto proc_cur_address = (uint8_t*)sys_info.lpMinimumApplicationAddress;
    auto proc_max_address = (uint8_t*)sys_info.lpMaximumApplicationAddress;

    std::vector<uint8_t> buffer;

    while (proc_cur_address < proc_max_address)
    {
        MEMORY_BASIC_INFORMATION mbi;
        SIZE_T size = VirtualQueryEx(hProcess, proc_cur_address, &mbi, sizeof(mbi));
        if (size == 0)
        {
            break;
        }

        if (mbi.Protect == PAGE_READWRITE &&
            mbi.State == MEM_COMMIT &&
            (mbi.Type == MEM_MAPPED || mbi.Type == MEM_PRIVATE))
        {
            SIZE_T bytes_read;
            buffer.resize(mbi.RegionSize);
            ReadProcessMemory(hProcess, proc_cur_address, buffer.data(), mbi.RegionSize, &bytes_read);
            buffer.resize(bytes_read);

            auto results = find_all(proc_cur_address, buffer.begin(), buffer.end(), pattern.begin(), pattern.end());
            uint32_t count = (uint32_t)results.size();
            if (count > 0)
            {
                std::for_each(results.begin(), results.end(), [&ret](auto d) {
                    ret.push_back(d);
                });
            }
        }

        proc_cur_address += mbi.RegionSize;
    }

    return ret;
}




void CChildView::OnHackHookdosbox()
{
    Actor main_actor, actor2, actor3;
    std::vector<PBYTE> r;
    std::vector<uint8_t> buffer;
    SIZE_T bytes_read;

    if (ghDosBox != nullptr)
    {
        // close the handle which opend
        goto exit;
    }

    // reset
    gNamePtr = nullptr;
    gPosPtr = nullptr;
    gHpPtr = nullptr;
    gLvlPtr = nullptr;
    gStrPtr = nullptr;

    auto dosbox_pid = FindProcessId("dosbox.exe");
    if (dosbox_pid == 0)
    {
        AfxMessageBox(_T("Can't find dosbox.exe is running!!!"), MB_OK);
        goto exit;
    }

    DWORD dwDesiredAccess = PROCESS_VM_READ | PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION | PROCESS_VM_WRITE;
    ghDosBox = OpenProcess(
        dwDesiredAccess,
        false,
        dosbox_pid);

    if (ghDosBox == NULL)
    {
        AfxMessageBox(_T("Can't open the process dosbox.exe!!!"), MB_OK);
        goto exit;
    }

    main_actor = gMapManager.obj_manager.get_actor(1);
    actor2 = gMapManager.obj_manager.get_actor(2);
    actor3 = gMapManager.obj_manager.get_actor(3);

    if (gNamePtr == nullptr)
    {
        r = InitSearch(ghDosBox, main_actor.name, "string");
        if (r.size() == 0)
        {
            AfxMessageBox(_T("Can't find the main actor name in the memory!!!"), MB_OK);
            goto exit;
        }

        gNamePtr = r[0];

        //OBSOLETE:
        //const uint8_t* name_ptr = (uint8_t*)0x000000000E50D5A4;
        //const uint8_t* pos_ptr = (uint8_t*)0x000000000E50FD84;
        //gPosPtr = r[0] + (pos_ptr - name_ptr);

        // find Position
        {
            uint8_t a1_b1 = (main_actor.x & 0x0ff); // x: bit 0 - 7
            uint8_t a1_b2 = ((main_actor.x & 0x300) >> 8) | ((main_actor.y & 0x03f) << 2); // x: bit 8, 9 | y: bit 0 - 5
            uint8_t a1_b3 = ((main_actor.y & 0x3c0) >> 6) | ((main_actor.z & 0x0f) << 4); // y: bit 6 - 9 | z: bit 0 - 3

            uint8_t a2_b1 = (actor2.x & 0x0ff); // x: bit 0 - 7
            uint8_t a2_b2 = ((actor2.x & 0x300) >> 8) | ((actor2.y & 0x03f) << 2); // x: bit 8, 9 | y: bit 0 - 5
            uint8_t a2_b3 = ((actor2.y & 0x3c0) >> 6) | ((actor2.z & 0x0f) << 4); // y: bit 6 - 9 | z: bit 0 - 3

            uint8_t a3_b1 = (actor3.x & 0x0ff); // x: bit 0 - 7
            uint8_t a3_b2 = ((actor3.x & 0x300) >> 8) | ((actor3.y & 0x03f) << 2); // x: bit 8, 9 | y: bit 0 - 5
            uint8_t a3_b3 = ((actor3.y & 0x3c0) >> 6) | ((actor3.z & 0x0f) << 4); // y: bit 6 - 9 | z: bit 0 - 3

            char buf[32];
            sprintf_s(buf, "%02x%02x%02x%02x%02x%02x%02x%02x%02x", a1_b1, a1_b2, a1_b3, a2_b1, a2_b2, a2_b3, a3_b1, a3_b2, a3_b3);
            auto s = InitSearch(ghDosBox, buf, "data");
            if (s.size() == 1)
            {
                gPosPtr = s[0];
            }
            else
            {
                AfxMessageBox(_T("Can't find the positioin list in the memory!!!"), MB_OK);
            }
        }

        // find HP
        {
            char buf[32];
            std::string test;
            for (int i = 1; i <= 16; i++)
            {
                auto a = gMapManager.obj_manager.get_actor(i);
                sprintf_s(buf, "%02x", *a.hp);
                test += buf;
            }

            auto t = InitSearch(ghDosBox, test, "data");
            if (t.size() == 1)
            {
                gHpPtr = t[0];
            }
            else
            {
                AfxMessageBox(_T("Can't find HP list!!!"), MB_OK);
            }
        }

        // find Level
        {
            char buf[32];
            std::string test;
            for (int i = 1; i <= 16; i++)
            {
                auto a = gMapManager.obj_manager.get_actor(i);
                sprintf_s(buf, "%02x", *a.level);
                test += buf;
            }

            auto u = InitSearch(ghDosBox, test, "data");
            if (u.size() == 1)
            {
                gLvlPtr = u[0];
            }
            else
            {
                AfxMessageBox(_T("Can't find Level list!!!"), MB_OK);
            }
        }

        // find Strength
        {
            char buf[32];
            std::string test;
            for (int i = 1; i <= 16; i++)
            {
                auto a = gMapManager.obj_manager.get_actor(i);
                sprintf_s(buf, "%02x", *a.strength);
                test += buf;
            }

            auto v = InitSearch(ghDosBox, test, "data");
            if (v.size() == 1)
            {
                gStrPtr = v[0];
            }
            else
            {
                AfxMessageBox(_T("Can't find Strength list!!!"), MB_OK);
            }
        }
    }

    // check if the actor name is correct
    buffer.resize(16);
    ReadProcessMemory(ghDosBox, gNamePtr, buffer.data(), buffer.size(), &bytes_read);
    if (bytes_read != buffer.size())
    {
        AfxMessageBox(_T("Can't get the correct name data!!!"), MB_OK);
        goto exit;
    }
    if (main_actor.name != (const char*)buffer.data())
    {
        AfxMessageBox(_T("Name is not same!!! data corrupted!!!"), MB_OK);
        goto exit;
    }

    buffer.resize(3 * 256);
    ReadProcessMemory(ghDosBox, gPosPtr, buffer.data(), buffer.size(), &bytes_read);
    if (bytes_read != buffer.size())
    {
        AfxMessageBox(_T("Can't get the correct position data!!!"), MB_OK);
        goto exit;
    }

    // we got what we want
    auto p = buffer.data();
    for (int i = 0; i < 256; i++)
    {
        int b1 = *p;
        int b2 = *(p + 1);
        int b3 = *(p + 2);
        int x = (((b2 & 0x03) << 8) | b1);                  // 10bits = 0 - 1023
        int y = (((b3 & 0x0f) << 6) | ((b2 & 0xfc) >> 2));  // 10bits = 0 - 1023
        int z = ((b3 & 0xf0) >> 4);                         // 4bits = 0 - 15
        if (z >= 0 && z <= 5)
        {
            gMapManager.obj_manager.set_actor_position(i, p);
        }
        p += 3;
    }

    main_actor = gMapManager.obj_manager.get_actor(1);
    MoveToTile(main_actor.x, main_actor.y, main_actor.z);

    return;

exit:
    if (ghDosBox != nullptr)
    {
        CloseHandle(ghDosBox);
    }
    ghDosBox = nullptr;
}


void CChildView::OnUpdateHackHookdosbox(CCmdUI *pCmdUI)
{
    pCmdUI->SetCheck(ghDosBox != nullptr);
}
