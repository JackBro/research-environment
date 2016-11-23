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

  /// <summary>
  /// Provides formatting for <see cref="Boolean"/> types.
  /// </summary>
  public class BooleanFormatProvider : IFormatProvider, ICustomFormatter
  {
    /// <summary>
    /// Gets a formatter based on the requested type.
    /// </summary>
    /// <param name="formatType">The requested type.</param>
    /// <returns>The formatter if one is found.</returns>
    public object GetFormat(Type formatType)
    {
      if (formatType == typeof(ICustomFormatter))
      {
        return this;
      }
      else
      {
        return null;
      }
    }

    /// <summary>
    /// Formats a <see cref="Boolean"/> based on the supplied format strings.
    /// </summary>
    /// <param name="format">The format in which to format.</param>
    /// <param name="arg">The value to format.</param>
    /// <param name="formatProvider">The formatter to use.</param>
    /// <returns>A formatted string.</returns>
    public string Format(string format, object arg, IFormatProvider formatProvider)
    {
      if (!(arg is bool))
      {
        throw new ArgumentException("Must be a boolean type", "arg");
      }

      bool value = (bool)arg;
      format = format == null ? string.Empty : format.Trim();

      if (string.Equals(format, "yn", StringComparison.OrdinalIgnoreCase))
      {
        return value ? "Yes" : "No";
      }

      string[] parts = format.Split('|');
      if ((parts.Length == 1 && !string.IsNullOrEmpty(format)) || parts.Length > 2)
      {
        throw new FormatException("Format string is invalid>");
      }

      if (parts.Length == 2)
      {
        return value ? parts[0] : parts[1];
      }
      else
      {
        return arg.ToString();
      }
    }
  }
}
