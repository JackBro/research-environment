// IPSecConfigDlg.h : header file
//

#if !defined(AFX_IPSECCONFIGDLG_H__DEEAD127_F0D6_11D2_A344_00A0C9CA5D48__INCLUDED_)
#define AFX_IPSECCONFIGDLG_H__DEEAD127_F0D6_11D2_A344_00A0C9CA5D48__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CIPSecConfigDlg dialog

class CIPSecConfigDlg : public CDialog
{
// Construction
public:
	CIPSecConfigDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CIPSecConfigDlg)
	enum { IDD = IDD_IPSECCONFIG_DIALOG };
	CString	m_DeviceTypeCombo;    
	CString	m_IPSecProtoCombo;
	CString	m_IPSecModeCombo;
	CString	m_LocalPortCombo;
	CString	m_ProtocolCombo;
	CString	m_RemotePortCombo;
	CString	m_AuthAlgCombo;	
	CString	m_RemoteTunnelIPEdit;
	CString	m_IPSecActionCombo;
	int		m_IPSecInterfaceEdit;
	CString	m_LocalIPEdit;
	CString	m_RemoteIPEdit;
	CString	m_RemoteSAAddrEdit;
	CString	m_LocalSAAddrEdit;
	long	m_RemoteSPIEdit;
	long	m_LocalSPIEdit;
	CString	m_LocalKeyFileEdit;
	CString	m_RemoteKeyFileEdit;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CIPSecConfigDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	//{{AFX_MSG(CIPSecConfigDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnHelpAbout();
	afx_msg void OnConfigureButton();
	afx_msg void OnSelchangeDevicetypeCombo();
	afx_msg void OnSelchangeIpsecmodeCombo();
	afx_msg void OnSelchangeIpsecactionCombo();
	afx_msg void OnUpdateRemoteipEdit();
	afx_msg void OnUpdateLocalipEdit();
	afx_msg void OnFileExit();
	afx_msg void OnSelchangeProtocolCombo();
	afx_msg void OnUpdateRmttunnelipEdit();
	afx_msg void OnSelchangeAuthalgCombo();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_IPSECCONFIGDLG_H__DEEAD127_F0D6_11D2_A344_00A0C9CA5D48__INCLUDED_)
