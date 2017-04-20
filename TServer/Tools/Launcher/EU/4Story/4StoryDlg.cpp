// 4StoryDlg.cpp : ���� ����
//

#include "stdafx.h"
#include "Unzip.h"
#include "4Story.h"
#include "4StoryDlg.h"
#include <XTrapArg.h>
#include <XTrap4Launcher.h>
#include ".\4storydlg.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define TMP_TIMER						(100)

// CStoryDlg ��ȭ ����
ULONGLONG CStoryDlg::m_lVIDEOMEM = 0;
ULONGLONG CStoryDlg::m_lSYSMEM = 0;


CStoryDlg::CStoryDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CStoryDlg::IDD, pParent)
	, m_pdlgPlaySetting		(NULL)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

	m_strHompageURLForLuancher = HOMEPAGE_NAME;
	m_strNoticeURLForLuancher = LAUNCHERWEB_NAME;

	m_bHACK = TMP_HACK_NONE;
	m_bRUN = FALSE;
	m_hWND = NULL;
	m_hMP = NULL;

	m_bCancel = FALSE;
	m_blPatchFileUpdate = FALSE;
	m_bDownloading = FALSE;
	m_bIsFtpConnecting = FALSE;
	m_strAppName = APP_NAME;
	m_hDownload = NULL;
	m_bProgressingColor = FALSE;
	m_dwMinBetaVer = 0;
	m_bFirstPrePatch = FALSE;
}
CStoryDlg::~CStoryDlg() 
{
	if( sFlag )
	{
		m_bSkin.Detach();
		//::DeleteObject(hBmp);
	}

	// ��ư ������
	if(m_bFlash)
	{
		m_bFlash = FALSE;
		KillTimer(1);
	}
}
void CStoryDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_PROGRESS_CURRENT, m_progressCurrent);
	DDX_Control(pDX, IDC_PROGRESS_TOTAL, m_progressTotal);
	DDX_Control(pDX, IDC_BUTTON_HOMEPAGE, m_bHome);
	DDX_Control(pDX, IDC_BUTTON_SETTING, m_bSetting);
	DDX_Control(pDX, IDC_CHK_PREPATCH, m_chkPrePatch);
	DDX_Control(pDX, IDOK, m_bOK);
	DDX_Control(pDX, IDCANCEL, m_bCANCEL);
}

BEGIN_MESSAGE_MAP(CStoryDlg, CDialog)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_BUTTON_HOMEPAGE, OnBnClickedButtonHomepage)
	ON_BN_CLICKED(IDC_BUTTON_SETTING, OnBnClickedButtonSetting)
	ON_BN_CLICKED(IDOK, OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, OnBnClickedCancel)
	ON_WM_ERASEBKGND()
	ON_WM_LBUTTONDOWN()
	ON_WM_TIMER()
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_CHK_PREPATCH, OnBnClickedChkPrepatch)
END_MESSAGE_MAP()


// CStoryDlg �޽��� ó����

BOOL CStoryDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// �� ��ȭ ������ �������� �����մϴ�. ���� ���α׷��� �� â�� ��ȭ ���ڰ� �ƴ� ��쿡��
	// �����ӿ�ũ�� �� �۾��� �ڵ����� �����մϴ�.
	SetIcon(m_hIcon, TRUE);			// ū �������� �����մϴ�.
	SetIcon(m_hIcon, FALSE);		// ���� �������� �����մϴ�.

	// TODO: ���⿡ �߰� �ʱ�ȭ �۾��� �߰��մϴ�.
	m_bCancel = FALSE;
	m_blPatchFileUpdate = FALSE;
	m_bDownloading = FALSE;
	m_bIsFtpConnecting = FALSE;
	m_strAppName = APP_NAME;
	m_hDownload = NULL;
	//m_bProgressing = FALSE;

	if ( !m_bDisclaimer )
	{
		if( AfxMessageBox( m_strDisclaimer, MB_OKCANCEL ) == IDCANCEL )
		{
			SetDisclaimer(FALSE);
			AfxPostQuitMessage(0);
			return TRUE;
		}

		SetDisclaimer(TRUE);
	}

	HMODULE hModule = ::GetModuleHandle(NULL);
	if(hModule == 0)
	{			
		OnCancel();
		return FALSE;
	}
	::GetModuleFileName(hModule, m_szPatchFileName, _MAX_PATH);
	
	char GAME_TITLE[256]={0,};	
	CString strNew = m_szPatchFileName;	
	int n = strNew.Find(APP_EXT);
	if(n > 0)
	{
		strNew.Delete(n,lstrlen(APP_EXT));
		n = strNew.ReverseFind('\\');
		if(n >= 0)
			strNew.Delete(0, n+1);
	}
	strcpy(GAME_TITLE,strNew);
	
	HANDLE hMutex = NULL;
	if( !OpenMutex(MUTEX_ALL_ACCESS,false,GAME_TITLE) )
		hMutex = CreateMutex(NULL,TRUE,GAME_TITLE);
	else
	{	
		memset(GAME_TITLE,0,sizeof(GAME_TITLE));
		AfxMessageBox(_T("4Story Launcher Already Running !!"));
		OnCancel();
		return FALSE;
	}

	WSADATA wsaDATA;
	WORD wVersionRequested = MAKEWORD( 2, 2);
	int nERROR = WSAStartup( wVersionRequested, &wsaDATA);

	//ReadURLFile();
	//ReadTextFile();
	InfoFileRead();

	//XTrap_L_Patch((char*)xArg, NULL, 60000);
	InitWeb();

	if(!ReadRegistry())
	{
		if( AfxMessageBox(_T("Can not read Registry"),MB_OK ) == IDOK)
		{
			OnCancel();
			return FALSE;
		}
	}

	m_session.SetOwner(this);
	SessionStart(m_strIP, m_wPort);

	m_progressCurrent.SetRange( 0, 100 );
	m_progressTotal.SetRange( 0, 100 );

	CButton * pOK = (CButton *)GetDlgItem(IDOK);
	if(pOK)
		pOK->EnableWindow(FALSE);

	// [ȯ�漳��] ��ư ��Ȱ��ȭ JINUK
	CButton * pSet = (CButton *)GetDlgItem(IDC_BUTTON_SETTING);
	if(pSet)
		pSet->EnableWindow(FALSE);

	m_bFlash = FALSE; // ��ư ������

	////////////////////////////////////////////////////////////////
	m_bOK.LoadBitmap(IDB_BITMAP_SN, IDB_BITMAP_SP, IDB_BITMAP_SH, IDB_BITMAP_SD);
	m_bCANCEL.LoadBitmap(IDB_BITMAP_EN, IDB_BITMAP_EP, IDB_BITMAP_EH, 0);
	m_bHome.LoadBitmap(IDB_BITMAP_HN, IDB_BITMAP_HP, IDB_BITMAP_HH, 0);
	m_bSetting.LoadBitmap(IDB_BITMAP_ON, IDB_BITMAP_OP, IDB_BITMAP_OH, IDB_BITMAP_OD);
	m_chkPrePatch.SetSkin(IDB_BIT_CHECK_NORMAL,IDB_BIT_CHECK_DOWN,0,0,0,0,1,0,0);

	CString str = TOOLTIP_START;
	m_bOK.SetToolTipText(&str);

	str = TOOLTIP_CLOSE;
	m_bCANCEL.SetToolTipText(&str);

	str = TOOLTIP_HOME;
	m_bHome.SetToolTipText(&str);

	str = TOOLTIP_OPTION;
	m_bSetting.SetToolTipText(&str);

	LoadSkin();
	SetControlPos(); // ��Ʈ�� ��ġ ����

	// progress bar color change
	::SendMessage(m_progressCurrent, PBM_SETBARCOLOR, 0, (LPARAM)(COLORREF)RGB(0,169,157));
	::SendMessage(m_progressTotal, PBM_SETBARCOLOR, 0, (LPARAM)(COLORREF)RGB(0,169,157));

	m_progressCurrent.SetBkColor(TRANSPARENT);
	m_progressTotal.SetBkColor(TRANSPARENT);

	SetWindowText(m_strAppName);

	m_chkPrePatch.SetCheck(CheakStartRegistry());
	//SetHWND(GetSafeHwnd());

	// ȯ�漳�� ���̾�α�
	//m_pdlgPlaySetting = new CPlaySetting();
	//m_pdlgPlaySetting->Create(IDD_PLAYSET);
	//m_pdlgPlaySetting->ShowWindow(SW_HIDE);

	return TRUE;  // ��Ʈ�ѿ� ���� ��Ŀ���� �������� ���� ��� TRUE�� ��ȯ�մϴ�.
}

// ��ȭ ���ڿ� �ּ�ȭ ���߸� �߰��� ��� �������� �׸����� 
// �Ʒ� �ڵ尡 �ʿ��մϴ�. ����/�� ���� ����ϴ� MFC ���� ���α׷��� ��쿡��
// �����ӿ�ũ���� �� �۾��� �ڵ����� �����մϴ�.

void CStoryDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // �׸��⸦ ���� ����̽� ���ؽ�Ʈ

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Ŭ���̾�Ʈ �簢������ �������� ����� ����ϴ�.
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// �������� �׸��ϴ�.
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// ����ڰ� �ּ�ȭ�� â�� ���� ���ȿ� Ŀ���� ǥ�õǵ��� �ý��ۿ���
//  �� �Լ��� ȣ���մϴ�. 
HCURSOR CStoryDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

BOOL CStoryDlg::DestroyWindow()
{
	DownloadEnd();
	return CDialog::DestroyWindow();
}

LRESULT CStoryDlg::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	// TODO: ���⿡ Ư��ȭ�� �ڵ带 �߰� ��/�Ǵ� �⺻ Ŭ������ ȣ���մϴ�.
	switch(message)
	{
	case WM_RESTART_PATCH:
		{
			OnCancel();

			m_strDownload.ReleaseBuffer();
			CString strNew=m_szPatchFileName;

			STARTUPINFO StartupInfo;
			PROCESS_INFORMATION ProcessInfo;

			ZeroMemory(&StartupInfo, sizeof(STARTUPINFO));
			ZeroMemory(&ProcessInfo, sizeof(PROCESS_INFORMATION)); 

			StartupInfo.cb = sizeof(STARTUPINFO);
			StartupInfo.dwFlags = STARTF_USESHOWWINDOW;
			StartupInfo.wShowWindow = SW_SHOWNORMAL;

			int n = strNew.ReverseFind('.');
			strNew.Insert(n,BACKUP_EXT);

			if(!CreateProcess(strNew, NULL, 
				NULL, NULL, FALSE, 0, NULL, NULL, &StartupInfo, &ProcessInfo))
				AfxMessageBox(IDS_DOWNLOAD_FAIL);
		}
		break;
	case WM_STOP_DOWNLOAD:
		DestroyWindow();
		break;

	case WM_CLOSE_SESSION:
		DownloadEnd();
		break;
	}

	return CDialog::WindowProc(message, wParam, lParam);
}

void CStoryDlg::InitWeb()
{
	CRect rect;
	GetClientRect(&rect);

	CRect WebRect( 0, 0, rect.Width()/2, rect.Height()/2 );

	CString strUrl = m_strNoticeURLForLuancher;
	if(strUrl == _T(""))
		strUrl = LAUNCHERWEB_NAME;

	m_webCtrl.Create(_T("WebControl"), WS_CHILD|WS_VISIBLE, WebRect, (CWnd*)this, 1001 );
	m_webCtrl.Navigate( strUrl, NULL, NULL, NULL, NULL );
}

BYTE CStoryDlg::ReadRegistry()
{
	CString strDisclaimer;
	BYTE bCurrent = FALSE;

	HKEY hKeyRet;	
	HKEY hKey = HKEY_LOCAL_MACHINE;
	
	CString strSubkey;
	strSubkey = m_strSubkey;
	if(strSubkey == _T(""))
        strSubkey = m_strAppName + REG_COUNTRY;

	strSubkey += _T("\\PB");	

	m_strRegSubKey.Format(_T("%s%s"), REG_SUBKEY, strSubkey);

	int err = RegOpenKey(hKey, m_strRegSubKey, &hKeyRet);
	if(ERROR_SUCCESS != err)
	{
		RegCloseKey(hKeyRet);
		hKey = HKEY_CURRENT_USER;

		bCurrent = TRUE;
	}

	err = RegCreateKey(hKey, m_strRegSubKey, &hKeyRet);
	if(ERROR_SUCCESS != err)
		return FALSE;

	BYTE	data[1024];
	DWORD   type;
	DWORD   cbdata =1024;

	//version
	memset(data, 0, 1024);
	err = RegQueryValueEx(hKeyRet, REG_VALUE_VERSION, NULL, &type, data, &cbdata);
	if( ERROR_SUCCESS != err || type != REG_DWORD)
		return FALSE;

	m_dwVersion = *((LPDWORD)data);
	m_dwNextVersion = m_dwVersion;

	//local path 
	cbdata = 1024;
	memset(data, 0, 1024);
	err = RegQueryValueEx(hKeyRet, REG_VALUE_LOCAL, NULL, &type, data, &cbdata);
	if( ERROR_SUCCESS != err || type != REG_SZ)
		return FALSE;
	
	m_strLocal = m_strDownload = data;
	m_strDownload += _T("\\_download");

	//exe file
	cbdata = 1024;
	memset(data, 0, 1024);
	err = RegQueryValueEx(hKeyRet, REG_VALUE_EXE, NULL, &type, data, &cbdata);
	if( ERROR_SUCCESS != err || type != REG_SZ)
		return FALSE;

	m_strGame = data;

	//patch svr address
	cbdata = 1024;
	memset(data, 0, 1024);
	err = RegQueryValueEx(hKeyRet, REG_VALUE_PATCHSVR, NULL, &type, data, &cbdata);
	if( ERROR_SUCCESS != err || type != REG_SZ)
		return FALSE;

	m_strIP = data;

	//patch svr port
	cbdata = 1024;
	memset(data, 0, 1024);
	err = RegQueryValueEx(hKeyRet, REG_VALUE_PATCHPORT, NULL, &type, data, &cbdata);
	if( ERROR_SUCCESS != err || type != REG_DWORD)
		return FALSE;

	m_wPort = *((LPWORD)data);

	cbdata = 1024;
	memset(data, 0, 1024);
	err = RegQueryValueEx(hKeyRet, REG_VALUE_DISCLAIMER, NULL, &type, data, &cbdata);
	if( ERROR_SUCCESS != err || type != REG_SZ)
		m_bDisclaimer = FALSE;
	else
	{
		strDisclaimer = data;
		strDisclaimer.MakeUpper();

		if ( strDisclaimer == CString(_T("TRUE")) )
			m_bDisclaimer = TRUE;
		else
			m_bDisclaimer = FALSE;
	}

	//	PrePatch ó�� ��ġ üũ
	if(RegQueryValueEx(hKeyRet, _T("PrePatchFirst"), NULL, NULL, NULL, NULL) != ERROR_SUCCESS)
	{
		m_bFirstPrePatch = TRUE;

		//version
		err = RegSetValueEx(hKeyRet, _T("PrePatchFirst"), 0, REG_NONE, NULL, NULL);
		if( ERROR_SUCCESS != err)
			return FALSE;
	}
		
	RegCloseKey(hKeyRet);

	if(bCurrent)
	{
		if(!CopyRegistry())
			return FALSE;
	}

	/////////////////////////////////////////////////////////////////////////////////////
	// �׷��� ���� JINUK
/*
	char strBuf[1024];	
	hKey = HKEY_CURRENT_USER;
	strSubkey = _T("");
	strSubkey = m_strAppName;
	strSubkey += _T("\\Settings");

	m_strRegSubKey.Format(_T("%s%s"), REG_SUBKEY, strSubkey);

	err = RegOpenKey(hKey, m_strRegSubKey, &hKeyRet);
	if(ERROR_SUCCESS != err)
	{
		RegCloseKey(hKeyRet);
		hKey = HKEY_LOCAL_MACHINE;

		bCurrent = TRUE;
	}


	// window mode
	cbdata = 1024;
	memset(data, 0, 1024);
	err = RegQueryValueEx(hKeyRet, REG_WINDOW, NULL, &type, (LPBYTE)strBuf, &cbdata);
	if( ERROR_SUCCESS != err )
		return TRUE;

	if( strcmp(strBuf,"TRUE") == 0 )
		m_dwWindowMode = 1;
	else
		m_dwWindowMode = 0;	
	
	// shader mode
	cbdata = 1024;
	memset(data, 0, 1024);
	err = RegQueryValueEx(hKeyRet, REG_SHADER, NULL, &type, (LPBYTE)strBuf, &cbdata);
	if( ERROR_SUCCESS != err)
		return TRUE;

	if( strcmp(strBuf,"TRUE") == 0)
		m_dwShaderMode = 1;
	else
		m_dwShaderMode = 0;	

	// character mode
	cbdata = 1024;
	memset(data, 0, 1024);
	err = RegQueryValueEx(hKeyRet, REG_CHARACTER, NULL, &type, data, &cbdata);
	if( ERROR_SUCCESS != err || type != REG_DWORD)
		return TRUE;

	m_dwCharMode = *((LPWORD)data);

	// paper matrix mode
	cbdata = 1024;
	memset(data, 0, 1024);
	err = RegQueryValueEx(hKeyRet, REG_MAPDETAIL, NULL, &type, data, &cbdata);
	if( ERROR_SUCCESS != err || type != REG_DWORD)
		return TRUE;

	m_dwPaperMode = *((LPWORD)data);

	// background mode
	cbdata = 1024;
	memset(data, 0, 1024);
	err = RegQueryValueEx(hKeyRet, REG_TEXDETAIL, NULL, &type, data, &cbdata);
	if( ERROR_SUCCESS != err || type != REG_DWORD)
		return TRUE;

	m_dwBackMode = *((LPWORD)data);	

	RegCloseKey(hKeyRet);
*/

	/////////////////////////////////////////////////////////////////////////////////////



	return TRUE;
}

void CStoryDlg::ReadURLFile()
{
	CStdioFile file;
	if( file.Open( "url.txt", CFile::modeRead | CFile::typeText ) )
	{
		file.ReadString( m_strHompageURLForLuancher );

		CString strLinkForGame;
		file.ReadString( strLinkForGame );

		CString strBtnNameForGame;
		file.ReadString( strBtnNameForGame );
		
		file.ReadString( m_strNoticeURLForLuancher );

		file.Close();
	}
}

void CStoryDlg::ReadTextFile()
{
	CString str;
	m_strSubkey = _T("");

	CStdioFile file;

	if( file.Open( "4story.txt", CFile::modeRead | CFile::typeText ) )
	{
		file.ReadString( m_strSubkey );

		file.Close();
	}
}

BYTE CStoryDlg::CopyRegistry()
{
	HKEY hKeyRet;
	HKEY hKey = HKEY_LOCAL_MACHINE;

	CString strSubkey;
	strSubkey = m_strSubkey;
	if(strSubkey == _T(""))
		strSubkey = m_strAppName + REG_COUNTRY;
	strSubkey += _T("\\PB");

	m_strRegSubKey.Format(_T("%s%s"), REG_SUBKEY, strSubkey);

	int err = RegCreateKey(hKey, m_strRegSubKey, &hKeyRet);
	if(ERROR_SUCCESS != err)
		return FALSE;

	BYTE	data[1024];
	DWORD   type = REG_DWORD;
	DWORD   cbData = 4;
	
	memcpy(data, &m_dwVersion, sizeof(DWORD));
	err = RegSetValueEx(hKeyRet, REG_VALUE_VERSION, 0, REG_DWORD, data, cbData);
	if(ERROR_SUCCESS != err)
		return FALSE;

	LPCSTR strLocal = m_strLocal;
	err = RegSetValueEx(
		hKeyRet,
		REG_VALUE_LOCAL, 
		0, 
		REG_SZ, 
		(LPBYTE)strLocal, 
		(DWORD) (lstrlen(strLocal)+1)*sizeof(TCHAR));
	if(ERROR_SUCCESS != err)
		return FALSE;

	LPCSTR strGame = m_strGame;
	err = RegSetValueEx(
		hKeyRet,
		REG_VALUE_EXE, 
		0, 
		REG_SZ, 
		(LPBYTE)strGame, 
		(DWORD) (lstrlen(strGame)+1)*sizeof(TCHAR));
	if(ERROR_SUCCESS != err)
		return FALSE;

	LPCSTR strIP = m_strIP;
	err = RegSetValueEx(
		hKeyRet,
		REG_VALUE_PATCHSVR, 
		0, 
		REG_SZ, 
		(LPBYTE)strIP, 
		(DWORD) (lstrlen(strIP)+1)*sizeof(TCHAR));
	if(ERROR_SUCCESS != err)
		return FALSE;

	DWORD dwPort = m_wPort;
	memcpy(data, &dwPort, sizeof(DWORD));
	err = RegSetValueEx(hKeyRet, REG_VALUE_PATCHPORT, 0, REG_DWORD, data, cbData);
	if(ERROR_SUCCESS != err)
		return FALSE;


	RegCloseKey(hKeyRet);
	return TRUE;
}
void CStoryDlg::LoadSkin()
{
	sFlag = m_bSkin.LoadBitmap(IDB_BITMAP_BASE);
	
	if( sFlag )
	{
		BITMAP bmp;
		m_bSkin.GetBitmap(&bmp);

		SetWindowPos( NULL, 0, 0, bmp.bmWidth, bmp.bmHeight, SWP_NOZORDER );
	}
}
void CStoryDlg::SetControlPos()
{
	CRect rect;
	GetClientRect(&rect);

	m_webCtrl.MoveWindow(rect.Width()/3-30, 92, rect.Width()/2+137, rect.Height()/2+46);

	m_progressCurrent.MoveWindow( rect.Width()/2-66, 431, rect.Width()/2+42, 8 );
	m_progressTotal.MoveWindow( rect.Width()/2-66, 447, rect.Width()/2+42, 8 );

	CRect bRect;
	GetDlgItem(IDOK)->GetWindowRect(&bRect);
	
	m_bHome.MoveWindow( 229, 476, bRect.Width(), bRect.Height() );
	m_bSetting.MoveWindow( 229+bRect.Width()+18, 476, bRect.Width(), bRect.Height() );
	m_bOK.MoveWindow( 229+bRect.Width()*2+36, 476, bRect.Width(), bRect.Height() );
	m_bCANCEL.MoveWindow( 229+bRect.Width()*3+54, 476, bRect.Width(), bRect.Height() );	
	m_chkPrePatch.MoveWindow(749, 512, 16, 14);
}
CString CStoryDlg::LoadString(int nID)
{
	CString strMsg;
	strMsg.LoadString(nID);
	return strMsg;
}
void CStoryDlg::OnConnect(int nErrorCode)
{
	if(nErrorCode)
	{
		AfxMessageBox(LoadString(IDS_CONNECT_FAIL));
		return;
	}
	else
		SendCT_NEWPATCH_REQ();
}
void CStoryDlg::OnClose(int nErrorCode)
{
	if(nErrorCode)
	{
		DownloadEnd();
		CString str;
		str.Format(_T("Error Code %d"), nErrorCode);
		AfxMessageBox(str);
	}
}
DWORD CStoryDlg::OnReceive(CPacket * pPacket)
{
	switch(pPacket->GetID())
	{
		HP_RECEIVE(CT_NEWPATCH_ACK)
	default:
		return 1;
	}
}
void CStoryDlg::SessionStart(CString strIp, DWORD dwPort)
{
	if(!m_session.Start(strIp, dwPort))
		AfxMessageBox(LoadString(IDS_CONNECT_FAIL));
}
void CStoryDlg::Say(CPacket * pPacket)
{
	m_session.Say(pPacket);
}

DWORD WINAPI CStoryDlg::_Download(LPVOID lpParam)
{
	return ((CStoryDlg*)lpParam)->Download();
}

DWORD CStoryDlg::Download()
{
	m_bDownloading = TRUE;

	////////////////////////////////////////////////
	// ���·� �� �߱������� ������ ���� ����
	while( m_webCtrl.GetBusy() )
	{
		Sleep(500);
	}
	////////////////////////////////////////////////
	//size_t nTotal = m_vPatchFile.size();
	//int nCurrent=0;

	CFile		fileNew;
	CStdioFile* remotefile = NULL;

	__int64 nTotalRead = 0;

	while(m_bDownloading)
	{
		LPPATCHFILE pPatchFile = NULL;

		if(m_vPatchFile.size())
		{
			pPatchFile = (LPPATCHFILE)m_vPatchFile.front();
			m_vPatchFile.erase(m_vPatchFile.begin());
		}

		if(!pPatchFile)
			break;

		//	Pre ��ġ �˻�
		BYTE bPatchType = PrePatchCheak(pPatchFile);
		if(bPatchType >= DOWN_ERR)
		{
			m_bDownloading = FALSE;
			break;
		}

		//	���丮 ����
		if(!SetPath(pPatchFile, bPatchType))
		{
			m_bDownloading = FALSE;
			break;
		}

        //	���� �ٿ�ε�
		if(bPatchType != DOWN_PREPATCH)
		{
			DWORD		dwRead = 0;

			TRY
			{
				//	URL ����, ���Ͽ���
				CString strURL = m_strFtpSvr + _T("/");
				if(!pPatchFile->m_strPath.IsEmpty())
					strURL += pPatchFile->m_strPath + _T("/");
				strURL += pPatchFile->m_strName;

				CInternetSession mysession;
				remotefile = mysession.OpenURL(strURL,1,INTERNET_FLAG_TRANSFER_BINARY|INTERNET_FLAG_RELOAD);
				if(!remotefile)
				{
					m_bDownloading = FALSE;
					break;
				}
				//	write ���� ����
				if(fileNew.Open(pPatchFile->m_strName, CFile::modeCreate | CFile::modeNoTruncate | CFile::modeWrite))
				{
					dwRead = (DWORD)fileNew.SeekToEnd();					
					remotefile->Seek(dwRead, CFile::begin);
					if(dwRead)
					{
						nTotalRead += dwRead;
						Progress(pPatchFile->m_strName, pPatchFile->m_dwSize, dwRead, nTotalRead,PROGRESS_TYPE_DOWNLOAD);
					}
				}
				else
				{
					int error = GetLastError();
					m_bDownloading = FALSE;
					break;
				}

				//	���� �ٿ�
				char	lpBuffer[1024+1];
				DWORD	dwNumberOfBytesRead;
				while (dwNumberOfBytesRead = remotefile->Read(lpBuffer, 1024))
				{
					if(!m_bDownloading)
						break;

					fileNew.Write(lpBuffer, dwNumberOfBytesRead);
					dwRead += dwNumberOfBytesRead;
					nTotalRead += dwNumberOfBytesRead;
					Progress(pPatchFile->m_strName, pPatchFile->m_dwSize, dwRead, nTotalRead,PROGRESS_TYPE_DOWNLOAD);
				}

				//	���ϴݱ�
				fileNew.Close();
				remotefile->Close();
				delete remotefile;
				remotefile = NULL;
			}
			CATCH_ALL(error)
			{
				TCHAR	szCause[255];
				CString Cause;
				error->GetErrorMessage(szCause,254,NULL);

				if(remotefile)
				{
					remotefile->Close();
					delete remotefile;
					remotefile = NULL;
				}

				Cause.Format(LoadString(IDS_DOWNLOAD_RETRY), szCause);
				if(AfxMessageBox(Cause, MB_YESNO) == IDYES)
					continue;
				else
					m_bDownloading = FALSE;

				if(fileNew.m_hFile != CFile::hFileNull)
					fileNew.Close();
			}
			END_CATCH_ALL;
		}

		//	��������
		if(m_bDownloading)
		{
			if(bPatchType == DOWN_PREPATCH)
			{
				nTotalRead += pPatchFile->m_dwSize;
				Progress(pPatchFile->m_strName, 1, 1, nTotalRead,PROGRESS_TYPE_DOWNLOAD);
			}

			CString strSource, strTarget;
			strTarget.Empty();

			if(bPatchType)
				strSource = m_strLocal + _T("_BetaPatch\\");
			else
				strSource = m_strDownload + _T("\\");

			if(!pPatchFile->m_strPath.IsEmpty())
			{
				strSource += pPatchFile->m_strPath + _T("\\");
				strTarget = _T("\\")+pPatchFile->m_strPath;
			}
			strSource += pPatchFile->m_strName;

			if( UNZ_OK != Unzip( strSource, strTarget))
			{
				m_bDownloading = FALSE;
				break;
			}

			////	��ġ �Ϸ�
			if(!m_blPatchFileUpdate)
				SetVersion(pPatchFile->m_dwVersion);

			if(pPatchFile)
			{
				delete pPatchFile;
				pPatchFile = NULL;
			}

			if(bPatchType)
				InfoFileWrite();

			remove(strSource);

			if(m_blPatchFileUpdate)
				break;
		}
	}

	if(fileNew.m_hFile != CFile::hFileNull)
		fileNew.Close();

	if(remotefile)
	{
		remotefile->Close();
		delete remotefile;
		remotefile = NULL;
	}

	DownloadFinish();
	m_bDownloading = FALSE;

	return 0;
}

BYTE CStoryDlg::PrePatchCheak(LPPATCHFILE& pPatchFile)
{

	//	PrePatch Ȯ��
	if(pPatchFile->m_dwBetaVer != 0)
	{
		for(VPATCHFILE::iterator it = m_vLocal.begin(); it != m_vLocal.end(); it++)
		{
			if((*it)->m_dwBetaVer == pPatchFile->m_dwBetaVer)
			{
				(*it)->m_dwVersion = pPatchFile->m_dwVersion;

				BYTE bRet = DOWN_PREPATCH;

				//	�̾�޾ƾ� �ϴ°�?					
				if(0 == (*it)->m_dwSize)
				{
					bRet = DOWN_INCOMPRE;
					(*it)->m_dwSize	= pPatchFile->m_dwSize;
				}
				delete pPatchFile;
				pPatchFile = (*it);
				m_vLocal.erase(it);
				return bRet;
			}
		}
	}
	return DOWN_NONE;
}

BYTE CStoryDlg::SetPath(LPPATCHFILE pPatchFile, BYTE bPatchType)
{
	CString strPathName;

	if(bPatchType)
		strPathName = m_strLocal + _T("_BetaPatch");
	else
		strPathName = m_strDownload;

	if(!pPatchFile->m_strPath.IsEmpty())
	{
		strPathName += _T("\\") + pPatchFile->m_strPath;
		if(!CreateDirectoryEx(strPathName))
			return FALSE;
	}
	if(!SetCurrentDirectory(strPathName))
		return FALSE;

	return TRUE;
}

void CStoryDlg::DownloadFinish()
{
	Progress(_T(""), 0, 100, 100, PROGRESS_TYPE_DEFAULT);
	if(m_bDownloading)
		CheckPatch();
	else
	{
		if(m_bCancel)
			PostMessage(WM_STOP_DOWNLOAD);
		else
			AfxMessageBox(LoadString(IDS_FTP_FAIL));
	}

	PostMessage(WM_CLOSE_SESSION);
}

void CStoryDlg::DownloadEnd()
{
	m_bDownloading = FALSE;
	
	//if(m_bProgressing)
	//	return;

	if( m_hDownload )
	{
		WaitForSingleObject(m_hDownload,10000);
		CloseHandle(m_hDownload);
		m_hDownload = NULL;
	}

	for(DWORD i = 0; i < m_vPatchFile.size(); i++)
		delete m_vPatchFile[i];
	m_vPatchFile.clear();
	
	for(DWORD i = 0; i < m_vLocal.size(); i++)
		delete m_vLocal[i];
	m_vLocal.clear();

	if(m_session.m_hSocket != INVALID_SOCKET)
		m_session.End();
}
void CStoryDlg::RemoveDownloadDirectory()
{
	DeleteDirectory(m_strDownload);

	if(SetCurrentDirectory(m_strLocal))
		_rmdir(m_strDownload);

	if(m_vLocal.size() < 1)
	{
		CString strPrePath =  m_strLocal + "_BetaPatch\\";
		DeleteDirectory(strPrePath);

		if(SetCurrentDirectory(m_strLocal))
			_rmdir(strPrePath);
	}

}

void CStoryDlg::RemoveOldPrePatch()
{
	if(!m_vLocal.size())
		return;
	BYTE bDel = FALSE;

	VPATCHFILE::iterator it = m_vLocal.begin();
	while(it < m_vLocal.end())
	{
		if((*it)->m_dwBetaVer < m_dwMinBetaVer)
		{
			remove(m_strLocal + _T("_BetaPatch\\") + (*it)->m_strPath + (*it)->m_strName);
			m_vLocal.erase(it);
			it = m_vLocal.begin();
			bDel = TRUE;
		}
		else
			it++;
	}
	if(bDel)
		InfoFileWrite();
}

void CStoryDlg::DeleteDirectory(LPCTSTR strPath)
{
	WIN32_FIND_DATA FileData;
	if(SetCurrentDirectory(strPath))
	{
		HANDLE hSearch = FindFirstFile(_T("*"), &FileData);
		if( hSearch == INVALID_HANDLE_VALUE) return;

		BYTE bContinue = TRUE;
		while(bContinue)
		{
			if( FileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				if( 0!=strcmp(FileData.cFileName, ".") && 
					0!=strcmp(FileData.cFileName, "..") )
				{
					CString strFullPath = strPath;

					strFullPath+= _T("\\");
					strFullPath += FileData.cFileName;

					DeleteDirectory(strFullPath);
					if(SetCurrentDirectory(strPath))
						_rmdir(strFullPath);
				}
			}
			else
				DeleteFile(FileData.cFileName);

			if(!FindNextFile(hSearch, &FileData)) bContinue = FALSE;
		}

		FindClose(hSearch);
	}
}

void CStoryDlg::CheckPatch()
{
	if(m_blPatchFileUpdate)
		PostMessage(WM_RESTART_PATCH);
	else
	{
		m_strDownload.ReleaseBuffer();
		CString strNew=m_szPatchFileName;
        	
		// ���� ���� ��ư Ȱ��ȭ
		CButton * pOK = (CButton *)GetDlgItem(IDOK);
		if(pOK)
		{
			m_bFlash = TRUE; // ��ư ������
			pOK->EnableWindow(TRUE);
			SetTimer(1, 500, NULL);
		}

		// ȯ�漳�� ��ư Ȱ��ȭ
		CButton * pSet = (CButton *)GetDlgItem(IDC_BUTTON_SETTING);
		if(pSet)
		{
			pSet->EnableWindow(TRUE);
		}

		int n = strNew.Find(BACKUP_EXT);
		if(n > 0)
		{
			strNew.Delete(n,lstrlen(BACKUP_EXT));
			n = strNew.ReverseFind('\\');
			if(n >= 0)
				strNew.Delete(0, n+1);

			CopyFile(m_szPatchFileName, strNew, FALSE);
		}
		//	������� �ʴ� �������� PrePatch �����
		if(m_dwMinBetaVer)
			RemoveOldPrePatch();

		RemoveDownloadDirectory();
	}	
}
int CStoryDlg::Unzip(LPCTSTR strZip, LPCTSTR strDirectory)
{
	int err = UNZ_OK;
	unz_global_info zipinfo;

	unzFile zip = unzOpen( strZip );
	if( NULL == zip )
		return -1;

	err = unzGetGlobalInfo( zip, &zipinfo );
	if( UNZ_OK != err)
	{
		unzClose( zip );
		return err;
	}

	DWORD totalcnt = zipinfo.number_entry;

	err =unzGoToFirstFile( zip );
	if( UNZ_OK != err)
	{
		unzClose( zip );
		return err;
	}

	for( DWORD i = 0; i < totalcnt && err == UNZ_OK; i++)
	{
		if( UNZ_OK == err || UNZ_END_OF_LIST_OF_FILE == err )
		{
			TCHAR szFileName[256];
			unz_file_info file_info;

			err = unzGetCurrentFileInfo( zip, &file_info, szFileName, 256, NULL, 0, NULL, 0);
			if( UNZ_OK != err )
			{
				unzClose( zip );
				return err;
			}

			CString strParse = szFileName;
			int nCurPos = 0;
			strParse.Tokenize(_T("."), nCurPos);
			if(nCurPos >= strParse.GetLength())
			{
				err = unzGoToNextFile( zip );
				continue;
			}
						
			VerifyDirectory(m_strLocal, strDirectory, FALSE);
			
			CString strTargetDir = m_strLocal + strDirectory;
			if( !SetCurrentDirectory( strTargetDir ))
			{
				unzClose( zip );
				return -1;
			}

			CString strOld, strNew;
			char szPatchFileName[_MAX_PATH], szFileExt[_MAX_PATH];
			_splitpath(m_szPatchFileName, NULL, NULL, szPatchFileName, szFileExt);

			strOld = strTargetDir+szFileName;
			if(!lstrcmpi(strOld, m_szPatchFileName))
			{
				CString strNew = szPatchFileName;
				strNew += BACKUP_EXT;
				strNew += szFileExt;
				lstrcpy(szFileName, strNew);
				m_blPatchFileUpdate = TRUE;
			}

			VerifyDirectory(strTargetDir, szFileName, TRUE);

			CFile file;
			if(!file.Open(szFileName, CFile::modeCreate|CFile::shareExclusive|CFile::modeWrite))
			{
				SetFileAttributes( szFileName, FILE_ATTRIBUTE_NORMAL );
				if( !file.Open(szFileName, CFile::modeCreate|CFile::shareExclusive|CFile::modeWrite) )
				{
					unzClose( zip );
					return -1; // ����
				}
			}

			err = unzOpenCurrentFile( zip );
			if( UNZ_OK != err)
			{
				unzClose( zip);
				file.Close();

				return err;
			}

			DWORD dwSize = file_info.uncompressed_size;
			LPVOID data =  malloc(dwSize);

			if(! data )
			{
				unzClose( zip);
				file.Close();
				return -1;
			}

			err = unzReadCurrentFile( zip, data, dwSize );
			if( err < 0)
			{
				unzCloseCurrentFile( zip );
				unzClose( zip );
				file.Close();
				return err;
			}
			
			file.Write(data, dwSize);
			file.Flush();

			free(data);

			if( UNZ_CRCERROR == unzCloseCurrentFile( zip ))
			{
				unzClose( zip);
				file.Flush();
				file.Close();
				return UNZ_CRCERROR;
			}
			file.Close();

			if(1 < totalcnt)
				Progress(szFileName, totalcnt, i+1, 0, PROGRESS_TYPE_UNZIP);
		}
		err = unzGoToNextFile( zip );
	}

	return unzClose( zip );
}
BYTE CStoryDlg::SetVersion(DWORD dwVer)
{
	HKEY hKeyRet;
	HKEY hKey = HKEY_LOCAL_MACHINE;

	CString strSubkey;
	strSubkey = m_strSubkey;
	if(strSubkey == _T(""))
		strSubkey = m_strAppName + REG_COUNTRY;
	strSubkey += _T("\\PB");

	m_strRegSubKey.Format(_T("%s%s"), REG_SUBKEY, strSubkey);

	int err = RegCreateKey(hKey, m_strRegSubKey, &hKeyRet);
	if(ERROR_SUCCESS != err)
		return FALSE;

	BYTE	data[1024];
	DWORD   type = REG_DWORD;
	DWORD   cbData = 4;
	
	memcpy(data, &dwVer, sizeof(DWORD));

	err = RegSetValueEx(hKeyRet, REG_VALUE_VERSION, 0, REG_DWORD, data, cbData);
	if(ERROR_SUCCESS != err)
		return FALSE;

	RegCloseKey(hKeyRet);
	return TRUE;
}
void CStoryDlg::VerifyDirectory(LPCTSTR target, LPCTSTR path, BYTE bHaveFile)
{
	TCHAR parse[256];
	char * tok = NULL;
	CString strDirectory;

	lstrcpy( parse, path );
	tok = strtok((char *)parse, PATH_DELIMETERS);
	if( !tok )
		return;

	strDirectory = target;

	if( bHaveFile )
	{
		do
		{
			if(lstrcmp(tok, _T(".")))
			{
				CreateDirectory(strDirectory, NULL);
				strDirectory += '\\';
				strDirectory += tok;
			}
		}while(NULL!= (tok = strtok(NULL, PATH_DELIMETERS)));
	}
	else
	{	
		CreateDirectory(strDirectory, NULL);										
		do
		{
			if(lstrcmp(tok, _T(".")))
			{	
				strDirectory += '\\';
				strDirectory += tok;
				CreateDirectory(strDirectory, NULL);										
			}
		}while(NULL!= (tok = strtok(NULL, PATH_DELIMETERS)));
	}
}
BYTE CStoryDlg::CreateDirectoryEx(CString strPath)
{
	if( strPath.IsEmpty() ) return TRUE;

	if( !CreateDirectory( strPath, NULL ))
	{
		DWORD dwErr = GetLastError();
		if( dwErr == ERROR_ALREADY_EXISTS ) return TRUE;

		if( dwErr == ERROR_PATH_NOT_FOUND )
		{
			int nPos = 0;
			int nNext = strPath.Find(_T("\\"), 0);
			while(nNext >=0 )
			{
				nPos = nNext;
				nNext = strPath.Find(_T("\\"), nPos+1 );
			}

			CString strCreate = strPath.Left( nPos );
			if( CreateDirectoryEx( strCreate) )
				return CreateDirectory(strPath, NULL );
		}
		return FALSE;
	}

	return TRUE;
}
void CStoryDlg::Progress(CString strName, DWORD dwSize, DWORD dwCurrent, __int64 nTotal, BYTE bType)
{
	if(!m_bDownloading)
		return;

	//m_bProgressing = TRUE;
	switch(bType)
	{
	case PROGRESS_TYPE_DEFAULT:
		{
			m_progressCurrent.SetPos( (int)dwCurrent );
			m_progressTotal.SetPos( (int)nTotal );
		}
		break;
	case PROGRESS_TYPE_DOWNLOAD:
		{
			if(m_bProgressingColor)
			{
				::SendMessage(m_progressCurrent, PBM_SETBARCOLOR, 0, (LPARAM)(COLORREF)RGB(0,169,157));
				m_bProgressingColor = FALSE;
			}
			int nPos = int(dwSize ? __int64(100) * dwCurrent / dwSize : 0);
			m_progressCurrent.SetPos( nPos );

			nPos = int(100 * nTotal / m_nPatchTotalSize);
			m_progressTotal.SetPos( nPos );
		}
		break;
	case PROGRESS_TYPE_UNZIP:
		{
			if(!m_bProgressingColor)
			{
				::SendMessage(m_progressCurrent, PBM_SETBARCOLOR, 0, (LPARAM)(COLORREF)RGB(180,40,100));
				m_bProgressingColor = TRUE;
			}
			int nPos = int(dwSize ? __int64(100) * dwCurrent / dwSize : 0);
			m_progressCurrent.SetPos( nPos );
		}
		break;
	default:
		break;
	}	
	//m_bProgressing = FALSE;
}

void CStoryDlg::StartGame()
{
	if(SetCurrentDirectory( m_strLocal ))
	{
		STARTUPINFO StartupInfo;
		PROCESS_INFORMATION ProcessInfo;

		ZeroMemory(&StartupInfo, sizeof(STARTUPINFO));
		ZeroMemory(&ProcessInfo, sizeof(PROCESS_INFORMATION)); 

		StartupInfo.cb = sizeof(STARTUPINFO);
		StartupInfo.dwFlags = STARTF_USESHOWWINDOW;
		StartupInfo.wShowWindow = SW_SHOWNORMAL;
		
		CString strCommandLine;
		strCommandLine.Format(_T("%s %s %d"), m_strGame, m_strGameSvr, m_wGamePort);

		if(!CreateProcess(m_strGame, strCommandLine.LockBuffer(), 
			NULL, NULL, FALSE, 0, NULL, NULL, &StartupInfo, &ProcessInfo))
		{
			strCommandLine.ReleaseBuffer();
			return;
		}

		strCommandLine.ReleaseBuffer();
	}
}

DWORD CStoryDlg::OnCT_NEWPATCH_ACK(CPacket * pPacket)
{
	m_nPatchTotalSize = 0;

	SOCKADDR_IN addr;
	WORD wCount;

	(*pPacket)
		>> m_strFtpSvr
		>> addr.sin_addr.s_addr
		>> m_wGamePort
		>> m_dwMinBetaVer
		>> wCount;

	m_strGameSvr = inet_ntoa(addr.sin_addr);

	for(WORD i=0; i<wCount; i++)
	{
		LPPATCHFILE pFile = new PATCHFILE();

		(*pPacket)
			>> pFile->m_dwVersion
			>> pFile->m_strPath
			>> pFile->m_strName
			>> pFile->m_dwSize
			>> pFile->m_dwBetaVer;

		m_nPatchTotalSize += pFile->m_dwSize;
		m_vPatchFile.push_back(pFile);
	}

	SendCT_PATCHSTART_REQ();

	if(m_bFirstPrePatch)
	{
		if( AfxMessageBox(_T("Will you register 4Story PrePatch Downloader to Starting Program?"), MB_YESNO ) == IDYES )
			WriteStartRegistry(TRUE);
		m_chkPrePatch.SetCheck(CheakStartRegistry());
	}

	if(wCount)
	{
		Progress(_T(""), 0, 0, 0, PROGRESS_TYPE_DEFAULT);

		DWORD dwThreadID;
		CreateDirectory( m_strDownload , NULL);
		m_hDownload = CreateThread(NULL, 0, _Download, (LPVOID)this, CREATE_SUSPENDED, &dwThreadID);
		if( !m_hDownload )
		{
			AfxMessageBox(LoadString(IDS_THREAD_FAIL));
			return 1;
		}
		ResumeThread(m_hDownload);
	}
	else
	{
		m_progressCurrent.SetPos(100);
		m_progressTotal.SetPos(100);
		CheckPatch();
		return 1;
	}
	return 0;
}
void CStoryDlg::SendCT_NEWPATCH_REQ()
{
	CPacket * pMsg = new CPacket();
	pMsg->SetID(CT_NEWPATCH_REQ)
		<< m_dwVersion;
	Say(pMsg);
}

void CStoryDlg::SendCT_PATCHSTART_REQ()
{
	CPacket * pMsg = new CPacket();
	pMsg->SetID(CT_PATCHSTART_REQ);
	Say(pMsg);
}

void CStoryDlg::OnBnClickedButtonHomepage()
{
	CString strUrl = m_strHompageURLForLuancher;
	if(strUrl == _T(""))
		strUrl = HOMEPAGE_NAME;
	    
	ShellExecute(NULL, _T("open"), _T("iexplore"), (LPCTSTR)strUrl, NULL, SW_SHOWNORMAL);
}

void CStoryDlg::OnBnClickedButtonSetting()
{
	// TODO: ���⿡ ��Ʈ�� �˸� ó���� �ڵ带 �߰��մϴ�.
	
	//CGameSetting dlg;
	//dlg.SetGraphicMode(m_dwWindowMode, m_dwShaderMode, m_dwCharMode, m_dwPaperMode, m_dwBackMode);
	//if( dlg.DoModal() == IDOK )
	//{
	//	dlg.GetGraphicMode(&m_dwWindowMode, &m_dwShaderMode, &m_dwCharMode, &m_dwPaperMode, &m_dwBackMode);
	//	WriteRegistry();
	//}

	// ��� �˻�
	InitCAPS();
	if( m_lSYSMEM >= 1000000000 && m_lVIDEOMEM >= 256 )
		theApp.m_bOptionLevel = OPTION_HI; // High
	else if( m_lSYSMEM < 1000000000 && m_lVIDEOMEM < 256 )
		theApp.m_bOptionLevel = OPTION_LOW; // Low
	else
		theApp.m_bOptionLevel = OPTION_MID; // Med

	CollectResolution();

	// ���Ӽ��� ��ȭ���� ��޸����� ����
	if( m_pdlgPlaySetting == NULL)
	{
		m_pdlgPlaySetting = new CPlaySetting();
		m_pdlgPlaySetting->Create(IDD_PLAYSET);
		m_pdlgPlaySetting->ShowWindow(SW_SHOW);
	}
	else
	{
		m_pdlgPlaySetting->ShowWindow(SW_SHOW);
	}

}

BYTE CStoryDlg::WriteRegistry()
{
	HKEY hKeyRet;
	HKEY hKey = HKEY_CURRENT_USER; //HKEY_LOCAL_MACHINE;

	CString strSubkey;
	strSubkey = m_strSubkey;
	if(strSubkey == _T(""))
		strSubkey = m_strAppName + REG_COUNTRY;
	strSubkey += _T("\\Settings"); //_T("\\PB");

	m_strRegSubKey.Format(_T("%s%s"), REG_SUBKEY, strSubkey);

	int err = RegCreateKey(hKey, m_strRegSubKey, &hKeyRet);
	if(ERROR_SUCCESS != err)
		return FALSE;

	char	strBuf[1024];
	BYTE	data[1024];
	DWORD   type = REG_DWORD;
	DWORD   cbData = 4;

	// window mode
	//memcpy(data, &m_dwWindowMode, sizeof(DWORD));
	//err = RegSetValueEx(hKeyRet, REG_VALUE_WINDOW, 0, REG_DWORD, data, cbData);

	if(m_dwWindowMode == 1)
		strcpy(strBuf,"TRUE");
	else
		strcpy(strBuf,"FALSE");

	err = RegSetValueEx(hKeyRet, REG_WINDOW, 0, REG_SZ, (BYTE*)strBuf, (DWORD)strlen(strBuf));

	if(ERROR_SUCCESS != err)
		return FALSE;

	// shader mode
	//memcpy(data, &m_dwShaderMode, sizeof(DWORD));

	if(m_dwShaderMode == 1)
		strcpy(strBuf,"TRUE");
	else
		strcpy(strBuf,"FALSE");

	err = RegSetValueEx(hKeyRet, REG_SHADER, 0, REG_SZ, (BYTE*)strBuf, (DWORD)strlen(strBuf));
	if( ERROR_SUCCESS != err || type != REG_DWORD)
		return FALSE;

	// character mode // ĳ���� ǰ��
	memcpy(data, &m_dwCharMode, sizeof(DWORD));
	err = RegSetValueEx(hKeyRet, REG_CHARACTER, 0, REG_DWORD, data, cbData);
	if( ERROR_SUCCESS != err || type != REG_DWORD)
		return FALSE;

	// paper matrix mode // ���� ǰ��
	memcpy(data, &m_dwPaperMode, sizeof(DWORD));
	err = RegSetValueEx(hKeyRet, REG_MAPDETAIL, 0, REG_DWORD, data, cbData);
	if( ERROR_SUCCESS != err || type != REG_DWORD)
		return FALSE;

	// background mode // �ؽ��� ǰ��
	memcpy(data, &m_dwBackMode, sizeof(DWORD));
	err = RegSetValueEx(hKeyRet, REG_TEXDETAIL, 0, REG_DWORD, data, cbData);
	if( ERROR_SUCCESS != err || type != REG_DWORD)
		return FALSE;

	RegCloseKey(hKeyRet);
	
	return TRUE;
}


void CStoryDlg::OnBnClickedOk()
{
	// TODO: ���⿡ ��Ʈ�� �˸� ó���� �ڵ带 �߰��մϴ�.
    if(m_bFlash)
	{
		m_bFlash = FALSE;
		KillTimer(1);
	}

	StartGame();
	SetCurrentDirectory( m_strLocal );

	//EndThread();
	//m_bHACK = LaunchTClient();


	OnOK();
}

void CStoryDlg::OnBnClickedCancel()
{
	// TODO: ���⿡ ��Ʈ�� �˸� ó���� �ڵ带 �߰��մϴ�.
	if(m_bFlash)
	{
		m_bFlash = FALSE;
		KillTimer(1);
	}

	if(m_bDownloading)
	{
		m_bCancel = TRUE;
		DownloadEnd();
	}
	else
		DestroyWindow();
}

BOOL CStoryDlg::OnEraseBkgnd(CDC* pDC)
{
	if( sFlag )
	{	
		CRect rect;
		GetClientRect(&rect);		

		CDC MemDC;
		MemDC.CreateCompatibleDC(pDC);
		CBitmap* pbmpOld = MemDC.SelectObject( &m_bSkin );

		BITMAP bmp;
		m_bSkin.GetBitmap(&bmp);

		pDC->StretchBlt(0, 0, rect.right, rect.bottom, &MemDC, 0, 0, bmp.bmWidth, bmp.bmHeight, SRCCOPY);
		MemDC.SelectObject( pbmpOld );

		ReleaseDC(pDC);
	}	

	return TRUE;
//	return CDialog::OnEraseBkgnd(pDC);
}

void CStoryDlg::OnLButtonDown(UINT nFlags, CPoint point)
{
	// TODO: ���⿡ �޽��� ó���� �ڵ带 �߰� ��/�Ǵ� �⺻���� ȣ���մϴ�.
	SendMessage(WM_NCLBUTTONDOWN, HTCAPTION, MAKELPARAM(point.x, point.y));
	CDialog::OnLButtonDown(nFlags, point);
}

void CStoryDlg::OnTimer(UINT nIDEvent)
{
	// TODO: ���⿡ �޽��� ó���� �ڵ带 �߰� ��/�Ǵ� �⺻���� ȣ���մϴ�.
	switch(nIDEvent)
	{
	case 1:
		m_bOK.FlashButton();
		m_bFlash++;
		break;
	}

	if(m_bFlash == 10)
		KillTimer(1);

	CDialog::OnTimer(nIDEvent);
}

void CStoryDlg::OnDestroy()
{
	// TODO: ���⿡ �޽��� ó���� �ڵ带 �߰��մϴ�.

	if( m_pdlgPlaySetting )
	{
		delete m_pdlgPlaySetting;
		m_pdlgPlaySetting = NULL;
	}

	//SetHWND(NULL);
	//EndThread();
	//HackMSG();	

	CDialog::OnDestroy();
}

void CStoryDlg::InitCAPS()
{
	IDxDiagProvider *pProvider = NULL;
	DXDIAG_INIT_PARAMS vPARAM;

	if( FAILED(CoCreateInstance(
		CLSID_DxDiagProvider,
		NULL,
		CLSCTX_INPROC_SERVER,
		IID_IDxDiagProvider,
		(LPVOID *) &pProvider)))
		return;

	ZeroMemory( &vPARAM, sizeof(DXDIAG_INIT_PARAMS));
	vPARAM.dwDxDiagHeaderVersion = DXDIAG_DX9_SDK_VERSION;
	vPARAM.bAllowWHQLChecks = TRUE;
	vPARAM.pReserved = NULL;
	vPARAM.dwSize = sizeof(DXDIAG_INIT_PARAMS);

	if(SUCCEEDED(pProvider->Initialize(&vPARAM)))
	{
		IDxDiagContainer *pROOT = NULL;

		if(SUCCEEDED(pProvider->GetRootContainer(&pROOT)))
		{
			IDxDiagContainer *pContainer = NULL;

			if(SUCCEEDED(pROOT->GetChildContainer( L"DxDiag_SystemInfo", &pContainer)))
			{
				VARIANT vVALUE;
				VariantInit(&vVALUE);

				if(SUCCEEDED(pContainer->GetProp( L"ullPhysicalMemory", &vVALUE)))
					m_lSYSMEM = _wtoi64(vVALUE.bstrVal);

				VariantClear(&vVALUE);
				pContainer->Release();
			}

			if(SUCCEEDED(pROOT->GetChildContainer( L"DxDiag_DisplayDevices", &pContainer)))
			{
				DWORD dwCount = 0;

				if( SUCCEEDED(pContainer->GetNumberOfChildContainers(&dwCount)) && dwCount > 0 )
				{
					WCHAR szNAME[MAX_PATH];

					if(SUCCEEDED(pContainer->EnumChildContainerNames( 0, szNAME, MAX_PATH)))
					{
						IDxDiagContainer *pDisplay = NULL;

						if(SUCCEEDED(pContainer->GetChildContainer( szNAME, &pDisplay)))
						{
							VARIANT vVALUE;
							VariantInit(&vVALUE);

							if(SUCCEEDED(pDisplay->GetProp( L"szDisplayMemoryEnglish", &vVALUE)))
								m_lVIDEOMEM = _wtoi64(vVALUE.bstrVal);

							VariantClear(&vVALUE);
							pDisplay->Release();
						}
					}
				}

				pContainer->Release();
			}

			pROOT->Release();
		}
	}

	pProvider->Release();
}

BYTE CStoryDlg::InitDevice()
{
	m_Device.m_pD3D = Direct3DCreate9(D3D_SDK_VERSION);
	
	if(!m_Device.m_pD3D)
	{
		MessageBox(_T("Resolution Init Error"),APP_NAME);		
		return FALSE;
	}

	D3DDISPLAYMODE d3ddm;
	BYTE bWindowMode = FALSE;
	if(bWindowMode)
	{
		HRESULT hr = m_Device.m_pD3D->GetAdapterDisplayMode(
			D3DADAPTER_DEFAULT,
			&d3ddm);

		if(FAILED(hr))
		{
			m_Device.m_pD3D->Release();
			m_Device.m_pD3D = NULL;

			return FALSE;
		}
	}
	else
		d3ddm.Format = D3DFMT_X8R8G8B8;

	ZeroMemory( &m_Device.m_vPRESENT, sizeof(m_Device.m_vPRESENT));
	m_Device.m_vPRESENT.BackBufferFormat = d3ddm.Format;

	return TRUE;
}


void CStoryDlg::CollectResolution()
{
	DEVMODE devMode;
	::EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &devMode);
	INT nCurrentFrequently = devMode.dmDisplayFrequency;

	InitDevice();

	if(!m_Device.m_pD3D)
	{
		CString strFMT;
		strFMT.Format("%d X %d",DEFAULT_SCREEN_X,DEFAULT_SCREEN_Y);
		strFMT.Trim(_T(" "));
		theApp.InsertResolution(DEFAULT_SCREEN_X,DEFAULT_SCREEN_Y,strFMT);
		return;
	}

	UINT uCount = m_Device.m_pD3D->GetAdapterModeCount(
		D3DADAPTER_DEFAULT,
		m_Device.m_vPRESENT.BackBufferFormat);

	for( UINT i=0 ; i < uCount ; ++i )
	{
		D3DDISPLAYMODE Mode;
		
		if( D3D_OK  == m_Device.m_pD3D->EnumAdapterModes(
			D3DADAPTER_DEFAULT,
			m_Device.m_vPRESENT.BackBufferFormat,
			i,
			&Mode) )
		{
			if( Mode.Width >= DEFAULT_SCREEN_X &&
				Mode.Height >= DEFAULT_SCREEN_Y &&
				Mode.Format == m_Device.m_vPRESENT.BackBufferFormat &&
				Mode.RefreshRate == nCurrentFrequently )
			{
				CString strFMT;
				strFMT.Format( "%d X %d ", Mode.Width,Mode.Height );				
				theApp.InsertResolution(Mode.Width,Mode.Height,strFMT);
			}
		}
	}

}

void CStoryDlg::HackMSG()
{
	switch(m_bHACK)
	{
	case TMP_HACK_INVALID_PE	:
		{
			CString strTITLE;
			CString strMSG;

			strMSG.LoadString(IDS_ERROR_LOAD_FILE);
			::MessageBox( NULL, strMSG, APP_NAME, MB_OK);
		}

		break;

	case TMP_HACK_TIMEOUT	:
		{
			CString strTITLE;
			CString strMSG;

			strMSG.LoadString(IDS_ERROR_TIMEOUT);
			::MessageBox( NULL, strMSG, APP_NAME, MB_OK);
		}

		break;

	case TMP_HACK_FOUND		:
		{
			CString strTITLE;
			CString strMSG;

			strMSG.LoadString(IDS_ERROR_HACK_DETECTED);
			::MessageBox( NULL, strMSG, APP_NAME, MB_OK);
		}

		break;
	}
}

BYTE CStoryDlg::BeginProtect()
{
	DWORD dwThreadID;

	InitializeCriticalSection(&m_cs);
	SetRUN(TRUE);

	if( !m_vModuleGuard.InitProtector( MAKEINTRESOURCE(IDR_MPCFILE1), _T("MPCFILE")) ||
		!m_vModuleGuard.BeginWatch() )
	{
		EndProtect();
		m_bRUN = FALSE;

		return FALSE;
	}

	m_hMP = ::CreateThread(
		NULL, 0,
		_MPThread,
		(LPVOID) this,
		0, &dwThreadID);

	if(!m_hMP)
	{
		EndProtect();
		m_bRUN = FALSE;

		return FALSE;
	}

	return TRUE;
}

void CStoryDlg::EndThread()
{
	if(m_hMP)
	{
		SetRUN(FALSE);
		WaitForSingleObject( m_hMP, INFINITE);

		m_hMP = NULL;
	}
}

void CStoryDlg::EndProtect()
{
	EndThread();

	DeleteCriticalSection(&m_cs);
	m_vModuleGuard.ClearModule();
}

DWORD WINAPI CStoryDlg::_MPThread( LPVOID lpParam)
{
	((CStoryDlg *) lpParam)->MPThread();
	return 0;
}

void CStoryDlg::MPThread()
{
	while(GetRUN())
	{
		if(!m_vModuleGuard.CheckValid())
			PostQUIT();

		Sleep(TMP_TIMER);
	}
}

void CStoryDlg::SetRUN( BYTE bRUN)
{
	SMART_LOCKCS(&m_cs);
	m_bRUN = bRUN;
}

BYTE CStoryDlg::GetRUN()
{
	SMART_LOCKCS(&m_cs);
	return m_bRUN ? TRUE : FALSE;
}

void CStoryDlg::SetHWND( HWND hWND)
{
	SMART_LOCKCS(&m_cs);
	m_hWND = hWND;
}

void CStoryDlg::PostQUIT()
{
	SMART_LOCKCS(&m_cs);

	if(!m_bHACK)
		m_bHACK = m_vModuleGuard.HackDetected() ? TMP_HACK_FOUND : TMP_HACK_TIMEOUT;

	if(m_hWND)
		::PostMessage( m_hWND, WM_COMMAND, IDCANCEL, 0);
}

BYTE CStoryDlg::LaunchTClient()
{
	CTModuleProtector vLauncher;

	CString strCommandLine;
	strCommandLine.Format(_T("%s %d"), m_strGameSvr, m_wGamePort);

	if(m_bHACK)
		return m_bHACK;

	if(!m_vModuleGuard.CheckValid())
		return m_vModuleGuard.HackDetected() ? TMP_HACK_FOUND : TMP_HACK_TIMEOUT;

	if(!vLauncher.InitProtector(CString(_T(".\\TClientMP.mpc"))))
		return TMP_HACK_INVALID_PE;

	return vLauncher.ExecPE( &CString(_T(".\\TClient.exe")), &strCommandLine) ? TMP_HACK_NONE : TMP_HACK_INVALID_PE;
}

CString CStoryDlg::GetLocalString()
{
	return m_strLocal;
}

void CStoryDlg::ReadDisclaimerFile()
{
	CStdioFile file;
	CString strBuf;

	if( file.Open( "disclaimer.txt", CFile::modeRead | CFile::typeText ) )
	{
		m_strDisclaimer = "";

		while(file.ReadString(strBuf))
			m_strDisclaimer += strBuf + _T("\r\n");

		file.Close();
	}
}

BYTE CStoryDlg::SetDisclaimer(BYTE bDisclaimer)
{
	HKEY hKeyRet;
	HKEY hKey = HKEY_LOCAL_MACHINE;

	CString strSubkey;
	strSubkey = m_strSubkey;
	if(strSubkey == _T(""))
		strSubkey = m_strAppName + REG_COUNTRY;
	strSubkey += _T("\\PB");

	m_strRegSubKey.Format(_T("%s%s"), REG_SUBKEY, strSubkey);

	int err = RegCreateKey(hKey, m_strRegSubKey, &hKeyRet);
	if(ERROR_SUCCESS != err)
		return FALSE;

	char	strBuf[1024];
	DWORD   type = REG_DWORD;
	DWORD   cbData = 4;


	if(bDisclaimer == TRUE)
		strcpy(strBuf,"TRUE");
	else
		strcpy(strBuf,"FALSE");

	err = RegSetValueEx(hKeyRet, REG_VALUE_DISCLAIMER, 0, REG_SZ, (BYTE*)strBuf, (DWORD)strlen(strBuf));
	if( ERROR_SUCCESS != err )
		return FALSE;

	RegCloseKey(hKeyRet);
	return TRUE;
}

void CStoryDlg::InfoFileWrite()
{
	char strFile[MAX_PATH];

	sprintf( strFile, "%s_BetaPatch\\PrePatch.btp", m_strLocal);

	FILE* f = fopen( strFile ,"wt");
	if( !f ) return;

	VPATCHFILE::iterator it;
	for(it = m_vLocal.begin(); it != m_vLocal.end(); it++)
	{	
		fprintf(f,"%d\t%s\t%d\t%s\n", (*it)->m_dwBetaVer, (*it)->m_strName, (*it)->m_dwSize, (*it)->m_strPath);
	}
	fclose(f);
}

void CStoryDlg::InfoFileRead()
{
	char strFile[MAX_PATH];
	char strLine[MAX_PATH];

	VPATCHFILE::iterator it;
	for(it = m_vLocal.begin(); it != m_vLocal.end(); it++)
		delete (LPPATCHFILE)*it;

	m_vLocal.clear();

	sprintf( strFile, "%s_BetaPatch\\PrePatch.btp", m_strLocal);

	FILE* f = fopen( strFile ,"rt");
	if( !f ) return;

	while(fgets(strLine, MAX_PATH, f))
	{
		strLine[strlen(strLine)-1] = NULL;
		LPPATCHFILE PatchFile = new PATCHFILE;
		int nIndex = 0;

		PatchFile->m_dwBetaVer	= (DWORD)atoi(strtok(strLine,"\t"));
		PatchFile->m_strName	= strtok(NULL,"\t");
		PatchFile->m_dwSize		= (DWORD)atoi(strtok(NULL,"\t"));
		PatchFile->m_strPath	= strtok(NULL,"\t");
		PatchFile->m_dwVersion	= 0;

		CString strPath = m_strLocal + "_BetaPatch\\" + PatchFile->m_strPath +'\\'+ PatchFile->m_strName;
		if(FindPatchFile(strPath))
			m_vLocal.push_back(PatchFile);
	}
	fclose(f);
}

BYTE CStoryDlg::FindPatchFile(CString strPathName)
{ 
	WIN32_FIND_DATA fd;
	HANDLE hFind = FindFirstFile(strPathName, &fd);

	if (hFind != INVALID_HANDLE_VALUE)
	{
		FindClose (hFind);
		return TRUE;
	}
	return FALSE;
}

BYTE CStoryDlg::CheakStartRegistry()
{
	HKEY hKeyRet;	
	
	int err = RegOpenKey(HKEY_LOCAL_MACHINE, _T("Software\\Microsoft\\Windows\\CurrentVersion\\run"), &hKeyRet);
	if(ERROR_SUCCESS != err)
	{
		RegCloseKey(hKeyRet);
		return FALSE;
	}
	err = RegQueryValueEx(hKeyRet, _T("4StoryPrePatch"), NULL, NULL, NULL, NULL);
	if( ERROR_SUCCESS != err)
	{
		RegCloseKey(hKeyRet);
		return FALSE;
	}
	RegCloseKey(hKeyRet);
	return TRUE;
}

BYTE CStoryDlg::WriteStartRegistry(BYTE bCheak)
{
	HKEY hKeyRet;

	int err = RegOpenKey(HKEY_LOCAL_MACHINE,_T("Software\\Microsoft\\Windows\\CurrentVersion\\run"), &hKeyRet);
	if(ERROR_SUCCESS != err)
	{
		RegCloseKey(hKeyRet);
		return FALSE;
	}
	if(bCheak)
	{
		CString strPath = m_strLocal + _T("PrePatch.exe");
		err = RegSetValueEx(hKeyRet, _T("4StoryPrePatch"), 0, REG_SZ, (LPBYTE)strPath.GetBuffer(), (DWORD) (lstrlen(strPath)+1)*sizeof(TCHAR));
		if(ERROR_SUCCESS != err)
		{
			RegCloseKey(hKeyRet);
			return FALSE;
		}
	}
	else
		RegDeleteValue(hKeyRet, _T("4StoryPrePatch"));

	RegCloseKey(hKeyRet);
	return TRUE;
}

void CStoryDlg::OnBnClickedChkPrepatch()
{
	WriteStartRegistry((BYTE)m_chkPrePatch.GetCheck());
}