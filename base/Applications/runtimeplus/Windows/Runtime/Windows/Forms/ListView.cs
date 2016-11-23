

namespace System.Extensions.Windows.Forms
{
  using System;
  using System.Drawing;
  using System.Runtime.InteropServices;
  using WF = System.Windows.Forms;

  /// <summary>
  /// Tight wrapper around stock <see cref="WF.ListView" /> control.  Enforces Vista drawing and styles as needed.
  /// </summary>
  [ToolboxBitmap(typeof(WF.ListView))]
  public class ListView : WF.ListView
  {
    /// <summary>
    /// Overridden from base to add Vista theming.
    /// </summary>
    /// <param name="e">EventArgs of the event.</param>
    protected override void OnHandleCreated(EventArgs e)
    {
      base.OnHandleCreated(e);

      if (NativeMethods.CanDrawVistaStyles)
      {
        HandleRef handle = new HandleRef(this, Handle);
        int currentStyle = NativeMethods.SendMessage(handle, NativeMethods.LVM_GETEXTENDEDLISTVIEWSTYLE, IntPtr.Zero, IntPtr.Zero).ToInt32();
        currentStyle |= NativeMethods.LVS_EX_DOUBLEBUFFER;
        NativeMethods.SendMessage(handle, NativeMethods.LVM_SETEXTENDEDLISTVIEWSTYLE, IntPtr.Zero, new IntPtr(currentStyle));

        // let OS theme take over
        NativeMethods.SetWindowTheme(handle, "explorer", null);
      }
    }
  }
}
