
// ChildView.h : interface of the CChildView class
//


#pragma once


// CChildView window

class CChildView : public CWnd
{
// Construction
public:
    CChildView();

// Attributes
public:

// Operations
public:
    void Update();

// Overrides
    protected:
    virtual BOOL PreCreateWindow(CREATESTRUCT& cs);

// Implementation
public:
    virtual ~CChildView();

    // Generated message map functions
protected:
    afx_msg void OnPaint();
    DECLARE_MESSAGE_MAP()
public:
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg BOOL OnEraseBkgnd(CDC* pDC);
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
    afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
    afx_msg void OnMouseMove(UINT nFlags, CPoint point);
    afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
    afx_msg void OnUpdateMapOriginText(CCmdUI* pCmdUI);
    afx_msg void OnGameType(UINT id);
    afx_msg void OnJumpTo(UINT id);
    afx_msg void OnViewZ(UINT id);
    afx_msg void OnHackConverse();

private:
    void MoveToTile(int xtile, int ytile, int z);

private:
    int     m_map_ox;
    int     m_map_oy;
    int     m_map_z;
    bool    m_mouse_caputred;
    CPoint  m_mouse_last_point;


public:
    afx_msg void OnHackTilemanager();
    afx_msg void OnHackHookdosbox();
    afx_msg void OnUpdateHackHookdosbox(CCmdUI *pCmdUI);
};

