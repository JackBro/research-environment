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

namespace System.Extensions.Data
{
  using System;
  using System.Data;

  /// <summary>
  /// Extension methods for a <see cref="DataRow"/>.
  /// </summary>
  public static class DataRowExtensions
  {
    #region GetBoolean

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Boolean"/>.
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="column">The column to retrieve.</param>
    /// <returns>The value of the column.</returns>
    public static bool GetBoolean(this DataRow row, DataColumn column)
    {
      return (bool)row[column];
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Boolean"/>.
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="columnIndex">The zero-based index of the column.</param>
    /// <returns>The value of the column.</returns>
    public static bool GetBoolean(this DataRow row, int columnIndex)
    {
      return (bool)row[columnIndex];
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Boolean"/>.
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="columnName">The name of the column to retrieve.</param>
    /// <returns>The value of the column.</returns>
    public static bool GetBoolean(this DataRow row, string columnName)
    {
      return (bool)row[columnName];
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Boolean"/>.
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="column">The column to retrieve.</param>
    /// <param name="version">The version of the row you want.</param>
    /// <returns>The value of the column.</returns>
    public static bool GetBoolean(this DataRow row, DataColumn column, DataRowVersion version)
    {
      return (bool)row[column, version];
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Boolean"/>.
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="columnIndex">The zero-based index of the column.</param>
    /// <param name="version">The version of the row you want.</param>
    /// <returns>The value of the column.</returns>
    public static bool GetBoolean(this DataRow row, int columnIndex, DataRowVersion version)
    {
      return (bool)row[columnIndex, version];
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Boolean"/>.
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="columnName">The name of the column to retrieve.</param>
    /// <param name="version">The version of the row you want.</param>
    /// <returns>The value of the column.</returns>
    public static bool GetBoolean(this DataRow row, string columnName, DataRowVersion version)
    {
      return (bool)row[columnName, version];
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Boolean"/> or null if the value is <see cref="DBNull"/>..
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="column">The column to retrieve.</param>
    /// <returns>The value of the column.</returns>
    public static bool? GetNullableBoolean(this DataRow row, DataColumn column)
    {
      if (row.IsNull(column))
      {
        return null;
      }
      else
      {
        return (bool)row[column];
      }
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Boolean"/> or null if the value is <see cref="DBNull"/>..
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="columnIndex">The zero-based index of the column.</param>
    /// <returns>The value of the column.</returns>
    public static bool? GetNullableBoolean(this DataRow row, int columnIndex)
    {
      if (row.IsNull(columnIndex))
      {
        return null;
      }
      else
      {
        return (bool)row[columnIndex];
      }
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Boolean"/> or null if the value is <see cref="DBNull"/>..
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="columnName">The name of the column to retrieve.</param>
    /// <returns>The value of the column.</returns>
    public static bool? GetNullableBoolean(this DataRow row, string columnName)
    {
      if (row.IsNull(columnName))
      {
        return null;
      }
      else
      {
        return (bool)row[columnName];
      }
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Boolean"/> or null if the value is <see cref="DBNull"/>..
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="column">The column to retrieve.</param>
    /// <param name="version">The version of the row you want.</param>
    /// <returns>The value of the column.</returns>
    public static bool? GetNullableBoolean(this DataRow row, DataColumn column, DataRowVersion version)
    {
      if (row.IsNull(column, version))
      {
        return null;
      }
      else
      {
        return (bool)row[column, version];
      }
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Boolean"/> or null if the value is <see cref="DBNull"/>..
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="columnIndex">The zero-based index of the column.</param>
    /// <param name="version">The version of the row you want.</param>
    /// <returns>The value of the column.</returns>
    public static bool? GetNullableBoolean(this DataRow row, int columnIndex, DataRowVersion version)
    {
      if (row.IsNull(row.Table.Columns[columnIndex], version))
      {
        return null;
      }
      else
      {
        return (bool)row[columnIndex, version];
      }
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Boolean"/> or null if the value is <see cref="DBNull"/>..
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="columnName">The name of the column to retrieve.</param>
    /// <param name="version">The version of the row you want.</param>
    /// <returns>The value of the column.</returns>
    public static bool? GetNullableBoolean(this DataRow row, string columnName, DataRowVersion version)
    {
      if (row.IsNull(row.Table.Columns[columnName], version))
      {
        return null;
      }
      else
      {
        return (bool)row[columnName, version];
      }
    }

    #endregion

    #region GetByte

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Byte"/>.
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="column">The column to retrieve.</param>
    /// <returns>The value of the column.</returns>
    public static byte GetByte(this DataRow row, DataColumn column)
    {
      return (byte)row[column];
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Byte"/>.
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="columnIndex">The zero-based index of the column.</param>
    /// <returns>The value of the column.</returns>
    public static byte GetByte(this DataRow row, int columnIndex)
    {
      return (byte)row[columnIndex];
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Byte"/>.
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="columnName">The name of the column to retrieve.</param>
    /// <returns>The value of the column.</returns>
    public static byte GetByte(this DataRow row, string columnName)
    {
      return (byte)row[columnName];
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Byte"/>.
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="column">The column to retrieve.</param>
    /// <param name="version">The version of the row you want.</param>
    /// <returns>The value of the column.</returns>
    public static byte GetByte(this DataRow row, DataColumn column, DataRowVersion version)
    {
      return (byte)row[column, version];
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Byte"/>.
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="columnIndex">The zero-based index of the column.</param>
    /// <param name="version">The version of the row you want.</param>
    /// <returns>The value of the column.</returns>
    public static byte GetByte(this DataRow row, int columnIndex, DataRowVersion version)
    {
      return (byte)row[columnIndex, version];
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Byte"/>.
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="columnName">The name of the column to retrieve.</param>
    /// <param name="version">The version of the row you want.</param>
    /// <returns>The value of the column.</returns>
    public static byte GetByte(this DataRow row, string columnName, DataRowVersion version)
    {
      return (byte)row[columnName, version];
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Byte"/> or null if the value is <see cref="DBNull"/>..
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="column">The column to retrieve.</param>
    /// <returns>The value of the column.</returns>
    public static byte? GetNullableByte(this DataRow row, DataColumn column)
    {
      if (row.IsNull(column))
      {
        return null;
      }
      else
      {
        return (byte)row[column];
      }
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Byte"/> or null if the value is <see cref="DBNull"/>..
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="columnIndex">The zero-based index of the column.</param>
    /// <returns>The value of the column.</returns>
    public static byte? GetNullableByte(this DataRow row, int columnIndex)
    {
      if (row.IsNull(columnIndex))
      {
        return null;
      }
      else
      {
        return (byte)row[columnIndex];
      }
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Byte"/> or null if the value is <see cref="DBNull"/>..
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="columnName">The name of the column to retrieve.</param>
    /// <returns>The value of the column.</returns>
    public static byte? GetNullableByte(this DataRow row, string columnName)
    {
      if (row.IsNull(columnName))
      {
        return null;
      }
      else
      {
        return (byte)row[columnName];
      }
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Byte"/> or null if the value is <see cref="DBNull"/>..
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="column">The column to retrieve.</param>
    /// <param name="version">The version of the row you want.</param>
    /// <returns>The value of the column.</returns>
    public static byte? GetNullableByte(this DataRow row, DataColumn column, DataRowVersion version)
    {
      if (row.IsNull(column, version))
      {
        return null;
      }
      else
      {
        return (byte)row[column, version];
      }
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Byte"/> or null if the value is <see cref="DBNull"/>..
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="columnIndex">The zero-based index of the column.</param>
    /// <param name="version">The version of the row you want.</param>
    /// <returns>The value of the column.</returns>
    public static byte? GetNullableByte(this DataRow row, int columnIndex, DataRowVersion version)
    {
      if (row.IsNull(row.Table.Columns[columnIndex], version))
      {
        return null;
      }
      else
      {
        return (byte)row[columnIndex, version];
      }
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Byte"/> or null if the value is <see cref="DBNull"/>..
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="columnName">The name of the column to retrieve.</param>
    /// <param name="version">The version of the row you want.</param>
    /// <returns>The value of the column.</returns>
    public static byte? GetNullableByte(this DataRow row, string columnName, DataRowVersion version)
    {
      if (row.IsNull(row.Table.Columns[columnName], version))
      {
        return null;
      }
      else
      {
        return (byte)row[columnName, version];
      }
    }

    #endregion

    #region GetBytes

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Byte"/> array.
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="column">The column to retrieve.</param>
    /// <returns>The value of the column.</returns>
    public static byte[] GetBytes(this DataRow row, DataColumn column)
    {
      return (byte[])row[column];
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Byte"/> array.
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="columnIndex">The zero-based index of the column.</param>
    /// <returns>The value of the column.</returns>
    public static byte[] GetBytes(this DataRow row, int columnIndex)
    {
      return (byte[])row[columnIndex];
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Byte"/> array.
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="columnName">The name of the column to retrieve.</param>
    /// <returns>The value of the column.</returns>
    public static byte[] GetBytes(this DataRow row, string columnName)
    {
      return (byte[])row[columnName];
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Byte"/> array.
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="column">The column to retrieve.</param>
    /// <param name="version">The version of the row you want.</param>
    /// <returns>The value of the column.</returns>
    public static byte[] GetBytes(this DataRow row, DataColumn column, DataRowVersion version)
    {
      return (byte[])row[column, version];
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Byte"/> array.
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="columnIndex">The zero-based index of the column.</param>
    /// <param name="version">The version of the row you want.</param>
    /// <returns>The value of the column.</returns>
    public static byte[] GetBytes(this DataRow row, int columnIndex, DataRowVersion version)
    {
      return (byte[])row[columnIndex, version];
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Byte"/> array.
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="columnName">The name of the column to retrieve.</param>
    /// <param name="version">The version of the row you want.</param>
    /// <returns>The value of the column.</returns>
    public static byte[] GetBytes(this DataRow row, string columnName, DataRowVersion version)
    {
      return (byte[])row[columnName, version];
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Byte"/> array or null if the value is <see cref="DBNull"/>..
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="column">The column to retrieve.</param>
    /// <returns>The value of the column.</returns>
    public static byte[] GetNullableBytes(this DataRow row, DataColumn column)
    {
      if (row.IsNull(column))
      {
        return null;
      }
      else
      {
        return (byte[])row[column];
      }
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Byte"/> array or null if the value is <see cref="DBNull"/>..
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="columnIndex">The zero-based index of the column.</param>
    /// <returns>The value of the column.</returns>
    public static byte[] GetNullableBytes(this DataRow row, int columnIndex)
    {
      if (row.IsNull(columnIndex))
      {
        return null;
      }
      else
      {
        return (byte[])row[columnIndex];
      }
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Byte"/> array or null if the value is <see cref="DBNull"/>..
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="columnName">The name of the column to retrieve.</param>
    /// <returns>The value of the column.</returns>
    public static byte[] GetNullableBytes(this DataRow row, string columnName)
    {
      if (row.IsNull(columnName))
      {
        return null;
      }
      else
      {
        return (byte[])row[columnName];
      }
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Byte"/> array or null if the value is <see cref="DBNull"/>..
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="column">The column to retrieve.</param>
    /// <param name="version">The version of the row you want.</param>
    /// <returns>The value of the column.</returns>
    public static byte[] GetNullableBytes(this DataRow row, DataColumn column, DataRowVersion version)
    {
      if (row.IsNull(column, version))
      {
        return null;
      }
      else
      {
        return (byte[])row[column, version];
      }
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Byte"/> array or null if the value is <see cref="DBNull"/>..
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="columnIndex">The zero-based index of the column.</param>
    /// <param name="version">The version of the row you want.</param>
    /// <returns>The value of the column.</returns>
    public static byte[] GetNullableBytes(this DataRow row, int columnIndex, DataRowVersion version)
    {
      if (row.IsNull(row.Table.Columns[columnIndex], version))
      {
        return null;
      }
      else
      {
        return (byte[])row[columnIndex, version];
      }
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Byte"/> array or null if the value is <see cref="DBNull"/>..
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="columnName">The name of the column to retrieve.</param>
    /// <param name="version">The version of the row you want.</param>
    /// <returns>The value of the column.</returns>
    public static byte[] GetNullableBytes(this DataRow row, string columnName, DataRowVersion version)
    {
      if (row.IsNull(row.Table.Columns[columnName], version))
      {
        return null;
      }
      else
      {
        return (byte[])row[columnName, version];
      }
    }

    #endregion

    #region GetChar

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Char"/>.
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="column">The column to retrieve.</param>
    /// <returns>The value of the column.</returns>
    public static char GetChar(this DataRow row, DataColumn column)
    {
      return (char)row[column];
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Char"/>.
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="columnIndex">The zero-based index of the column.</param>
    /// <returns>The value of the column.</returns>
    public static char GetChar(this DataRow row, int columnIndex)
    {
      return (char)row[columnIndex];
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Char"/>.
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="columnName">The name of the column to retrieve.</param>
    /// <returns>The value of the column.</returns>
    public static char GetChar(this DataRow row, string columnName)
    {
      return (char)row[columnName];
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Char"/>.
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="column">The column to retrieve.</param>
    /// <param name="version">The version of the row you want.</param>
    /// <returns>The value of the column.</returns>
    public static char GetChar(this DataRow row, DataColumn column, DataRowVersion version)
    {
      return (char)row[column, version];
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Char"/>.
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="columnIndex">The zero-based index of the column.</param>
    /// <param name="version">The version of the row you want.</param>
    /// <returns>The value of the column.</returns>
    public static char GetChar(this DataRow row, int columnIndex, DataRowVersion version)
    {
      return (char)row[columnIndex, version];
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Char"/>.
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="columnName">The name of the column to retrieve.</param>
    /// <param name="version">The version of the row you want.</param>
    /// <returns>The value of the column.</returns>
    public static char GetChar(this DataRow row, string columnName, DataRowVersion version)
    {
      return (char)row[columnName, version];
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Char"/> or null if the value is <see cref="DBNull"/>..
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="column">The column to retrieve.</param>
    /// <returns>The value of the column.</returns>
    public static char? GetNullableChar(this DataRow row, DataColumn column)
    {
      if (row.IsNull(column))
      {
        return null;
      }
      else
      {
        return (char)row[column];
      }
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Char"/> or null if the value is <see cref="DBNull"/>..
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="columnIndex">The zero-based index of the column.</param>
    /// <returns>The value of the column.</returns>
    public static char? GetNullableChar(this DataRow row, int columnIndex)
    {
      if (row.IsNull(columnIndex))
      {
        return null;
      }
      else
      {
        return (char)row[columnIndex];
      }
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Char"/> or null if the value is <see cref="DBNull"/>..
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="columnName">The name of the column to retrieve.</param>
    /// <returns>The value of the column.</returns>
    public static char? GetNullableChar(this DataRow row, string columnName)
    {
      if (row.IsNull(columnName))
      {
        return null;
      }
      else
      {
        return (char)row[columnName];
      }
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Char"/> or null if the value is <see cref="DBNull"/>..
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="column">The column to retrieve.</param>
    /// <param name="version">The version of the row you want.</param>
    /// <returns>The value of the column.</returns>
    public static char? GetNullableChar(this DataRow row, DataColumn column, DataRowVersion version)
    {
      if (row.IsNull(column, version))
      {
        return null;
      }
      else
      {
        return (char)row[column, version];
      }
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Char"/> or null if the value is <see cref="DBNull"/>..
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="columnIndex">The zero-based index of the column.</param>
    /// <param name="version">The version of the row you want.</param>
    /// <returns>The value of the column.</returns>
    public static char? GetNullableChar(this DataRow row, int columnIndex, DataRowVersion version)
    {
      if (row.IsNull(row.Table.Columns[columnIndex], version))
      {
        return null;
      }
      else
      {
        return (char)row[columnIndex, version];
      }
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Char"/> or null if the value is <see cref="DBNull"/>..
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="columnName">The name of the column to retrieve.</param>
    /// <param name="version">The version of the row you want.</param>
    /// <returns>The value of the column.</returns>
    public static char? GetNullableChar(this DataRow row, string columnName, DataRowVersion version)
    {
      if (row.IsNull(row.Table.Columns[columnName], version))
      {
        return null;
      }
      else
      {
        return (char)row[columnName, version];
      }
    }

    #endregion

    #region GetChars

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Char"/> array.
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="column">The column to retrieve.</param>
    /// <returns>The value of the column.</returns>
    public static char[] GetChars(this DataRow row, DataColumn column)
    {
      return (char[])row[column];
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Char"/> array.
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="columnIndex">The zero-based index of the column.</param>
    /// <returns>The value of the column.</returns>
    public static char[] GetChars(this DataRow row, int columnIndex)
    {
      return (char[])row[columnIndex];
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Char"/> array.
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="columnName">The name of the column to retrieve.</param>
    /// <returns>The value of the column.</returns>
    public static char[] GetChars(this DataRow row, string columnName)
    {
      return (char[])row[columnName];
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Char"/> array.
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="column">The column to retrieve.</param>
    /// <param name="version">The version of the row you want.</param>
    /// <returns>The value of the column.</returns>
    public static char[] GetChars(this DataRow row, DataColumn column, DataRowVersion version)
    {
      return (char[])row[column, version];
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Char"/> array.
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="columnIndex">The zero-based index of the column.</param>
    /// <param name="version">The version of the row you want.</param>
    /// <returns>The value of the column.</returns>
    public static char[] GetChars(this DataRow row, int columnIndex, DataRowVersion version)
    {
      return (char[])row[columnIndex, version];
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Char"/> array.
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="columnName">The name of the column to retrieve.</param>
    /// <param name="version">The version of the row you want.</param>
    /// <returns>The value of the column.</returns>
    public static char[] GetChars(this DataRow row, string columnName, DataRowVersion version)
    {
      return (char[])row[columnName, version];
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Char"/> array or null if the value is <see cref="DBNull"/>..
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="column">The column to retrieve.</param>
    /// <returns>The value of the column.</returns>
    public static char[] GetNullableChars(this DataRow row, DataColumn column)
    {
      if (row.IsNull(column))
      {
        return null;
      }
      else
      {
        return (char[])row[column];
      }
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Char"/> array or null if the value is <see cref="DBNull"/>..
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="columnIndex">The zero-based index of the column.</param>
    /// <returns>The value of the column.</returns>
    public static char[] GetNullableChars(this DataRow row, int columnIndex)
    {
      if (row.IsNull(columnIndex))
      {
        return null;
      }
      else
      {
        return (char[])row[columnIndex];
      }
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Char"/> array or null if the value is <see cref="DBNull"/>..
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="columnName">The name of the column to retrieve.</param>
    /// <returns>The value of the column.</returns>
    public static char[] GetNullableChars(this DataRow row, string columnName)
    {
      if (row.IsNull(columnName))
      {
        return null;
      }
      else
      {
        return (char[])row[columnName];
      }
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Char"/> array or null if the value is <see cref="DBNull"/>..
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="column">The column to retrieve.</param>
    /// <param name="version">The version of the row you want.</param>
    /// <returns>The value of the column.</returns>
    public static char[] GetNullableChars(this DataRow row, DataColumn column, DataRowVersion version)
    {
      if (row.IsNull(column, version))
      {
        return null;
      }
      else
      {
        return (char[])row[column, version];
      }
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Char"/> array or null if the value is <see cref="DBNull"/>..
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="columnIndex">The zero-based index of the column.</param>
    /// <param name="version">The version of the row you want.</param>
    /// <returns>The value of the column.</returns>
    public static char[] GetNullableChars(this DataRow row, int columnIndex, DataRowVersion version)
    {
      if (row.IsNull(row.Table.Columns[columnIndex], version))
      {
        return null;
      }
      else
      {
        return (char[])row[columnIndex, version];
      }
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Char"/> array or null if the value is <see cref="DBNull"/>..
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="columnName">The name of the column to retrieve.</param>
    /// <param name="version">The version of the row you want.</param>
    /// <returns>The value of the column.</returns>
    public static char[] GetNullableChars(this DataRow row, string columnName, DataRowVersion version)
    {
      if (row.IsNull(row.Table.Columns[columnName], version))
      {
        return null;
      }
      else
      {
        return (char[])row[columnName, version];
      }
    }

    #endregion

    #region GetDateTime

    /// <summary>
    /// Gets the value of the specified column as a <see cref="DateTime"/>.
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="column">The column to retrieve.</param>
    /// <returns>The value of the column.</returns>
    public static DateTime GetDateTime(this DataRow row, DataColumn column)
    {
      return (DateTime)row[column];
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="DateTime"/>.
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="columnIndex">The zero-based index of the column.</param>
    /// <returns>The value of the column.</returns>
    public static DateTime GetDateTime(this DataRow row, int columnIndex)
    {
      return (DateTime)row[columnIndex];
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="DateTime"/>.
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="columnName">The name of the column to retrieve.</param>
    /// <returns>The value of the column.</returns>
    public static DateTime GetDateTime(this DataRow row, string columnName)
    {
      return (DateTime)row[columnName];
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="DateTime"/>.
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="column">The column to retrieve.</param>
    /// <param name="version">The version of the row you want.</param>
    /// <returns>The value of the column.</returns>
    public static DateTime GetDateTime(this DataRow row, DataColumn column, DataRowVersion version)
    {
      return (DateTime)row[column, version];
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="DateTime"/>.
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="columnIndex">The zero-based index of the column.</param>
    /// <param name="version">The version of the row you want.</param>
    /// <returns>The value of the column.</returns>
    public static DateTime GetDateTime(this DataRow row, int columnIndex, DataRowVersion version)
    {
      return (DateTime)row[columnIndex, version];
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="DateTime"/>.
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="columnName">The name of the column to retrieve.</param>
    /// <param name="version">The version of the row you want.</param>
    /// <returns>The value of the column.</returns>
    public static DateTime GetDateTime(this DataRow row, string columnName, DataRowVersion version)
    {
      return (DateTime)row[columnName, version];
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="DateTime"/> or null if the value is <see cref="DBNull"/>..
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="column">The column to retrieve.</param>
    /// <returns>The value of the column.</returns>
    public static DateTime? GetNullableDateTime(this DataRow row, DataColumn column)
    {
      if (row.IsNull(column))
      {
        return null;
      }
      else
      {
        return (DateTime)row[column];
      }
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="DateTime"/> or null if the value is <see cref="DBNull"/>..
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="columnIndex">The zero-based index of the column.</param>
    /// <returns>The value of the column.</returns>
    public static DateTime? GetNullableDateTime(this DataRow row, int columnIndex)
    {
      if (row.IsNull(columnIndex))
      {
        return null;
      }
      else
      {
        return (DateTime)row[columnIndex];
      }
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="DateTime"/> or null if the value is <see cref="DBNull"/>..
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="columnName">The name of the column to retrieve.</param>
    /// <returns>The value of the column.</returns>
    public static DateTime? GetNullableDateTime(this DataRow row, string columnName)
    {
      if (row.IsNull(columnName))
      {
        return null;
      }
      else
      {
        return (DateTime)row[columnName];
      }
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="DateTime"/> or null if the value is <see cref="DBNull"/>..
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="column">The column to retrieve.</param>
    /// <param name="version">The version of the row you want.</param>
    /// <returns>The value of the column.</returns>
    public static DateTime? GetNullableDateTime(this DataRow row, DataColumn column, DataRowVersion version)
    {
      if (row.IsNull(column, version))
      {
        return null;
      }
      else
      {
        return (DateTime)row[column, version];
      }
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="DateTime"/> or null if the value is <see cref="DBNull"/>..
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="columnIndex">The zero-based index of the column.</param>
    /// <param name="version">The version of the row you want.</param>
    /// <returns>The value of the column.</returns>
    public static DateTime? GetNullableDateTime(this DataRow row, int columnIndex, DataRowVersion version)
    {
      if (row.IsNull(row.Table.Columns[columnIndex], version))
      {
        return null;
      }
      else
      {
        return (DateTime)row[columnIndex, version];
      }
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="DateTime"/> or null if the value is <see cref="DBNull"/>..
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="columnName">The name of the column to retrieve.</param>
    /// <param name="version">The version of the row you want.</param>
    /// <returns>The value of the column.</returns>
    public static DateTime? GetNullableDateTime(this DataRow row, string columnName, DataRowVersion version)
    {
      if (row.IsNull(row.Table.Columns[columnName], version))
      {
        return null;
      }
      else
      {
        return (DateTime)row[columnName, version];
      }
    }

    #endregion

    #region GetDecimal

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Decimal"/>.
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="column">The column to retrieve.</param>
    /// <returns>The value of the column.</returns>
    public static decimal GetDecimal(this DataRow row, DataColumn column)
    {
      return (decimal)row[column];
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Decimal"/>.
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="columnIndex">The zero-based index of the column.</param>
    /// <returns>The value of the column.</returns>
    public static decimal GetDecimal(this DataRow row, int columnIndex)
    {
      return (decimal)row[columnIndex];
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Decimal"/>.
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="columnName">The name of the column to retrieve.</param>
    /// <returns>The value of the column.</returns>
    public static decimal GetDecimal(this DataRow row, string columnName)
    {
      return (decimal)row[columnName];
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Decimal"/>.
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="column">The column to retrieve.</param>
    /// <param name="version">The version of the row you want.</param>
    /// <returns>The value of the column.</returns>
    public static decimal GetDecimal(this DataRow row, DataColumn column, DataRowVersion version)
    {
      return (decimal)row[column, version];
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Decimal"/>.
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="columnIndex">The zero-based index of the column.</param>
    /// <param name="version">The version of the row you want.</param>
    /// <returns>The value of the column.</returns>
    public static decimal GetDecimal(this DataRow row, int columnIndex, DataRowVersion version)
    {
      return (decimal)row[columnIndex, version];
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Decimal"/>.
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="columnName">The name of the column to retrieve.</param>
    /// <param name="version">The version of the row you want.</param>
    /// <returns>The value of the column.</returns>
    public static decimal GetDecimal(this DataRow row, string columnName, DataRowVersion version)
    {
      return (decimal)row[columnName, version];
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Decimal"/> or null if the value is <see cref="DBNull"/>..
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="column">The column to retrieve.</param>
    /// <returns>The value of the column.</returns>
    public static decimal? GetNullableDecimal(this DataRow row, DataColumn column)
    {
      if (row.IsNull(column))
      {
        return null;
      }
      else
      {
        return (decimal)row[column];
      }
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Decimal"/> or null if the value is <see cref="DBNull"/>..
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="columnIndex">The zero-based index of the column.</param>
    /// <returns>The value of the column.</returns>
    public static decimal? GetNullableDecimal(this DataRow row, int columnIndex)
    {
      if (row.IsNull(columnIndex))
      {
        return null;
      }
      else
      {
        return (decimal)row[columnIndex];
      }
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Decimal"/> or null if the value is <see cref="DBNull"/>..
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="columnName">The name of the column to retrieve.</param>
    /// <returns>The value of the column.</returns>
    public static decimal? GetNullableDecimal(this DataRow row, string columnName)
    {
      if (row.IsNull(columnName))
      {
        return null;
      }
      else
      {
        return (decimal)row[columnName];
      }
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Decimal"/> or null if the value is <see cref="DBNull"/>..
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="column">The column to retrieve.</param>
    /// <param name="version">The version of the row you want.</param>
    /// <returns>The value of the column.</returns>
    public static decimal? GetNullableDecimal(this DataRow row, DataColumn column, DataRowVersion version)
    {
      if (row.IsNull(column, version))
      {
        return null;
      }
      else
      {
        return (decimal)row[column, version];
      }
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Decimal"/> or null if the value is <see cref="DBNull"/>..
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="columnIndex">The zero-based index of the column.</param>
    /// <param name="version">The version of the row you want.</param>
    /// <returns>The value of the column.</returns>
    public static decimal? GetNullableDecimal(this DataRow row, int columnIndex, DataRowVersion version)
    {
      if (row.IsNull(row.Table.Columns[columnIndex], version))
      {
        return null;
      }
      else
      {
        return (decimal)row[columnIndex, version];
      }
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Decimal"/> or null if the value is <see cref="DBNull"/>..
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="columnName">The name of the column to retrieve.</param>
    /// <param name="version">The version of the row you want.</param>
    /// <returns>The value of the column.</returns>
    public static decimal? GetNullableDecimal(this DataRow row, string columnName, DataRowVersion version)
    {
      if (row.IsNull(row.Table.Columns[columnName], version))
      {
        return null;
      }
      else
      {
        return (decimal)row[columnName, version];
      }
    }

    #endregion

    #region GetDouble

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Double"/>.
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="column">The column to retrieve.</param>
    /// <returns>The value of the column.</returns>
    public static double GetDouble(this DataRow row, DataColumn column)
    {
      return (double)row[column];
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Double"/>.
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="columnIndex">The zero-based index of the column.</param>
    /// <returns>The value of the column.</returns>
    public static double GetDouble(this DataRow row, int columnIndex)
    {
      return (double)row[columnIndex];
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Double"/>.
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="columnName">The name of the column to retrieve.</param>
    /// <returns>The value of the column.</returns>
    public static double GetDouble(this DataRow row, string columnName)
    {
      return (double)row[columnName];
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Double"/>.
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="column">The column to retrieve.</param>
    /// <param name="version">The version of the row you want.</param>
    /// <returns>The value of the column.</returns>
    public static double GetDouble(this DataRow row, DataColumn column, DataRowVersion version)
    {
      return (double)row[column, version];
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Double"/>.
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="columnIndex">The zero-based index of the column.</param>
    /// <param name="version">The version of the row you want.</param>
    /// <returns>The value of the column.</returns>
    public static double GetDouble(this DataRow row, int columnIndex, DataRowVersion version)
    {
      return (double)row[columnIndex, version];
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Double"/>.
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="columnName">The name of the column to retrieve.</param>
    /// <param name="version">The version of the row you want.</param>
    /// <returns>The value of the column.</returns>
    public static double GetDouble(this DataRow row, string columnName, DataRowVersion version)
    {
      return (double)row[columnName, version];
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Double"/> or null if the value is <see cref="DBNull"/>..
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="column">The column to retrieve.</param>
    /// <returns>The value of the column.</returns>
    public static double? GetNullableDouble(this DataRow row, DataColumn column)
    {
      if (row.IsNull(column))
      {
        return null;
      }
      else
      {
        return (double)row[column];
      }
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Double"/> or null if the value is <see cref="DBNull"/>..
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="columnIndex">The zero-based index of the column.</param>
    /// <returns>The value of the column.</returns>
    public static double? GetNullableDouble(this DataRow row, int columnIndex)
    {
      if (row.IsNull(columnIndex))
      {
        return null;
      }
      else
      {
        return (double)row[columnIndex];
      }
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Double"/> or null if the value is <see cref="DBNull"/>..
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="columnName">The name of the column to retrieve.</param>
    /// <returns>The value of the column.</returns>
    public static double? GetNullableDouble(this DataRow row, string columnName)
    {
      if (row.IsNull(columnName))
      {
        return null;
      }
      else
      {
        return (double)row[columnName];
      }
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Double"/> or null if the value is <see cref="DBNull"/>..
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="column">The column to retrieve.</param>
    /// <param name="version">The version of the row you want.</param>
    /// <returns>The value of the column.</returns>
    public static double? GetNullableDouble(this DataRow row, DataColumn column, DataRowVersion version)
    {
      if (row.IsNull(column, version))
      {
        return null;
      }
      else
      {
        return (double)row[column, version];
      }
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Double"/> or null if the value is <see cref="DBNull"/>..
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="columnIndex">The zero-based index of the column.</param>
    /// <param name="version">The version of the row you want.</param>
    /// <returns>The value of the column.</returns>
    public static double? GetNullableDouble(this DataRow row, int columnIndex, DataRowVersion version)
    {
      if (row.IsNull(row.Table.Columns[columnIndex], version))
      {
        return null;
      }
      else
      {
        return (double)row[columnIndex, version];
      }
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Double"/> or null if the value is <see cref="DBNull"/>..
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="columnName">The name of the column to retrieve.</param>
    /// <param name="version">The version of the row you want.</param>
    /// <returns>The value of the column.</returns>
    public static double? GetNullableDouble(this DataRow row, string columnName, DataRowVersion version)
    {
      if (row.IsNull(row.Table.Columns[columnName], version))
      {
        return null;
      }
      else
      {
        return (double)row[columnName, version];
      }
    }

    #endregion

    #region GetGuid

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Guid"/>.
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="column">The column to retrieve.</param>
    /// <returns>The value of the column.</returns>
    public static Guid GetGuid(this DataRow row, DataColumn column)
    {
      return (Guid)row[column];
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Guid"/>.
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="columnIndex">The zero-based index of the column.</param>
    /// <returns>The value of the column.</returns>
    public static Guid GetGuid(this DataRow row, int columnIndex)
    {
      return (Guid)row[columnIndex];
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Guid"/>.
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="columnName">The name of the column to retrieve.</param>
    /// <returns>The value of the column.</returns>
    public static Guid GetGuid(this DataRow row, string columnName)
    {
      return (Guid)row[columnName];
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Guid"/>.
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="column">The column to retrieve.</param>
    /// <param name="version">The version of the row you want.</param>
    /// <returns>The value of the column.</returns>
    public static Guid GetGuid(this DataRow row, DataColumn column, DataRowVersion version)
    {
      return (Guid)row[column, version];
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Guid"/>.
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="columnIndex">The zero-based index of the column.</param>
    /// <param name="version">The version of the row you want.</param>
    /// <returns>The value of the column.</returns>
    public static Guid GetGuid(this DataRow row, int columnIndex, DataRowVersion version)
    {
      return (Guid)row[columnIndex, version];
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Guid"/>.
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="columnName">The name of the column to retrieve.</param>
    /// <param name="version">The version of the row you want.</param>
    /// <returns>The value of the column.</returns>
    public static Guid GetGuid(this DataRow row, string columnName, DataRowVersion version)
    {
      return (Guid)row[columnName, version];
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Guid"/> or null if the value is <see cref="DBNull"/>..
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="column">The column to retrieve.</param>
    /// <returns>The value of the column.</returns>
    public static Guid? GetNullableGuid(this DataRow row, DataColumn column)
    {
      if (row.IsNull(column))
      {
        return null;
      }
      else
      {
        return (Guid)row[column];
      }
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Guid"/> or null if the value is <see cref="DBNull"/>..
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="columnIndex">The zero-based index of the column.</param>
    /// <returns>The value of the column.</returns>
    public static Guid? GetNullableGuid(this DataRow row, int columnIndex)
    {
      if (row.IsNull(columnIndex))
      {
        return null;
      }
      else
      {
        return (Guid)row[columnIndex];
      }
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Guid"/> or null if the value is <see cref="DBNull"/>..
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="columnName">The name of the column to retrieve.</param>
    /// <returns>The value of the column.</returns>
    public static Guid? GetNullableGuid(this DataRow row, string columnName)
    {
      if (row.IsNull(columnName))
      {
        return null;
      }
      else
      {
        return (Guid)row[columnName];
      }
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Guid"/> or null if the value is <see cref="DBNull"/>..
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="column">The column to retrieve.</param>
    /// <param name="version">The version of the row you want.</param>
    /// <returns>The value of the column.</returns>
    public static Guid? GetNullableGuid(this DataRow row, DataColumn column, DataRowVersion version)
    {
      if (row.IsNull(column, version))
      {
        return null;
      }
      else
      {
        return (Guid)row[column, version];
      }
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Guid"/> or null if the value is <see cref="DBNull"/>..
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="columnIndex">The zero-based index of the column.</param>
    /// <param name="version">The version of the row you want.</param>
    /// <returns>The value of the column.</returns>
    public static Guid? GetNullableGuid(this DataRow row, int columnIndex, DataRowVersion version)
    {
      if (row.IsNull(row.Table.Columns[columnIndex], version))
      {
        return null;
      }
      else
      {
        return (Guid)row[columnIndex, version];
      }
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Guid"/> or null if the value is <see cref="DBNull"/>..
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="columnName">The name of the column to retrieve.</param>
    /// <param name="version">The version of the row you want.</param>
    /// <returns>The value of the column.</returns>
    public static Guid? GetNullableGuid(this DataRow row, string columnName, DataRowVersion version)
    {
      if (row.IsNull(row.Table.Columns[columnName], version))
      {
        return null;
      }
      else
      {
        return (Guid)row[columnName, version];
      }
    }

    #endregion

    #region GetInt16

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Int16"/>.
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="column">The column to retrieve.</param>
    /// <returns>The value of the column.</returns>
    public static short GetInt16(this DataRow row, DataColumn column)
    {
      return (short)row[column];
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Int16"/>.
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="columnIndex">The zero-based index of the column.</param>
    /// <returns>The value of the column.</returns>
    public static short GetInt16(this DataRow row, int columnIndex)
    {
      return (short)row[columnIndex];
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Int16"/>.
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="columnName">The name of the column to retrieve.</param>
    /// <returns>The value of the column.</returns>
    public static short GetInt16(this DataRow row, string columnName)
    {
      return (short)row[columnName];
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Int16"/>.
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="column">The column to retrieve.</param>
    /// <param name="version">The version of the row you want.</param>
    /// <returns>The value of the column.</returns>
    public static short GetInt16(this DataRow row, DataColumn column, DataRowVersion version)
    {
      return (short)row[column, version];
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Int16"/>.
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="columnIndex">The zero-based index of the column.</param>
    /// <param name="version">The version of the row you want.</param>
    /// <returns>The value of the column.</returns>
    public static short GetInt16(this DataRow row, int columnIndex, DataRowVersion version)
    {
      return (short)row[columnIndex, version];
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Int16"/>.
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="columnName">The name of the column to retrieve.</param>
    /// <param name="version">The version of the row you want.</param>
    /// <returns>The value of the column.</returns>
    public static short GetInt16(this DataRow row, string columnName, DataRowVersion version)
    {
      return (short)row[columnName, version];
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Int16"/> or null if the value is <see cref="DBNull"/>..
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="column">The column to retrieve.</param>
    /// <returns>The value of the column.</returns>
    public static short? GetNullableInt16(this DataRow row, DataColumn column)
    {
      if (row.IsNull(column))
      {
        return null;
      }
      else
      {
        return (short)row[column];
      }
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Int16"/> or null if the value is <see cref="DBNull"/>..
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="columnIndex">The zero-based index of the column.</param>
    /// <returns>The value of the column.</returns>
    public static short? GetNullableInt16(this DataRow row, int columnIndex)
    {
      if (row.IsNull(columnIndex))
      {
        return null;
      }
      else
      {
        return (short)row[columnIndex];
      }
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Int16"/> or null if the value is <see cref="DBNull"/>..
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="columnName">The name of the column to retrieve.</param>
    /// <returns>The value of the column.</returns>
    public static short? GetNullableInt16(this DataRow row, string columnName)
    {
      if (row.IsNull(columnName))
      {
        return null;
      }
      else
      {
        return (short)row[columnName];
      }
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Int16"/> or null if the value is <see cref="DBNull"/>..
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="column">The column to retrieve.</param>
    /// <param name="version">The version of the row you want.</param>
    /// <returns>The value of the column.</returns>
    public static short? GetNullableInt16(this DataRow row, DataColumn column, DataRowVersion version)
    {
      if (row.IsNull(column, version))
      {
        return null;
      }
      else
      {
        return (short)row[column, version];
      }
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Int16"/> or null if the value is <see cref="DBNull"/>..
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="columnIndex">The zero-based index of the column.</param>
    /// <param name="version">The version of the row you want.</param>
    /// <returns>The value of the column.</returns>
    public static short? GetNullableInt16(this DataRow row, int columnIndex, DataRowVersion version)
    {
      if (row.IsNull(row.Table.Columns[columnIndex], version))
      {
        return null;
      }
      else
      {
        return (short)row[columnIndex, version];
      }
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Int16"/> or null if the value is <see cref="DBNull"/>..
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="columnName">The name of the column to retrieve.</param>
    /// <param name="version">The version of the row you want.</param>
    /// <returns>The value of the column.</returns>
    public static short? GetNullableInt16(this DataRow row, string columnName, DataRowVersion version)
    {
      if (row.IsNull(row.Table.Columns[columnName], version))
      {
        return null;
      }
      else
      {
        return (short)row[columnName, version];
      }
    }

    #endregion

    #region GetInt32

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Int32"/>.
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="column">The column to retrieve.</param>
    /// <returns>The value of the column.</returns>
    public static int GetInt32(this DataRow row, DataColumn column)
    {
      return (int)row[column];
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Int32"/>.
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="columnIndex">The zero-based index of the column.</param>
    /// <returns>The value of the column.</returns>
    public static int GetInt32(this DataRow row, int columnIndex)
    {
      return (int)row[columnIndex];
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Int32"/>.
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="columnName">The name of the column to retrieve.</param>
    /// <returns>The value of the column.</returns>
    public static int GetInt32(this DataRow row, string columnName)
    {
      return (int)row[columnName];
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Int32"/>.
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="column">The column to retrieve.</param>
    /// <param name="version">The version of the row you want.</param>
    /// <returns>The value of the column.</returns>
    public static int GetInt32(this DataRow row, DataColumn column, DataRowVersion version)
    {
      return (int)row[column, version];
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Int32"/>.
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="columnIndex">The zero-based index of the column.</param>
    /// <param name="version">The version of the row you want.</param>
    /// <returns>The value of the column.</returns>
    public static int GetInt32(this DataRow row, int columnIndex, DataRowVersion version)
    {
      return (int)row[columnIndex, version];
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Int32"/>.
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="columnName">The name of the column to retrieve.</param>
    /// <param name="version">The version of the row you want.</param>
    /// <returns>The value of the column.</returns>
    public static int GetInt32(this DataRow row, string columnName, DataRowVersion version)
    {
      return (int)row[columnName, version];
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Int32"/> or null if the value is <see cref="DBNull"/>..
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="column">The column to retrieve.</param>
    /// <returns>The value of the column.</returns>
    public static int? GetNullableInt32(this DataRow row, DataColumn column)
    {
      if (row.IsNull(column))
      {
        return null;
      }
      else
      {
        return (int)row[column];
      }
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Int32"/> or null if the value is <see cref="DBNull"/>..
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="columnIndex">The zero-based index of the column.</param>
    /// <returns>The value of the column.</returns>
    public static int? GetNullableInt32(this DataRow row, int columnIndex)
    {
      if (row.IsNull(columnIndex))
      {
        return null;
      }
      else
      {
        return (int)row[columnIndex];
      }
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Int32"/> or null if the value is <see cref="DBNull"/>..
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="columnName">The name of the column to retrieve.</param>
    /// <returns>The value of the column.</returns>
    public static int? GetNullableInt32(this DataRow row, string columnName)
    {
      if (row.IsNull(columnName))
      {
        return null;
      }
      else
      {
        return (int)row[columnName];
      }
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Int32"/> or null if the value is <see cref="DBNull"/>..
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="column">The column to retrieve.</param>
    /// <param name="version">The version of the row you want.</param>
    /// <returns>The value of the column.</returns>
    public static int? GetNullableInt32(this DataRow row, DataColumn column, DataRowVersion version)
    {
      if (row.IsNull(column, version))
      {
        return null;
      }
      else
      {
        return (int)row[column, version];
      }
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Int32"/> or null if the value is <see cref="DBNull"/>..
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="columnIndex">The zero-based index of the column.</param>
    /// <param name="version">The version of the row you want.</param>
    /// <returns>The value of the column.</returns>
    public static int? GetNullableInt32(this DataRow row, int columnIndex, DataRowVersion version)
    {
      if (row.IsNull(row.Table.Columns[columnIndex], version))
      {
        return null;
      }
      else
      {
        return (int)row[columnIndex, version];
      }
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Int32"/> or null if the value is <see cref="DBNull"/>..
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="columnName">The name of the column to retrieve.</param>
    /// <param name="version">The version of the row you want.</param>
    /// <returns>The value of the column.</returns>
    public static int? GetNullableInt32(this DataRow row, string columnName, DataRowVersion version)
    {
      if (row.IsNull(row.Table.Columns[columnName], version))
      {
        return null;
      }
      else
      {
        return (int)row[columnName, version];
      }
    }

    #endregion

    #region GetInt64

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Int64"/>.
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="column">The column to retrieve.</param>
    /// <returns>The value of the column.</returns>
    public static long GetInt64(this DataRow row, DataColumn column)
    {
      return (long)row[column];
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Int64"/>.
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="columnIndex">The zero-based index of the column.</param>
    /// <returns>The value of the column.</returns>
    public static long GetInt64(this DataRow row, int columnIndex)
    {
      return (long)row[columnIndex];
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Int64"/>.
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="columnName">The name of the column to retrieve.</param>
    /// <returns>The value of the column.</returns>
    public static long GetInt64(this DataRow row, string columnName)
    {
      return (long)row[columnName];
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Int64"/>.
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="column">The column to retrieve.</param>
    /// <param name="version">The version of the row you want.</param>
    /// <returns>The value of the column.</returns>
    public static long GetInt64(this DataRow row, DataColumn column, DataRowVersion version)
    {
      return (long)row[column, version];
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Int64"/>.
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="columnIndex">The zero-based index of the column.</param>
    /// <param name="version">The version of the row you want.</param>
    /// <returns>The value of the column.</returns>
    public static long GetInt64(this DataRow row, int columnIndex, DataRowVersion version)
    {
      return (long)row[columnIndex, version];
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Int64"/>.
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="columnName">The name of the column to retrieve.</param>
    /// <param name="version">The version of the row you want.</param>
    /// <returns>The value of the column.</returns>
    public static long GetInt64(this DataRow row, string columnName, DataRowVersion version)
    {
      return (long)row[columnName, version];
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Int64"/> or null if the value is <see cref="DBNull"/>..
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="column">The column to retrieve.</param>
    /// <returns>The value of the column.</returns>
    public static long? GetNullableInt64(this DataRow row, DataColumn column)
    {
      if (row.IsNull(column))
      {
        return null;
      }
      else
      {
        return (long)row[column];
      }
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Int64"/> or null if the value is <see cref="DBNull"/>..
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="columnIndex">The zero-based index of the column.</param>
    /// <returns>The value of the column.</returns>
    public static long? GetNullableInt64(this DataRow row, int columnIndex)
    {
      if (row.IsNull(columnIndex))
      {
        return null;
      }
      else
      {
        return (long)row[columnIndex];
      }
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Int64"/> or null if the value is <see cref="DBNull"/>..
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="columnName">The name of the column to retrieve.</param>
    /// <returns>The value of the column.</returns>
    public static long? GetNullableInt64(this DataRow row, string columnName)
    {
      if (row.IsNull(columnName))
      {
        return null;
      }
      else
      {
        return (long)row[columnName];
      }
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Int64"/> or null if the value is <see cref="DBNull"/>..
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="column">The column to retrieve.</param>
    /// <param name="version">The version of the row you want.</param>
    /// <returns>The value of the column.</returns>
    public static long? GetNullableInt64(this DataRow row, DataColumn column, DataRowVersion version)
    {
      if (row.IsNull(column, version))
      {
        return null;
      }
      else
      {
        return (long)row[column, version];
      }
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Int64"/> or null if the value is <see cref="DBNull"/>..
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="columnIndex">The zero-based index of the column.</param>
    /// <param name="version">The version of the row you want.</param>
    /// <returns>The value of the column.</returns>
    public static long? GetNullableInt64(this DataRow row, int columnIndex, DataRowVersion version)
    {
      if (row.IsNull(row.Table.Columns[columnIndex], version))
      {
        return null;
      }
      else
      {
        return (long)row[columnIndex, version];
      }
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Int64"/> or null if the value is <see cref="DBNull"/>..
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="columnName">The name of the column to retrieve.</param>
    /// <param name="version">The version of the row you want.</param>
    /// <returns>The value of the column.</returns>
    public static long? GetNullableInt64(this DataRow row, string columnName, DataRowVersion version)
    {
      if (row.IsNull(row.Table.Columns[columnName], version))
      {
        return null;
      }
      else
      {
        return (long)row[columnName, version];
      }
    }

    #endregion

    #region GetSingle

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Single"/>.
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="column">The column to retrieve.</param>
    /// <returns>The value of the column.</returns>
    public static float GetSingle(this DataRow row, DataColumn column)
    {
      return (float)row[column];
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Single"/>.
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="columnIndex">The zero-based index of the column.</param>
    /// <returns>The value of the column.</returns>
    public static float GetSingle(this DataRow row, int columnIndex)
    {
      return (float)row[columnIndex];
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Single"/>.
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="columnName">The name of the column to retrieve.</param>
    /// <returns>The value of the column.</returns>
    public static float GetSingle(this DataRow row, string columnName)
    {
      return (float)row[columnName];
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Single"/>.
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="column">The column to retrieve.</param>
    /// <param name="version">The version of the row you want.</param>
    /// <returns>The value of the column.</returns>
    public static float GetSingle(this DataRow row, DataColumn column, DataRowVersion version)
    {
      return (float)row[column, version];
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Single"/>.
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="columnIndex">The zero-based index of the column.</param>
    /// <param name="version">The version of the row you want.</param>
    /// <returns>The value of the column.</returns>
    public static float GetSingle(this DataRow row, int columnIndex, DataRowVersion version)
    {
      return (float)row[columnIndex, version];
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Single"/>.
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="columnName">The name of the column to retrieve.</param>
    /// <param name="version">The version of the row you want.</param>
    /// <returns>The value of the column.</returns>
    public static float GetSingle(this DataRow row, string columnName, DataRowVersion version)
    {
      return (float)row[columnName, version];
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Single"/> or null if the value is <see cref="DBNull"/>..
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="column">The column to retrieve.</param>
    /// <returns>The value of the column.</returns>
    public static float? GetNullableSingle(this DataRow row, DataColumn column)
    {
      if (row.IsNull(column))
      {
        return null;
      }
      else
      {
        return (float)row[column];
      }
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Single"/> or null if the value is <see cref="DBNull"/>..
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="columnIndex">The zero-based index of the column.</param>
    /// <returns>The value of the column.</returns>
    public static float? GetNullableSingle(this DataRow row, int columnIndex)
    {
      if (row.IsNull(columnIndex))
      {
        return null;
      }
      else
      {
        return (float)row[columnIndex];
      }
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Single"/> or null if the value is <see cref="DBNull"/>..
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="columnName">The name of the column to retrieve.</param>
    /// <returns>The value of the column.</returns>
    public static float? GetNullableSingle(this DataRow row, string columnName)
    {
      if (row.IsNull(columnName))
      {
        return null;
      }
      else
      {
        return (float)row[columnName];
      }
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Single"/> or null if the value is <see cref="DBNull"/>..
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="column">The column to retrieve.</param>
    /// <param name="version">The version of the row you want.</param>
    /// <returns>The value of the column.</returns>
    public static float? GetNullableSingle(this DataRow row, DataColumn column, DataRowVersion version)
    {
      if (row.IsNull(column, version))
      {
        return null;
      }
      else
      {
        return (float)row[column, version];
      }
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Single"/> or null if the value is <see cref="DBNull"/>..
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="columnIndex">The zero-based index of the column.</param>
    /// <param name="version">The version of the row you want.</param>
    /// <returns>The value of the column.</returns>
    public static float? GetNullableSingle(this DataRow row, int columnIndex, DataRowVersion version)
    {
      if (row.IsNull(row.Table.Columns[columnIndex], version))
      {
        return null;
      }
      else
      {
        return (float)row[columnIndex, version];
      }
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Single"/> or null if the value is <see cref="DBNull"/>..
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="columnName">The name of the column to retrieve.</param>
    /// <param name="version">The version of the row you want.</param>
    /// <returns>The value of the column.</returns>
    public static float? GetNullableSingle(this DataRow row, string columnName, DataRowVersion version)
    {
      if (row.IsNull(row.Table.Columns[columnName], version))
      {
        return null;
      }
      else
      {
        return (float)row[columnName, version];
      }
    }

    #endregion

    #region GetString

    /// <summary>
    /// Gets the value of the specified column as a <see cref="String"/>.
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="column">The column to retrieve.</param>
    /// <returns>The value of the column.</returns>
    public static string GetString(this DataRow row, DataColumn column)
    {
      return (string)row[column];
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="String"/>.
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="columnIndex">The zero-based index of the column.</param>
    /// <returns>The value of the column.</returns>
    public static string GetString(this DataRow row, int columnIndex)
    {
      return (string)row[columnIndex];
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="String"/>.
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="columnName">The name of the column to retrieve.</param>
    /// <returns>The value of the column.</returns>
    public static string GetString(this DataRow row, string columnName)
    {
      return (string)row[columnName];
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="String"/>.
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="column">The column to retrieve.</param>
    /// <param name="version">The version of the row you want.</param>
    /// <returns>The value of the column.</returns>
    public static string GetString(this DataRow row, DataColumn column, DataRowVersion version)
    {
      return (string)row[column, version];
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="String"/>.
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="columnIndex">The zero-based index of the column.</param>
    /// <param name="version">The version of the row you want.</param>
    /// <returns>The value of the column.</returns>
    public static string GetString(this DataRow row, int columnIndex, DataRowVersion version)
    {
      return (string)row[columnIndex, version];
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="String"/>.
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="columnName">The name of the column to retrieve.</param>
    /// <param name="version">The version of the row you want.</param>
    /// <returns>The value of the column.</returns>
    public static string GetString(this DataRow row, string columnName, DataRowVersion version)
    {
      return (string)row[columnName, version];
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="String"/> or null if the value is <see cref="DBNull"/>..
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="column">The column to retrieve.</param>
    /// <returns>The value of the column.</returns>
    public static string GetNullableString(this DataRow row, DataColumn column)
    {
      if (row.IsNull(column))
      {
        return null;
      }
      else
      {
        return (string)row[column];
      }
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="String"/> or null if the value is <see cref="DBNull"/>..
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="columnIndex">The zero-based index of the column.</param>
    /// <returns>The value of the column.</returns>
    public static string GetNullableString(this DataRow row, int columnIndex)
    {
      if (row.IsNull(columnIndex))
      {
        return null;
      }
      else
      {
        return (string)row[columnIndex];
      }
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="String"/> or null if the value is <see cref="DBNull"/>..
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="columnName">The name of the column to retrieve.</param>
    /// <returns>The value of the column.</returns>
    public static string GetNullableString(this DataRow row, string columnName)
    {
      if (row.IsNull(columnName))
      {
        return null;
      }
      else
      {
        return (string)row[columnName];
      }
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="String"/> or null if the value is <see cref="DBNull"/>..
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="column">The column to retrieve.</param>
    /// <param name="version">The version of the row you want.</param>
    /// <returns>The value of the column.</returns>
    public static string GetNullableString(this DataRow row, DataColumn column, DataRowVersion version)
    {
      if (row.IsNull(column, version))
      {
        return null;
      }
      else
      {
        return (string)row[column, version];
      }
    }

    /// <summary>
    /// {
    /// Gets the value of the specified column as a <see cref="String"/> or null if the value is <see cref="DBNull"/>..
    /// }
    /// </summary>
    /// {
    /// <param name="row">The row to retrieve the value from.</param>
    /// }
    /// <param name="columnIndex">The zero-based index of the column.</param>
    /// {
    /// <param name="version">The version of the row you want.</param>
    /// }
    /// <returns>The value of the column.</returns>
    public static string GetNullableString(this DataRow row, int columnIndex, DataRowVersion version)
    {
      if (row.IsNull(row.Table.Columns[columnIndex], version))
      {
        return null;
      }
      else
      {
        return (string)row[columnIndex, version];
      }
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="String"/> or null if the value is <see cref="DBNull"/>..
    /// </summary>
    /// <param name="row">The row to retrieve the value from.</param>
    /// <param name="columnName">The name of the column to retrieve.</param>
    /// <param name="version">The version of the row you want.</param>
    /// <returns>The value of the column.</returns>
    public static string GetNullableString(this DataRow row, string columnName, DataRowVersion version)
    {
      if (row.IsNull(row.Table.Columns[columnName], version))
      {
        return null;
      }
      else
      {
        return (string)row[columnName, version];
      }
    }

    #endregion
  }
}
