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
  /// Extension methods for a <see cref="DateTime"/>.
  /// </summary>
  public static class DateTimeExtensions
  {
    /// <summary>
    /// Indicates whether the specified <see cref="DateTime"/> is on a weekend day.
    /// </summary>
    /// <param name="dateTime">The <see cref="DateTime"/> to check.</param>
    /// <returns><b>True</b> if the day represented is a weekend day; <b>false</b> otherwise.</returns>
    public static bool IsWeekend(this DateTime dateTime)
    {
      return dateTime.DayOfWeek == DayOfWeek.Saturday || dateTime.DayOfWeek == DayOfWeek.Sunday;
    }

    /// <summary>
    /// Indicates whether the specified <see cref="DateTime"/> occurs on a week day.
    /// </summary>
    /// <param name="dateTime">The <see cref="DateTime"/> to check.</param>
    /// <returns><b>True</b> if the day represented is a week day; <b>false</b> otherwise.</returns>
    public static bool IsWeekday(this DateTime dateTime)
    {
      return !IsWeekend(dateTime);
    }
  }
}
