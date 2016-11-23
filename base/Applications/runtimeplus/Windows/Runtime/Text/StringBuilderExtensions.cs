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

namespace System.Extensions.Text
{
  using System;
  using System.Text;
  using System.Globalization;

  /// <summary>
  /// Extension methods for a <see cref="StringBuilder"/>.
  /// </summary> 
  public static class StringBuilderExtensions
  {
    /// <summary>
    /// Appends a formatted string and the default line terminator, which contains zero or more format specifications, to this instance.  Each format specification is replaced by the string representation of a corresponding object argument.
    /// </summary>
    /// <param name="builder">The <see cref="StringBuilder"/> instance to append text to.</param>
    /// <param name="format">A string containing zero or more format specifications.</param>
    /// <param name="arg0">An object to format.</param>
    /// <returns>A reference to this instance with format appended.  Any format specifications in format is replaced by the string representation of the corresponding object argument.</returns>
    /// <exception cref="ArgumentNullException">format is null.</exception>
    /// <exception cref="ArgumentOutOfRangeException">Enlarging the value of this instance would exceed <see cref="StringBuilder.MaxCapacity"/>.</exception>
    /// <exception cref="FormatException">format is invalid.</exception>
    public static StringBuilder AppendFormatLine(this StringBuilder builder, string format, object arg0)
    {
      builder.AppendFormat(format, arg0);
      return builder.AppendLine();
    }

    /// <summary>
    /// Appends a formatted string and the default line terminator, which contains zero or more format specifications, to this instance.  Each format specification is replaced by the string representation of a corresponding object argument.
    /// </summary>
    /// <param name="builder">The <see cref="StringBuilder"/> instance to append text to.</param>
    /// <param name="format">A string containing zero or more format specifications.</param>
    /// <param name="args">An array of objects to format.</param>
    /// <returns>A reference to this instance with format appended.  Any format specifications in format is replaced by the string representation of the corresponding object argument.</returns>
    /// <exception cref="ArgumentNullException">format is null.</exception>
    /// <exception cref="ArgumentOutOfRangeException">Enlarging the value of this instance would exceed <see cref="StringBuilder.MaxCapacity"/>.</exception>
    /// <exception cref="FormatException">format is invalid.</exception>
    public static StringBuilder AppendFormatLine(this StringBuilder builder, string format, params object[] args)
    {
      builder.AppendFormat(CultureInfo.CurrentCulture, format, args);
      return builder.AppendLine();
    }

    /// <summary>
    /// Appends a formatted string and the default line terminator, which contains zero or more format specifications, to this instance.  Each format specification is replaced by the string representation of a corresponding object argument.
    /// </summary>
    /// <param name="builder">The <see cref="StringBuilder"/> instance to append text to.</param>
    /// <param name="provider">A <see cref="IFormatProvider"/> that determines the way formatting specifications in format are interpreted.</param>
    /// <param name="format">A string containing zero or more format specifications.</param>
    /// <param name="args">An array of objects to format.</param>
    /// <returns>A reference to this instance with format appended.  Any format specifications in format is replaced by the string representation of the corresponding object argument.</returns>
    /// <exception cref="ArgumentNullException">format is null.</exception>
    /// <exception cref="ArgumentOutOfRangeException">Enlarging the value of this instance would exceed <see cref="StringBuilder.MaxCapacity"/>.</exception>
    /// <exception cref="FormatException">format is invalid.</exception>
    public static StringBuilder AppendFormatLine(this StringBuilder builder, IFormatProvider provider, string format, params object[] args)
    {
      builder.AppendFormat(provider, format, args);
      return builder.AppendLine();
    }

    /// <summary>
    /// Appends a formatted string and the default line terminator, which contains zero or more format specifications, to this instance.  Each format specification is replaced by the string representation of a corresponding object argument.
    /// </summary>
    /// <param name="builder">The <see cref="StringBuilder"/> instance to append text to.</param>
    /// <param name="format">A string containing zero or more format specifications.</param>
    /// <param name="arg0">The first object to format.</param>
    /// <param name="arg1">The second object to format.</param>
    /// <returns>A reference to this instance with format appended.  Any format specifications in format is replaced by the string representation of the corresponding object argument.</returns>
    /// <exception cref="ArgumentNullException">format is null.</exception>
    /// <exception cref="ArgumentOutOfRangeException">Enlarging the value of this instance would exceed <see cref="StringBuilder.MaxCapacity"/>.</exception>
    /// <exception cref="FormatException">format is invalid.</exception>
    public static StringBuilder AppendFormatLine(this StringBuilder builder, string format, object arg0, object arg1)
    {
      builder.AppendFormat(format, arg0, arg1);
      return builder.AppendLine();
    }

    /// <summary>
    /// Appends a formatted string and the default line terminator, which contains zero or more format specifications, to this instance.  Each format specification is replaced by the string representation of a corresponding object argument.
    /// </summary>
    /// <param name="builder">The <see cref="StringBuilder"/> instance to append text to.</param>
    /// <param name="format">A string containing zero or more format specifications.</param>
    /// <param name="arg0">The first object to format.</param>
    /// <param name="arg1">The second object to format.</param>
    /// <param name="arg2">The third object to format.</param>
    /// <returns>A reference to this instance with format appended.  Any format specifications in format is replaced by the string representation of the corresponding object argument.</returns>
    /// <exception cref="ArgumentNullException">format is null.</exception>
    /// <exception cref="ArgumentOutOfRangeException">Enlarging the value of this instance would exceed <see cref="StringBuilder.MaxCapacity"/>.</exception>
    /// <exception cref="FormatException">format is invalid.</exception>
    public static StringBuilder AppendFormatLine(this StringBuilder builder, string format, object arg0, object arg1, object arg2)
    {
      builder.AppendFormat(format, arg0, arg1, arg2);
      return builder.AppendLine();
    }
  }
}
