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

namespace System.Extensions
{
  using System;
  using System.ComponentModel;

  /// <summary>
  /// Represents the method of a cancellable event that has a piece of data.
  /// </summary>
  /// <typeparam name="T">The type of the data attached to the event.</typeparam>
  /// <param name="sender">The source of the event.</param>
  /// <param name="e">The event arguments with attached data.</param>
  [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Design", "CA1003:UseGenericEventHandlerInstances", Justification = "Without this delegate, usage would require double nesting of generics.")]
  public delegate void DataCancelEventHandler<T>(object sender, DataCancelEventArgs<T> e);

  /// <summary>
  /// Represents a CancelEventArgs that has a piece of data attached to it.
  /// </summary>
  /// <typeparam name="T">The type of the data attached to the event.</typeparam>
  public class DataCancelEventArgs<T> : CancelEventArgs
  {
    /// <summary>
    /// Initializes a new instance of the <see cref="DataCancelEventArgs{T}"/> class.
    /// </summary>
    /// <param name="data">The data to store with the EventArgs.</param>
    /// <exception cref="ArgumentNullException">If <em>data</em> is null.</exception>
    public DataCancelEventArgs(T data)
    {
      if (data == null)
      {
        throw new ArgumentNullException("data");
      }

      this.Data = data;
    }

    /// <summary>
    /// Gets the data associated with this EventArgs
    /// </summary>
    public T Data
    {
      get;
      private set;
    }
  }
}
