// IPSecConfigDlg.cpp : implementation file
//

#include "stdafx.h"
#include "ipsecgui.h"
#include "ipsecdlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	//{{AFX_MSG(CAboutDlg)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
		// No message handlers
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CIPSecConfigDlg dialog

CIPSecConfigDlg::CIPSecConfigDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CIPSecConfigDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CIPSecConfigDlg)
	m_DeviceTypeCombo = _T("HOST");    
	m_IPSecProtoCombo = _T("AH");
	m_IPSecModeCombo = _T("TRANSPORT");
	m_LocalPortCombo = _T("");
	m_ProtocolCombo = _T("*");
	m_RemotePortCombo = _T("");
	m_AuthAlgCombo = _T("HMAC-MD5");	
	m_RemoteTunnelIPEdit = _T("");
	m_IPSecActionCombo = _T("APPLY");
	m_IPSecInterfaceEdit = 0;
	m_LocalIPEdit = _T("*");
	m_RemoteIPEdit = _T("*");
	m_RemoteSAAddrEdit = _T("");
	m_LocalSAAddrEdit = _T("");
	m_RemoteSPIEdit = 0;
	m_LocalSPIEdit = 0;
	m_LocalKeyFileEdit = _T("");
	m_RemoteKeyFileEdit = _T("");
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CIPSecConfigDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CIPSecConfigDlg)
	DDX_CBString(pDX, IDC_DEVICETYPE_COMBO, m_DeviceTypeCombo);
	DDV_MaxChars(pDX, m_DeviceTypeCombo, 20);
	DDX_CBString(pDX, IDC_IPSECPROTO_COMBO, m_IPSecProtoCombo);
	DDX_CBString(pDX, IDC_IPSECMODE_COMBO, m_IPSecModeCombo);
	DDX_CBString(pDX, IDC_LCLPORT_COMBO, m_LocalPortCombo);
	DDX_CBString(pDX, IDC_PROTOCOL_COMBO, m_ProtocolCombo);
	DDX_CBString(pDX, IDC_RMTPORT_COMBO, m_RemotePortCombo);
	DDX_CBString(pDX, IDC_AUTHALG_COMBO, m_AuthAlgCombo);
	DDX_Text(pDX, IDC_RMTTUNNELIP_EDIT, m_RemoteTunnelIPEdit);
	DDX_CBString(pDX, IDC_IPSECACTION_COMBO, m_IPSecActionCombo);
	DDX_Text(pDX, IDC_IPSECINTERFACE_EDIT, m_IPSecInterfaceEdit);
	DDX_Text(pDX, IDC_LOCALIP_EDIT, m_LocalIPEdit);
	DDX_Text(pDX, IDC_REMOTEIP_EDIT, m_RemoteIPEdit);
	DDX_Text(pDX, IDC_RMTSAADDR_EDIT, m_RemoteSAAddrEdit);
	DDX_Text(pDX, IDC_LCLSAADDR_EDIT, m_LocalSAAddrEdit);
	DDX_Text(pDX, IDC_REMOTESPI_EDIT, m_RemoteSPIEdit);
	DDX_Text(pDX, IDC_LOCALSPI_EDIT, m_LocalSPIEdit);
	DDX_Text(pDX, IDC_LOCALKEYFILE_EDIT, m_LocalKeyFileEdit);
	DDV_MaxChars(pDX, m_LocalKeyFileEdit, 15);
	DDX_Text(pDX, IDC_REMOTEKEYFILE_EDIT, m_RemoteKeyFileEdit);
	DDV_MaxChars(pDX, m_RemoteKeyFileEdit, 15);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CIPSecConfigDlg, CDialog)
	//{{AFX_MSG_MAP(CIPSecConfigDlg)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_COMMAND(ID_HELP_ABOUT, OnHelpAbout)
	ON_BN_CLICKED(IDC_CONFIGURE_BUTTON, OnConfigureButton)
	ON_CBN_SELCHANGE(IDC_DEVICETYPE_COMBO, OnSelchangeDevicetypeCombo)
	ON_CBN_SELCHANGE(IDC_IPSECMODE_COMBO, OnSelchangeIpsecmodeCombo)
	ON_CBN_SELCHANGE(IDC_IPSECACTION_COMBO, OnSelchangeIpsecactionCombo)
	ON_EN_UPDATE(IDC_REMOTEIP_EDIT, OnUpdateRemoteipEdit)
	ON_EN_UPDATE(IDC_LOCALIP_EDIT, OnUpdateLocalipEdit)
	ON_COMMAND(ID_FILE_EXIT, OnFileExit)
	ON_CBN_SELCHANGE(IDC_PROTOCOL_COMBO, OnSelchangeProtocolCombo)
	ON_EN_UPDATE(IDC_RMTTUNNELIP_EDIT, OnUpdateRmttunnelipEdit)
	ON_CBN_SELCHANGE(IDC_AUTHALG_COMBO, OnSelchangeAuthalgCombo)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CIPSecConfigDlg message handlers

BOOL CIPSecConfigDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon
	
	// TODO: Add extra initialization here

    CComboBox* pComboBox = (CComboBox *)GetDlgItem(IDC_DEVICETYPE_COMBO);
    ASSERT(pComboBox != NULL);
    pComboBox->AddString("HOST");
    pComboBox->AddString("GATEWAY"); 
    
    pComboBox = (CComboBox *)GetDlgItem(IDC_IPSECACTION_COMBO);
    ASSERT(pComboBox != NULL);
    pComboBox->AddString("APPLY");
    pComboBox->AddString("BYPASS"); 
    pComboBox->AddString("DISCARD"); 

    GetDlgItem(IDC_IPSECINTERFACE_EDIT)->EnableWindow(FALSE);
   
    pComboBox = (CComboBox *)GetDlgItem(IDC_IPSECMODE_COMBO);
    ASSERT(pComboBox != NULL);
    pComboBox->AddString("TRANSPORT");
    pComboBox->AddString("TUNNEL"); 
    
    pComboBox = (CComboBox *)GetDlgItem(IDC_IPSECPROTO_COMBO);
    ASSERT(pComboBox != NULL);
    pComboBox->AddString("AH");
    pComboBox->AddString("ESP"); 
    pComboBox->AddString("AH-ESP");
    
    pComboBox = (CComboBox *)GetDlgItem(IDC_AUTHALG_COMBO);
    ASSERT(pComboBox != NULL);
    pComboBox->AddString("HMAC-MD5");
    pComboBox->AddString("HMAC-MD5-96");
    pComboBox->AddString("HMAC-SHA1"); 
    pComboBox->AddString("HMAC-SHA1-96");  
    pComboBox->AddString("NULL");     
    
    pComboBox = (CComboBox *)GetDlgItem(IDC_PROTOCOL_COMBO);
    ASSERT(pComboBox != NULL);
    pComboBox->AddString("TCP");
    pComboBox->AddString("UDP");
    pComboBox->AddString("ICMP");
    pComboBox->AddString("*");
    
    pComboBox = (CComboBox *)GetDlgItem(IDC_RMTPORT_COMBO);
    ASSERT(pComboBox != NULL);
    pComboBox->AddString("*");
    pComboBox->EnableWindow(FALSE);
    
    pComboBox = (CComboBox *)GetDlgItem(IDC_LCLPORT_COMBO);
    ASSERT(pComboBox != NULL);
    pComboBox->AddString("*");
    pComboBox->EnableWindow(FALSE);

    GetDlgItem(IDC_RMTTUNNELIP_EDIT)->EnableWindow(FALSE);

    UpdateData(FALSE);
    
	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CIPSecConfigDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CIPSecConfigDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CIPSecConfigDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

void CIPSecConfigDlg::OnHelpAbout() 
{
	// TODO: Add your command handler code here

    CAboutDlg dlg;

    // Display About dialog box.
    dlg.DoModal();
	
}

void CIPSecConfigDlg::OnConfigureButton() 
{
	CString SPEntry, SAEntry;
    char pBuf[100];
    int SPEntryNum, SAEntryNum;
    int Length;
    char *Token;

    UpdateData(TRUE);
    
    // Get the highest SP and SA entry numbers.
    system("ipsec cg ~ipsec.res");

    CFile File;
    if(File.Open("~ipsec.res", CFile::modeRead) == FALSE)
    {
        return;
    }

    Length = File.GetLength();

    File.Read(pBuf, Length);
    
    File.Close();

    Token = strtok(pBuf, "-");

    SPEntryNum = atoi(Token) + 1;

    Token = strtok(NULL, "-");

    SAEntryNum = atoi(Token) + 1;

    // Set the undefined values.
    if(m_IPSecModeCombo == "TRANSPORT")
    {
        m_RemoteTunnelIPEdit = "*";
    }

    if(m_IPSecActionCombo == "BYPASS" || m_IPSecActionCombo == "DISCARD")
    {
        // Set the blank fields.
        m_IPSecProtoCombo = "NONE";
        m_IPSecModeCombo = "*";
        m_RemoteTunnelIPEdit = "*";
    }   
    
    if(m_ProtocolCombo == "*" || m_ProtocolCombo == "ICMP")
    {
        m_RemotePortCombo = "*";
        m_LocalPortCombo = "*";
    }
   
    if(m_DeviceTypeCombo == "GATEWAY" && m_IPSecInterfaceEdit == 0)
    {       
        MessageBox("The \"IPSec Interface\" cannot be \"0\", which is all interfaces.\n"
            "Set to a specific interface.\n");
        return;
    }

    if(m_RemoteTunnelIPEdit == "")
    {
        m_RemoteTunnelIPEdit = "*";
    }
    
    if(m_IPSecProtoCombo == "AH-ESP")
    {
        // Save SPD info.
        SPEntry.Format("%d %s %s %s %s %s %s %s %s %s %s %s %s %s %d %s %s %d ; %d %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %d ;", 
            SPEntryNum+1, "-", m_RemoteIPEdit, "-", m_LocalIPEdit, "-", m_ProtocolCombo, 
            "-", m_RemotePortCombo, "-", m_LocalPortCombo, 
            "ESP", m_IPSecModeCombo, 
            m_RemoteTunnelIPEdit, SPEntryNum, "BIDIRECT", m_IPSecActionCombo, 
            m_IPSecInterfaceEdit,
            SPEntryNum, "-", m_RemoteIPEdit, "-", m_LocalIPEdit, "-", m_ProtocolCombo, 
            "-", m_RemotePortCombo, "-", m_LocalPortCombo, 
            "AH", "TRANSPORT", 
            m_RemoteTunnelIPEdit, "NONE", "BIDIRECT", m_IPSecActionCombo, 
            m_IPSecInterfaceEdit);         
    }    
    else
    {
        // Save SPD info.
        SPEntry.Format("%d %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %d ; ", 
            SPEntryNum, "-", m_RemoteIPEdit, "-", m_LocalIPEdit, "-", m_ProtocolCombo, 
            "-", m_RemotePortCombo, "-", m_LocalPortCombo, 
            m_IPSecProtoCombo, m_IPSecModeCombo, 
            m_RemoteTunnelIPEdit, "NONE", "BIDIRECT", m_IPSecActionCombo, 
            m_IPSecInterfaceEdit); 
    }
        
    File.Open("~ipsec.spd", CFile::modeCreate | CFile::modeWrite);    

    File.Write(SPEntry, SPEntry.GetLength());
   
    File.Close();

    if(m_IPSecActionCombo == "APPLY")
    {
        // Check that SA remote and local addresses are present.
        if(m_RemoteSAAddrEdit == "*" || m_RemoteSAAddrEdit == "")
        {
            MessageBox("A \"Remote SA Address\" is needed for the SA.");
            return;
        }
        if(m_LocalSAAddrEdit == "*" || m_LocalSAAddrEdit == "")
        {
            MessageBox("A \"Local SA Address\" is needed for the SA.");
            return;
        }
        if(m_RemoteSPIEdit == 0)
        {
            MessageBox("\"Remote SPI\" cannot be \"0\".\n");
            return;
        }
        if(m_LocalSPIEdit == 0)
        {
            MessageBox("\"Local SPI\" cannot be \"0\".\n");
            return;
        }

        if(m_RemoteKeyFileEdit == "" && m_AuthAlgCombo != "NULL")
        {
            MessageBox("A \"Remote Key File\" is needed for the SA.");
            return;
        }
        else if(m_RemoteKeyFileEdit == "")
        {
            m_RemoteKeyFileEdit = "NONE";
        }

        if(m_LocalKeyFileEdit == "" && m_AuthAlgCombo != "NULL")
        {
            MessageBox("A \"Local Key File\" is needed for the SA.");
            return;
        }   
        else if(m_LocalKeyFileEdit == "")
        {
            m_LocalKeyFileEdit = "NONE";
        }

        if(m_IPSecProtoCombo == "AH-ESP")
        {
            // Save SAD info.
            SAEntry.Format("%d %d %s %s %s %s %s %s %s %s %s %d ; %d %d %s %s %s %s %s %s %s %s %s %d ; %d %d %s %s %s %s %s %s %s %s %s %d ; %d %d %s %s %s %s %s %s %s %s %s %d ;",
                SAEntryNum+3, m_RemoteSPIEdit, m_RemoteSAAddrEdit, 
                "POLICY", "POLICY", "POLICY", "POLICY", "POLICY",
                m_AuthAlgCombo, m_RemoteKeyFileEdit, "OUTBOUND", SPEntryNum+1,
                SAEntryNum+2, m_LocalSPIEdit, m_LocalSAAddrEdit, 
                "POLICY", "POLICY", "POLICY", "POLICY", "POLICY",
                m_AuthAlgCombo, m_LocalKeyFileEdit, "INBOUND", SPEntryNum+1,
                SAEntryNum+1, m_RemoteSPIEdit, m_RemoteSAAddrEdit, 
                "POLICY", "POLICY", "POLICY", "POLICY", "POLICY",
                m_AuthAlgCombo, m_RemoteKeyFileEdit, "OUTBOUND", SPEntryNum,
                SAEntryNum, m_LocalSPIEdit, m_LocalSAAddrEdit, 
                "POLICY", "POLICY", "POLICY", "POLICY", "POLICY",
                m_AuthAlgCombo, m_LocalKeyFileEdit, "INBOUND", SPEntryNum);           
        }   
        else
        {
            // Save SAD info.
            SAEntry.Format("%d %d %s %s %s %s %s %s %s %s %s %d ; %d %d %s %s %s %s %s %s %s %s %s %d ;",
                SAEntryNum+1, m_RemoteSPIEdit, m_RemoteSAAddrEdit, 
                "POLICY", "POLICY", "POLICY", "POLICY", "POLICY",
                m_AuthAlgCombo, m_RemoteKeyFileEdit, "OUTBOUND", SPEntryNum,
                SAEntryNum, m_LocalSPIEdit, m_LocalSAAddrEdit, 
                "POLICY", "POLICY", "POLICY", "POLICY", "POLICY",
                m_AuthAlgCombo, m_LocalKeyFileEdit, "INBOUND", SPEntryNum);   
        }            
    }
    else
    {
        SAEntry = "0";
    }

    File.Open("~ipsec.sad", CFile::modeCreate | CFile::modeWrite);      
    
    
    File.Write(SAEntry, SAEntry.GetLength());
    
    File.Close();

    system("ipsec ag ~ipsec");

    // Check the status by reading the result file.
    if(File.Open("~ipsec.res", CFile::modeRead) == FALSE)
    {
        return;
    }

    Length = File.GetLength();

    char ResultBuf[100] = "";
    char OkString[3] = "OK";

    File.Read(ResultBuf, Length);

    if(!(strcmp(ResultBuf, "OK")))
    {
        strcpy(ResultBuf, "SP and SA entries installed.\n");
    }  
    else
    {
        // Remove the SP entry since this was not successful.
        char DeleteSP[30] = "ipsec d sp ";        
        char StringNum[20] = "";       

        _itoa(SPEntryNum, StringNum, 10);
        strcat(DeleteSP, StringNum);
        system(DeleteSP);

        if(m_IPSecProtoCombo = "AH-ESP")
        {
            char DeleteSP2[30] = "ipsec d sp ";
            char StringNum2[20] = ""; 

            _itoa(SPEntryNum+1, StringNum2, 10);
            strcat(DeleteSP2, StringNum2);
            system(DeleteSP2);
        }
    }
    
    MessageBox(ResultBuf);
    
    File.Close();     
}


void CIPSecConfigDlg::OnSelchangeDevicetypeCombo() 
{
    CString OldEntry;    
    
    OldEntry = m_DeviceTypeCombo;
   
    UpdateData(TRUE);    

    // Compare the old to new.
    if(OldEntry == m_DeviceTypeCombo )
    {
        // Nothing to do since the combo box entry has not changed.
        return;
    }    
    
    if(m_DeviceTypeCombo == "GATEWAY")
    {  
        if(m_IPSecActionCombo == "APPLY")
        {
            // Disable selection.
            GetDlgItem(IDC_IPSECMODE_COMBO)->EnableWindow(FALSE);
            GetDlgItem(IDC_IPSECINTERFACE_EDIT)->EnableWindow(TRUE);

            if(m_IPSecModeCombo == "TRANSPORT")
            {                      
                GetDlgItem(IDC_RMTTUNNELIP_EDIT)->EnableWindow(TRUE); 
                m_IPSecModeCombo = "TUNNEL";
                m_RemoteTunnelIPEdit = "*";
            }          
        }   
        
        GetDlgItem(IDC_IPSECINTERFACE_EDIT)->EnableWindow(TRUE);
    }  
    else if(m_DeviceTypeCombo == "HOST")
    {         
        if(m_IPSecActionCombo == "APPLY")
        {
            // Enable selection.
            GetDlgItem(IDC_IPSECMODE_COMBO)->EnableWindow(TRUE);               
        }  
        
        GetDlgItem(IDC_IPSECINTERFACE_EDIT)->EnableWindow(FALSE);  
        m_IPSecInterfaceEdit = 0;
    }  
    
    UpdateData(FALSE);	
}

void CIPSecConfigDlg::OnSelchangeIpsecmodeCombo() 
{
	CString OldEntry; 
    
    OldEntry = m_IPSecModeCombo;
   
    UpdateData(TRUE);
    
    // Compare the old to new.
    if(OldEntry == m_IPSecModeCombo)
    {
        // Nothing to do since the combo box entry has not changed.
        return;
    }

    if(m_IPSecModeCombo == "TUNNEL")
    {
        GetDlgItem(IDC_RMTTUNNELIP_EDIT)->EnableWindow(TRUE);
        m_RemoteTunnelIPEdit = "*";
    }
    else if (m_IPSecModeCombo == "TRANSPORT")
    {
        GetDlgItem(IDC_RMTTUNNELIP_EDIT)->EnableWindow(FALSE);
        
        // change the remote SA address if it is the same.
        if(m_RemoteSAAddrEdit == m_RemoteTunnelIPEdit)
        {
            if(m_RemoteIPEdit != "*")
            {
                m_RemoteSAAddrEdit = m_RemoteIPEdit;
            }
            else
            {
                m_RemoteSAAddrEdit = "";
            }
        }
            
        m_RemoteTunnelIPEdit = "";
    }	

    UpdateData(FALSE);
}

void CIPSecConfigDlg::OnSelchangeIpsecactionCombo() 
{
	CString OldEntry;  
    
    OldEntry = m_IPSecActionCombo;
   
    UpdateData(TRUE);
    
    // Compare the old to new.
    if(OldEntry == m_IPSecActionCombo)
    {
        // Nothing to do since the combo box entry has not changed.
        return;
    }

    if(m_IPSecActionCombo == "APPLY")
    {
        // Enable combos
        if(m_DeviceTypeCombo == "HOST")
        {
            GetDlgItem(IDC_IPSECMODE_COMBO)->EnableWindow(TRUE);
            m_IPSecModeCombo = "Transport";            
            GetDlgItem(IDC_RMTTUNNELIP_EDIT)->EnableWindow(FALSE); 
        }
        else if(m_DeviceTypeCombo == "GATEWAY")
        {
            m_IPSecModeCombo = "TUNNEL";
            GetDlgItem(IDC_RMTTUNNELIP_EDIT)->EnableWindow(TRUE); 
            m_RemoteTunnelIPEdit = "*";
        }

        GetDlgItem(IDC_IPSECPROTO_COMBO)->EnableWindow(TRUE);
        m_IPSecProtoCombo = "AH";
        GetDlgItem(IDC_AUTHALG_COMBO)->EnableWindow(TRUE);
        m_AuthAlgCombo = "HMAC-MD5";
        GetDlgItem(IDC_RMTSAADDR_EDIT)->EnableWindow(TRUE);
        GetDlgItem(IDC_LCLSAADDR_EDIT)->EnableWindow(TRUE);
        GetDlgItem(IDC_REMOTEKEYFILE_EDIT)->EnableWindow(TRUE); 
        GetDlgItem(IDC_LOCALKEYFILE_EDIT)->EnableWindow(TRUE);
        GetDlgItem(IDC_REMOTESPI_EDIT)->EnableWindow(TRUE);
        GetDlgItem(IDC_LOCALSPI_EDIT)->EnableWindow(TRUE);
    }
    else if (m_IPSecActionCombo == "BYPASS" || m_IPSecActionCombo == "DISCARD" )
    {        
        GetDlgItem(IDC_IPSECMODE_COMBO)->EnableWindow(FALSE);        
        GetDlgItem(IDC_IPSECPROTO_COMBO)->EnableWindow(FALSE);        
        GetDlgItem(IDC_AUTHALG_COMBO)->EnableWindow(FALSE);        
        GetDlgItem(IDC_RMTTUNNELIP_EDIT)->EnableWindow(FALSE);        
        GetDlgItem(IDC_RMTSAADDR_EDIT)->EnableWindow(FALSE);
        GetDlgItem(IDC_LCLSAADDR_EDIT)->EnableWindow(FALSE);
        GetDlgItem(IDC_LOCALKEYFILE_EDIT)->EnableWindow(FALSE);
        GetDlgItem(IDC_REMOTEKEYFILE_EDIT)->EnableWindow(FALSE); 
        GetDlgItem(IDC_REMOTESPI_EDIT)->EnableWindow(FALSE);
        GetDlgItem(IDC_LOCALSPI_EDIT)->EnableWindow(FALSE);
        m_RemoteSAAddrEdit = "";
        m_LocalSAAddrEdit = "";
        m_RemoteKeyFileEdit = "";
        m_LocalKeyFileEdit = "";
        m_RemoteTunnelIPEdit = "";
    }
    
    UpdateData(FALSE);
}

void CIPSecConfigDlg::OnUpdateRemoteipEdit() 
{
    // Change the Remote SA Address edit box to match this input.
    UpdateData(TRUE);

    if(m_RemoteTunnelIPEdit == "*" || m_RemoteTunnelIPEdit == "")
    {    
        m_RemoteSAAddrEdit = m_RemoteIPEdit;
    }

    UpdateData(FALSE);
	
}

void CIPSecConfigDlg::OnUpdateLocalipEdit() 
{
    // Change the Local SA Address edit box to match this input.
    UpdateData(TRUE);
    
    m_LocalSAAddrEdit = m_LocalIPEdit;

    UpdateData(FALSE);
	
}

void CIPSecConfigDlg::OnFileExit() 
{
    OnOK();	
}

void CIPSecConfigDlg::OnSelchangeProtocolCombo() 
{
    CString OldEntry = m_ProtocolCombo;

    UpdateData(TRUE);

    // Compare the old to new.
    if(OldEntry == m_ProtocolCombo)
    {
        // Nothing to do since the combo box entry has not changed.
        return;
    }

    if(m_ProtocolCombo == "*" || m_ProtocolCombo == "ICMP")
    {
        GetDlgItem(IDC_RMTPORT_COMBO)->EnableWindow(FALSE);
        m_RemotePortCombo = "";
        GetDlgItem(IDC_LCLPORT_COMBO)->EnableWindow(FALSE);
        m_LocalPortCombo = "";
    }
    else
    {
        GetDlgItem(IDC_RMTPORT_COMBO)->EnableWindow(TRUE);
        m_RemotePortCombo = "*";
        GetDlgItem(IDC_LCLPORT_COMBO)->EnableWindow(TRUE);
        m_LocalPortCombo = "*";
    }

    UpdateData(FALSE);
}

void CIPSecConfigDlg::OnUpdateRmttunnelipEdit() 
{
	// Change the Remote SA Address edit box to match this input.
    UpdateData(TRUE);
    
    m_RemoteSAAddrEdit = m_RemoteTunnelIPEdit;

    UpdateData(FALSE);
	
}

void CIPSecConfigDlg::OnSelchangeAuthalgCombo() 
{
	CString OldEntry = m_AuthAlgCombo;    
   
    UpdateData(TRUE);    

    // Compare the old to new.
    if(OldEntry == m_AuthAlgCombo)
    {
        // Nothing to do since the combo box entry has not changed.
        return;
    }

    if(m_AuthAlgCombo == "NULL")
    {
        GetDlgItem(IDC_REMOTEKEYFILE_EDIT)->EnableWindow(FALSE);
        m_RemoteKeyFileEdit = "";
        GetDlgItem(IDC_LOCALKEYFILE_EDIT)->EnableWindow(FALSE);
        m_LocalKeyFileEdit = "";
    }
    else
    {
        GetDlgItem(IDC_REMOTEKEYFILE_EDIT)->EnableWindow(TRUE);        
        GetDlgItem(IDC_LOCALKEYFILE_EDIT)->EnableWindow(TRUE);
    }

    UpdateData(FALSE);	
}
