// IPSecConfig.h : main header file for the IPSECCONFIG application
//

#if !defined(AFX_IPSECCONFIG_H__DEEAD125_F0D6_11D2_A344_00A0C9CA5D48__INCLUDED_)
#define AFX_IPSECCONFIG_H__DEEAD125_F0D6_11D2_A344_00A0C9CA5D48__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CIPSecConfigApp:
// See IPSecConfig.cpp for the implementation of this class
//

class CIPSecConfigApp : public CWinApp
{
public:
	CIPSecConfigApp();
    

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CIPSecConfigApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CIPSecConfigApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_IPSECCONFIG_H__DEEAD125_F0D6_11D2_A344_00A0C9CA5D48__INCLUDED_)
