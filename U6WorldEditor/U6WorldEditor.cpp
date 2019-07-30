
// U6WorldEditor.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include <timeapi.h>
#include "afxwinappex.h"
#include "afxdialogex.h"
#include "U6WorldEditor.h"
#include "MainFrm.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CU6WorldEditorApp

BEGIN_MESSAGE_MAP(CU6WorldEditorApp, CWinApp)
    ON_COMMAND(ID_APP_ABOUT, &CU6WorldEditorApp::OnAppAbout)
END_MESSAGE_MAP()


// CU6WorldEditorApp construction

CU6WorldEditorApp::CU6WorldEditorApp()
{
    // support Restart Manager
    m_dwRestartManagerSupportFlags = AFX_RESTART_MANAGER_SUPPORT_RESTART;
#ifdef _MANAGED
    // If the application is built using Common Language Runtime support (/clr):
    //     1) This additional setting is needed for Restart Manager support to work properly.
    //     2) In your project, you must add a reference to System.Windows.Forms in order to build.
    System::Windows::Forms::Application::SetUnhandledExceptionMode(System::Windows::Forms::UnhandledExceptionMode::ThrowException);
#endif

    // TODO: replace application ID string below with unique ID string; recommended
    // format for string is CompanyName.ProductName.SubProduct.VersionInformation
    SetAppID(_T("U6WorldEditor.AppID.NoVersion"));

    // TODO: add construction code here,
    // Place all significant initialization in InitInstance
}

// The one and only CU6WorldEditorApp object

CU6WorldEditorApp theApp;


// CU6WorldEditorApp initialization

BOOL CU6WorldEditorApp::InitInstance()
{
    // InitCommonControlsEx() is required on Windows XP if an application
    // manifest specifies use of ComCtl32.dll version 6 or later to enable
    // visual styles.  Otherwise, any window creation will fail.
    INITCOMMONCONTROLSEX InitCtrls;
    InitCtrls.dwSize = sizeof(InitCtrls);
    // Set this to include all the common control classes you want to use
    // in your application.
    InitCtrls.dwICC = ICC_WIN95_CLASSES;
    InitCommonControlsEx(&InitCtrls);

    CWinApp::InitInstance();


    // Initialize OLE libraries
    if (!AfxOleInit())
    {
        AfxMessageBox(IDP_OLE_INIT_FAILED);
        return FALSE;
    }

    AfxEnableControlContainer();

    EnableTaskbarInteraction(FALSE);

    // AfxInitRichEdit2() is required to use RichEdit control
    // AfxInitRichEdit2();

    // Standard initialization
    // If you are not using these features and wish to reduce the size
    // of your final executable, you should remove from the following
    // the specific initialization routines you do not need
    // Change the registry key under which our settings are stored
    // TODO: You should modify this string to be something appropriate
    // such as the name of your company or organization
    SetRegistryKey(_T("Local AppWizard-Generated Applications"));


    // To create the main window, this code creates a new frame window
    // object and then sets it as the application's main window object
    CMainFrame* pFrame = new CMainFrame;
    if (!pFrame)
        return FALSE;
    m_pMainWnd = pFrame;
    // create and load the frame with its resources
    pFrame->LoadFrame(IDR_MAINFRAME,
        WS_OVERLAPPEDWINDOW | FWS_ADDTOTITLE, NULL,
        NULL);





    // The one and only window has been initialized, so show and update it
    pFrame->ShowWindow(SW_SHOW);
    pFrame->UpdateWindow();
    return TRUE;
}

int CU6WorldEditorApp::ExitInstance()
{
    //TODO: handle additional resources you may have added
    AfxOleTerm(FALSE);

    return CWinApp::ExitInstance();
}

// CU6WorldEditorApp message handlers


// CAboutDlg dialog used for App About

class CAboutDlg : public CDialogEx
{
public:
    CAboutDlg();

// Dialog Data
#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_ABOUTBOX };
#endif

protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
    DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()

// App command to run the dialog
void CU6WorldEditorApp::OnAppAbout()
{
    CAboutDlg aboutDlg;
    aboutDlg.DoModal();
}

// CU6WorldEditorApp message handlers


// Number of frames with a delay of 0 ms before the
// animation thread yields to other running threads.
const int NO_DELAYS_PER_YIELD = 16;

const int MAX_STATS_INTERVAL = 1000000; // 1 second

inline DWORD GET_TIME()
{
    return timeGetTime() * 1000;
}

inline float TIME_TO_SECONDS(DWORD time)
{
    return (float)time / 1000000;
}

DWORD startTime = GET_TIME();
DWORD prevStatsTime = startTime;

INT32 beforeTime = startTime, afterTime, timeDiff, sleepTime;
INT32 overSleepTime = 0L;
INT32 noDelays = 0;
INT32 excess = 0L;

const DWORD FRAME_INTERVAL = 1000000/20;

BOOL CU6WorldEditorApp::OnIdle(LONG lCount)
{
    CWinApp::OnIdle(lCount);

    afterTime = GET_TIME();
    INT32 timeDiff = afterTime - beforeTime;

    if (timeDiff > FRAME_INTERVAL)
    {
        beforeTime += FRAME_INTERVAL;
        ((CMainFrame*)m_pMainWnd)->Update();
    }

    return TRUE;
}
