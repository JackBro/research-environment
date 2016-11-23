/* Copyright (c) 2015 Jonathan Moore
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
*/
namespace System.Extensions
{
  using System;
  using System.Runtime.InteropServices;

  /// <summary>
  /// Wraps P/Invoke methods.
  /// </summary>
  internal static class NativeMethods
  {
    private const ushort FOF_ALLOWUNDO = 0x40;
    private const ushort FOF_NOCONFIRMATION = 0x10;

    private const int SHARD_PATH = 0x2;

    private enum FO_Func : uint
    {
      FO_MOVE = 0x0001,
      FO_COPY = 0x0002,
      FO_DELETE = 0x0003,
      FO_RENAME = 0x0004,
    }
    
    public static void SHAddToRecentDocs(string fileName)
    {
      SHAddToRecentDocs(SHARD_PATH, fileName);
    }

    public static void DeleteToRecycleBin(string fileName, bool showConfirmation)
    {
      var shf = new SHFILEOPSTRUCT();
      shf.hwnd = IntPtr.Zero;
      shf.wFunc = FO_Func.FO_DELETE;
      if (showConfirmation)
      {
        shf.fFlags = FOF_ALLOWUNDO;
      }
      else
      {
        shf.fFlags = FOF_ALLOWUNDO | FOF_NOCONFIRMATION;
      }

      shf.pFrom = fileName + '\0';
      SHFileOperation(ref shf);
    }

    [DllImport("shell32.dll", CharSet = CharSet.Unicode)]
    private static extern int SHFileOperation(ref SHFILEOPSTRUCT FileOp);

        [DllImport("shell32.dll")]
        static extern void SHAddToRecentDocs(uint uFlags, string pv);

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
    private struct SHFILEOPSTRUCT
    {
      public IntPtr hwnd;
      public FO_Func wFunc;
      [MarshalAs(UnmanagedType.LPWStr)]
      public string pFrom;
      [MarshalAs(UnmanagedType.LPWStr)]
      public string pTo;
      public ushort fFlags;
      public int fAnyOperationsAborted;
      public IntPtr hNameMappings;
      [MarshalAs(UnmanagedType.LPWStr)]
      public string lpszProgressTitle;
    }
  }
}
