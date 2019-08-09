#pragma once
#include "afxcmn.h"
#include "DynObj.h"
#include "CustomDrawListCtrl.h"

#include "MapManager.h"


class CTileListCtrl : public CCustomDrawListCtrl
{
public:
    CTileListCtrl() = default;

    BOOL SetItemTextEx(int nItem, int nCol, LPCTSTR format, ...)
    {
        va_list args;
        va_start(args, format);

        int nBuf;
        TCHAR szBuffer[512];

        nBuf = _vstprintf_s(szBuffer, format, args);
        ASSERT(nBuf < (sizeof(szBuffer) / sizeof(szBuffer[0])));
        va_end(args);

        return SetItemText(nItem, nCol, szBuffer);
    }

private:
    virtual bool IsNotifyItemDraw() { return true; }
    virtual bool IsNotifySubItemDraw(int /*nItem*/, UINT /*nState*/, LPARAM /*lParam*/) { return true; }
    virtual bool IsSubItemDraw(int /*nItem*/, int /*nSubItem*/, UINT /*nState*/, LPARAM /*lParam*/) { return true; }
    virtual bool OnSubItemDraw(CDC* pDC, int nItem, int nSubItem, UINT nState, LPARAM lParam)
    {
        if (nSubItem == 1)
        {
            CRect rc;
            GetSubItemRect(nItem, nSubItem, LVIR_BOUNDS, rc);
            pDC->FillSolidRect(&rc, RGB(0, 0, 0));

            int x = rc.left + (rc.Width() - 16) / 2;
            int y = rc.top + (rc.Height() - 16) / 2;

            int x_src = (nItem % 32) * 16;
            int y_src = (nItem / 32) * 16;

            ::SetDIBitsToDevice(pDC->GetSafeHdc(),
                x, y, 16, 16,
                x_src, 1024 - y_src - 1 - 16, // check MSDN, The origin of a bottom-up DIB is the lower-left corner of the bitmap;
                0, 1024,
                m_tiles_dib.bits(),
                (LPBITMAPINFO)m_tiles_dib.bitmap_info(),
                DIB_RGB_COLORS);

            return true;
        }
        else
            return false;
    }

public:
    DibSection m_tiles_dib;
};

// CTileManagerDlg dialog

class CTileManagerDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CTileManagerDlg)

public:
	CTileManagerDlg(MapManager* mm, CWnd* pParent = NULL);   // standard constructor
	virtual ~CTileManagerDlg();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_TILE_MANAGER };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

    DECLARE_OBJ_MAP();
    afx_msg void OnMove(int, int);                      // ON_WM_MOVE()
    afx_msg void OnSize(UINT, int, int);                // ON_WM_SIZE()
    afx_msg void OnGetMinMaxInfo(MINMAXINFO* lpMMI);    // ON_WM_GETMINMAXINFO()

	DECLARE_MESSAGE_MAP()
public:
    MapManager& m_map_manager;

    CTileListCtrl m_lcTiles;
    CImageList m_ilTiles;
    virtual BOOL OnInitDialog();
};
