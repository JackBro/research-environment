

namespace System.Extensions.Windows.Forms
{
  using System;
  using System.Runtime.InteropServices;
  using System.Windows.Forms;

  internal static class NativeMethods
  {
    public static bool IsVistaOrLater
    {
      get { return Environment.OSVersion.Platform == PlatformID.Win32NT && Environment.OSVersion.Version.Major >= 6; }
    }

    public static bool CanDrawVistaStyles
    {
      get { return IsVistaOrLater && Application.RenderWithVisualStyles; }
    }

    #region Buttons

    public const int BS_COMMANDLINK = 0x0000000E;
    public const int BS_ICON = 0x00000040;

    public const int BCM_SETNOTE = 0x00001609;
    public const int BCM_SETSHIELD = 0x0000160C;
    public const int BM_SETIMAGE = 0x00F7;

    #endregion

    public const int LVM_FIRST = 0x1000;
    public const int LVM_SETEXTENDEDLISTVIEWSTYLE = LVM_FIRST + 54;
    public const int LVM_GETEXTENDEDLISTVIEWSTYLE = LVM_FIRST + 55;

    public const int LVS_EX_DOUBLEBUFFER = 0x00010000;

    public const int TV_FIRST = 0x1100;
    public const int TVM_SETEXTENDEDSTYLE = TV_FIRST + 44;
    public const int TVM_GETEXTENDEDSTYLE = TV_FIRST + 45;
    public const int TVM_SETAUTOSCROLLINFO = TV_FIRST + 59;

    public const int TVS_NOHSCROLL = 0x8000;
    public const int TVS_EX_DOUBLEBUFFER = 0x0004;
    public const int TVS_EX_AUTOHSCROLL = 0x0020;
    public const int TVS_EX_FADEINOUTEXPANDOS = 0x0040;

    [DllImport("user32.dll", CharSet = CharSet.Auto, SetLastError = false)]
    public static extern IntPtr SendMessage(HandleRef hWnd, uint Msg, IntPtr wParam, IntPtr lParam);

    [DllImport("user32.dll", CharSet = CharSet.Unicode, SetLastError = false)]
    public static extern IntPtr SendMessage(HandleRef hWnd, uint Msg, IntPtr wParam, string lParam);

    [DllImport("uxtheme.dll", CharSet = CharSet.Unicode)]
    public static extern int SetWindowTheme(HandleRef hWnd, string pszSubAppName, string pszSubIdList);

    public const int ECM_FIRST = 0x1500;
    public const int EM_SETCUEBANNER = ECM_FIRST + 1;

    public static bool EditSetCueBanner(Control control, string cueBannerText, bool showCueBannerWhenFocused)
    {
      if (control.IsHandleCreated)
      {
        HandleRef handle = new HandleRef(control, control.Handle);
        IntPtr showWhenFocused = new IntPtr(showCueBannerWhenFocused ? 1 : 0);
        IntPtr result = SendMessage(handle, NativeMethods.EM_SETCUEBANNER, showWhenFocused, cueBannerText);
        if (result == IntPtr.Zero)
        {
          return false;
        }
      }

      return true;
    }
  }
}
