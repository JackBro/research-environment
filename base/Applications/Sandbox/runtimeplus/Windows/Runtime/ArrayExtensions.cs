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
  using System.Linq;
  using System.Text;

  /// <summary>
  /// Extension methods for an <see cref="Array"/>.
  /// </summary>
  public static class ArrayExtensions
  {
    /// <summary>
    /// Indicates whether the specified <see cref="Array"/> is null or has a zero length.
    /// </summary>
    /// <param name="array">The <see cref="Array"/> to check.</param>
    /// <returns><b>True</b> if the <see cref="Array"/> is null or has a zero length; <b>false</b> otherwise.</returns>
    public static bool IsNullOrEmpty(this Array array)
    {
      return array == null || array.Length == 0;
    }

    /// <summary>
    /// Formats the array as a string, each element delimited by a seperator.
    /// </summary>
    /// <typeparam name="T">The <see cref="Type"/> of the array.</typeparam>
    /// <param name="array">The array to format.</param>
    /// <param name="format">The format specifier for each element of the array.</param>
    /// <param name="delimiter">The delimiting string to use.</param>
    /// <returns>The array, formatted as as string, each element delimited by a seperator.</returns>
    public static string ToDelimitedString<T>(this T[] array, string format, string delimiter)
    {
      if (array == null || array.Length == 0)
      {
        return string.Empty;
      }

      StringBuilder builder = new StringBuilder();
      for (int index = 0; index < array.Length; ++index)
      {
        if (index != 0)
        {
          builder.Append(delimiter);
        }

        builder.AppendFormat(format, array[index]);
      }

      return builder.ToString();
    }

    /// <summary>
    /// Converts the given byte array to pure hex digits.  { 0, 255 } becomes 00FF
    /// </summary>
    /// <param name="array">The byte array to convert to hex.</param>
    /// <returns>The hex string representation of the byte array.</returns>
    public static string ToHexString(this byte[] array)
    {
      if (IsNullOrEmpty(array))
      {
        return string.Empty;
      }

      StringBuilder hex = new StringBuilder(array.Length * 2);
      foreach (byte b in array)
      {
        hex.AppendFormat("{0:x2}", b);
      }

      return hex.ToString();
    }

    /// <summary>
    /// Combines a series of arrays of the same type into a single array.
    /// </summary>
    /// <typeparam name="T">The <see cref="Type"/> of the array.</typeparam>
    /// <param name="arrays">The arrays to combine.</param>
    /// <returns>A single array of type T, with all of the contents of the arrays.</returns>
    public static T[] Combine<T>(params T[][] arrays)
    {
      T[] rv = new T[arrays.Sum(a => a.Length)];
      int offset = 0;
      foreach (T[] array in arrays)
      {
        Array.Copy(array, 0, rv, offset, array.Length);
        offset += array.Length;
      }

      return rv;
    }
  }
}
