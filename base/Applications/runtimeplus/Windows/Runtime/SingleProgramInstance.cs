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
  using System.Diagnostics;
  using System.Reflection;
  using System.Runtime.InteropServices;
  using System.Threading;

  /// <summary>
  /// <strong>SingleProgamInstance</strong> uses a mutex synchronization object
  /// to ensure that only one copy of process is running at
  /// a particular time. It also allows for UI identification
  /// of the intial process by bring that window to the foreground.
  /// </summary>
  public sealed class SingleProgramInstance : IDisposable
  {
        #region Platform Invoke

        [DllImport("user32.dll")]
        static extern bool SetForegroundWindow(IntPtr hWnd);
        [DllImport("user32.dll")]
        static extern bool ShowWindowAsync(IntPtr hWnd, int nCmdShow);
        [DllImport("user32.dll")]
        static extern bool IsIconic(IntPtr hWnd);

        private const int SW_RESTORE = 9;

    #endregion

    private Mutex processSync;

    /// <summary>
    /// Initializes a new instance of the <see cref="SingleProgramInstance"/> class.
    /// </summary>
    public SingleProgramInstance()
      : this(string.Empty)
    {
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="SingleProgramInstance"/> class using the specified string as the process identifier.
    /// </summary>
    /// <param name="identifier">The string that represents the process identifier.</param>
    public SingleProgramInstance(string identifier)
    {
      this.processSync = new Mutex(false, Assembly.GetEntryAssembly().GetName().Name + identifier);
    }

    /// <summary>
    /// Gets a value indicating whether this instance is single instance.
    /// </summary>
    /// <value>
    ///   <c>true</c> if this instance is single instance; otherwise, <c>false</c>.
    /// </value>
    public bool IsSingleInstance
    {
      get
      {
        if (this.processSync.WaitOne(0, false))
        {
          return true;
        }
        else
        {
          return false;
        }
      }
    }

    /// <summary>
    /// If another istance of the application is already running, raises the other process.
    /// </summary>
    public void RaiseOtherProcess()
    {
      var proc = Process.GetCurrentProcess();

      // Using Process.ProcessName does not function properly when
      // the name exceeds 15 characters. Using the assembly name
      // takes care of this problem and is more accruate than other
      // work arounds.
      string assemblyName = Assembly.GetEntryAssembly().GetName().Name;
      foreach (Process otherProc in Process.GetProcessesByName(assemblyName))
      {
        // ignore this process
        if (proc.Id != otherProc.Id)
        {
          // Found a "same named process".
          // Assume it is the one we want brought to the foreground.
          // Use the Win32 API to bring it to the foreground.
          IntPtr hWnd = otherProc.MainWindowHandle;
          if (IsIconic(hWnd))
          {
            ShowWindowAsync(hWnd, SW_RESTORE);
          }

          SetForegroundWindow(hWnd);
          return;
        }
      }
    }

    #region Implementation of IDisposable

    /// <summary>
    /// Releases unmanaged resources and performs other cleanup operations before the
    /// <see cref="SingleProgramInstance"/> is reclaimed by garbage collection.
    /// </summary>
    ~SingleProgramInstance()
    {
      // Release mutex (if necessary) 
      // This should have been accomplished using Dispose() 
      this.FreeResources();
    }

    private void FreeResources()
    {
      try
      {
        if (this.processSync.WaitOne(0, false))
        {
          // If we own the mutex than release it so that
          // other "same" processes can now start.
          this.processSync.ReleaseMutex();
        }

        this.processSync.Close();
      }
      catch
      {
      }
    }

    /// <summary>
    /// Performs application-defined tasks associated with freeing, releasing, or resetting unmanaged resources.
    /// </summary>
    public void Dispose()
    {
      // release mutex (if necessary) and notify 
      // the garbage collector to ignore the destructor
      this.FreeResources();
      GC.SuppressFinalize(this);
    }

    #endregion
  }
}
