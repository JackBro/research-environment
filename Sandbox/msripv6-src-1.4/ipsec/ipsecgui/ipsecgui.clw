; CLW file contains information for the MFC ClassWizard

[General Info]
Version=1
LastClass=CIPSecConfigDlg
LastTemplate=CComboBox
NewFileInclude1=#include "stdafx.h"
NewFileInclude2=#include "IPSecConfig.h"

ClassCount=4
Class1=CIPSecConfigApp
Class2=CIPSecConfigDlg
Class3=CAboutDlg

ResourceCount=4
Resource1=IDD_IPSECCONFIG_DIALOG
Resource2=IDR_MAINFRAME
Resource3=IDD_ABOUTBOX
Class4=CDeviceTypeComboBox
Resource4=IDR_MENU1

[CLS:CIPSecConfigApp]
Type=0
HeaderFile=IPSecConfig.h
ImplementationFile=IPSecConfig.cpp
Filter=N
LastObject=CIPSecConfigApp

[CLS:CIPSecConfigDlg]
Type=0
HeaderFile=IPSecConfigDlg.h
ImplementationFile=IPSecConfigDlg.cpp
Filter=D
LastObject=IDC_AUTHALG_COMBO
BaseClass=CDialog
VirtualFilter=dWC

[CLS:CAboutDlg]
Type=0
HeaderFile=IPSecConfigDlg.h
ImplementationFile=IPSecConfigDlg.cpp
Filter=D

[DLG:IDD_ABOUTBOX]
Type=1
Class=CAboutDlg
ControlCount=4
Control1=IDC_STATIC,static,1342177283
Control2=IDC_STATIC,static,1342308480
Control3=IDC_STATIC,static,1342308352
Control4=IDOK,button,1342373889

[DLG:IDD_IPSECCONFIG_DIALOG]
Type=1
Class=CIPSecConfigDlg
ControlCount=41
Control1=IDC_DEVICETYPE_COMBO,combobox,1344340035
Control2=IDC_STATIC,static,1342308352
Control3=IDC_CONFIGURE_BUTTON,button,1342242816
Control4=IDC_IPSECMODE_COMBO,combobox,1344339971
Control5=IDC_STATIC,static,1342308352
Control6=IDC_IPSECPROTO_COMBO,combobox,1344339971
Control7=IDC_STATIC,static,1342308352
Control8=IDC_STATIC,static,1342308352
Control9=IDC_STATIC,static,1342308352
Control10=IDC_RMTPORT_COMBO,combobox,1344340034
Control11=IDC_STATIC,static,1342308352
Control12=IDC_LCLPORT_COMBO,combobox,1344340290
Control13=IDC_STATIC,static,1342308352
Control14=IDC_PROTOCOL_COMBO,combobox,1344339971
Control15=IDC_STATIC,static,1342308352
Control16=IDC_AUTHALG_COMBO,combobox,1344339971
Control17=IDC_STATIC,static,1342308352
Control18=IDC_STATIC,static,1342308352
Control19=IDC_RMTTUNNELIP_EDIT,edit,1350631552
Control20=IDC_IPSECACTION_COMBO,combobox,1344339971
Control21=IDC_STATIC,static,1342308352
Control22=IDC_STATIC,static,1342308352
Control23=IDC_IPSECINTERFACE_EDIT,edit,1350631552
Control24=IDC_STATIC,button,1342177287
Control25=IDC_STATIC,button,1342177287
Control26=IDC_STATIC,button,1342177287
Control27=IDC_REMOTEIP_EDIT,edit,1350631552
Control28=IDC_LOCALIP_EDIT,edit,1350631552
Control29=IDC_REMOTEKEYFILE_EDIT,edit,1350631552
Control30=IDC_STATIC,static,1342308352
Control31=IDC_LCLSAADDR_EDIT,edit,1350631552
Control32=IDC_STATIC,static,1342308352
Control33=IDC_RMTSAADDR_EDIT,edit,1350631552
Control34=IDC_STATIC,static,1342308352
Control35=IDC_STATIC,button,1342177287
Control36=IDC_LOCALSPI_EDIT,edit,1350631552
Control37=IDC_STATIC,static,1342308352
Control38=IDC_REMOTESPI_EDIT,edit,1350631552
Control39=IDC_STATIC,static,1342308352
Control40=IDC_LOCALKEYFILE_EDIT,edit,1350631552
Control41=IDC_STATIC,static,1342308352

[MNU:IDR_MENU1]
Type=1
Class=CIPSecConfigDlg
Command1=ID_FILE_EXIT
Command2=ID_HELP_ABOUT
CommandCount=2

[CLS:CDeviceTypeComboBox]
Type=0
HeaderFile=DeviceTypeComboBox.h
ImplementationFile=DeviceTypeComboBox.cpp
BaseClass=CComboBox
Filter=W
LastObject=CDeviceTypeComboBox

