/// Copyright (c) 2015 Jonathan Moore
///
/// This software is provided 'as-is', without any express or implied warranty. 
/// In no event will the authors be held liable for any damages arising from 
/// the use of this software.
/// 
/// Permission is granted to anyone to use this software for any purpose, 
/// including commercial applications, and to alter it and redistribute it 
/// freely, subject to the following restrictions:
///
/// 1. The origin of this software must not be misrepresented; 
/// you must not claim that you wrote the original software. 
/// If you use this software in a product, an acknowledgment in the product 
/// documentation would be appreciated but is not required.
/// 
/// 2. Altered source versions must be plainly marked as such, 
/// and must not be misrepresented as being the original software.
///
///3. This notice may not be removed or altered from any source distribution.

namespace System.Extensions.Compact
{
  using System;
  using System.Runtime.InteropServices;

  /// <summary>
  /// Provides methods for turning the display on and off on a smart client.
  /// </summary>
  public static class Video
  {
    /// <summary>
    /// Turn the display off.
    /// </summary>
    public static void PowerOff()
    {
      IntPtr hDc = NativeMethods.GetDC(IntPtr.Zero);
      byte[] vpm = new byte[] { 12, 0, 0, 0, 1, 0, 0, 0, (byte)NativeMethods.VideoPowerState.VideoPowerOff, 0, 0, 0, 0 };
      NativeMethods.ExtEscape(hDc, NativeMethods.SETPOWERMANAGEMENT, 12, vpm, 0, IntPtr.Zero);
      NativeMethods.ReleaseDC(IntPtr.Zero, hDc);
    }

    /// <summary>
    /// Turn the display on.
    /// </summary>
    public static void PowerOn()
    {
      IntPtr hDc = NativeMethods.GetDC(IntPtr.Zero);
      byte[] vpm = new byte[] { 12, 0, 0, 0, 1, 0, 0, 0, (byte)NativeMethods.VideoPowerState.VideoPowerOn, 0, 0, 0, 0 };
      NativeMethods.ExtEscape(hDc, NativeMethods.SETPOWERMANAGEMENT, 12, vpm, 0, IntPtr.Zero);
      NativeMethods.ReleaseDC(IntPtr.Zero, hDc);
    }
  }
}
