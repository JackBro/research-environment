

namespace System.Extensions.Windows.Forms
{
  using System;
  using System.Drawing;
  using System.Runtime.InteropServices;
  using System.Security.Permissions;
  using WF = System.Windows.Forms;

  /// <summary>
  /// Tight wrapper around stock <see cref="WF.TreeView" /> control.  Enforces Vista drawing and styles as needed.
  /// </summary>
  [ToolboxBitmap(typeof(WF.TreeView))]
  public class TreeView : WF.TreeView
  {
    /// <summary>
    /// Overridden from base to add Vista theming.
    /// </summary>
    protected override WF.CreateParams CreateParams
    {
      [SecurityPermission(SecurityAction.LinkDemand, Flags = SecurityPermissionFlag.UnmanagedCode)]
      get
      {
        WF.CreateParams cp = base.CreateParams;
        if (NativeMethods.CanDrawVistaStyles)
        {
          cp.Style |= NativeMethods.TVS_NOHSCROLL; // Vista doesn't use horizontal scroll-bar
        }

        return cp;
      }
    }

    /// <summary>
    /// Overridden from base to add Vista theming.
    /// </summary>
    protected override void OnHandleCreated(EventArgs e)
    {
      base.OnHandleCreated(e);

      if (NativeMethods.CanDrawVistaStyles)
      {
        HandleRef handle = new HandleRef(this, Handle);

        // Vista has autoscroll horizontally, auto hide the glyphs, and double buffered
        int currentStyle = NativeMethods.SendMessage(handle, NativeMethods.TVM_GETEXTENDEDSTYLE, IntPtr.Zero, IntPtr.Zero).ToInt32();
        currentStyle |= NativeMethods.TVS_EX_AUTOHSCROLL | NativeMethods.TVS_EX_FADEINOUTEXPANDOS | NativeMethods.TVS_EX_DOUBLEBUFFER;
        NativeMethods.SendMessage(handle, NativeMethods.TVM_SETEXTENDEDSTYLE, IntPtr.Zero, new IntPtr(currentStyle));

        // OS theming (pretty glyphs)
        NativeMethods.SetWindowTheme(handle, "explorer", null);

        HotTracking = true; // to allow for Vista animations
        ShowLines = false; // Vista doesn't show lines
        HideSelection = false; // Vista always shows
      }
    }
  }
}