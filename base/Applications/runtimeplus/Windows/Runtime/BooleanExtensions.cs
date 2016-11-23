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
  /// <summary>
  /// Extension methods for a <see cref="System.Boolean"/>.
  /// </summary>
  public static class BooleanExtensions
  {
    /// <summary>
    /// Converts the boolean into a string representation using the <see cref="BooleanFormatProvider"/>.
    /// </summary>
    /// <param name="value">The <see cref="System.Boolean"/> to convert.</param>
    /// <param name="format">The format string to use during conversion.</param>
    /// <returns>A string representation of the boolean.</returns>
    public static string ToString(this bool value, string format)
    {
      return string.Format(new BooleanFormatProvider(), "{0:" + format + "}", value);
    }
  }
}
