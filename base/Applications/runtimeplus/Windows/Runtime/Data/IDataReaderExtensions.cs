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
  using System.Collections;
  using System.Data;
  using System.Data.Common;

  /// <summary>
  /// Extension methods for an <see cref="IDataReader"/>.
  /// </summary>
  public static class IDataReaderExtensions
  {
    #region GetBoolean

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Boolean"/>.
    /// </summary>
    /// <param name="reader">The reader to retrieve the value from.</param>
    /// <param name="name">The name of the field to find.</param>
    /// <returns>The value of the column.</returns>
    public static bool GetBoolean(this IDataRecord reader, string name)
    {
      return reader.GetBoolean(reader.GetOrdinal(name));
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Boolean"/> or null of it the value is <see cref="DBNull"/>.
    /// </summary>
    /// <param name="reader">The reader to retrieve the value from.</param>
    /// <param name="index">The zero-based column ordinal.</param>
    /// <returns>The value of the column.</returns>
    public static bool? GetNullableBoolean(this IDataRecord reader, int index)
    {
      if (reader.IsDBNull(index))
      {
        return null;
      }
      else
      {
        return reader.GetBoolean(index);
      }
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Boolean"/> or null of it the value is <see cref="DBNull"/>.
    /// </summary>
    /// <param name="reader">The reader to retrieve the value from.</param>
    /// <param name="name">The name of the field to find.</param>
    /// <returns>The value of the column.</returns>
    public static bool? GetNullableBoolean(this IDataRecord reader, string name)
    {
      return reader.GetNullableBoolean(reader.GetOrdinal(name));
    }

    #endregion

    #region GetByte

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Byte"/>.
    /// </summary>
    /// <param name="reader">The reader to retrieve the value from.</param>
    /// <param name="name">The name of the field to find.</param>
    /// <returns>The value of the column.</returns>
    public static byte GetByte(this IDataRecord reader, string name)
    {
      return reader.GetByte(reader.GetOrdinal(name));
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Byte"/> or null of it the value is <see cref="DBNull"/>.
    /// </summary>
    /// <param name="reader">The reader to retrieve the value from.</param>
    /// <param name="index">The zero-based column ordinal.</param>
    /// <returns>The value of the column.</returns>
    public static byte? GetNullableByte(this IDataRecord reader, int index)
    {
      if (reader.IsDBNull(index))
      {
        return null;
      }
      else
      {
        return reader.GetByte(index);
      }
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Byte"/> or null of it the value is <see cref="DBNull"/>.
    /// </summary>
    /// <param name="reader">The reader to retrieve the value from.</param>
    /// <param name="name">The name of the field to find.</param>
    /// <returns>The value of the column.</returns>
    public static byte? GetNullableByte(this IDataRecord reader, string name)
    {
      return reader.GetNullableByte(reader.GetOrdinal(name));
    }

    #endregion

    #region GetBytes

    /// <summary>
    /// Reads a stream of bytes from the specified column offset into the buffer an array starting at the given buffer offset.
    /// </summary>
    /// <param name="reader">The reader to retrieve the value from.</param>
    /// <param name="name">The name of the field to find.</param>
    /// <param name="fieldOffset">The index within the field from which to begin the read operation.</param>
    /// <param name="buffer">The buffer into which to read the stream of bytes.</param>
    /// <param name="bufferOffset">The index for <em>buffer</em> to begin the write operation.</param>
    /// <param name="length">The maximum length to copy into the buffer.</param>
    /// <returns>The actual number of bytes read.</returns>
    public static long GetBytes(this IDataRecord reader, string name, long fieldOffset, byte[] buffer, int bufferOffset, int length)
    {
      return reader.GetBytes(reader.GetOrdinal(name), fieldOffset, buffer, bufferOffset, length);
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Byte"/> array.
    /// </summary>
    /// <param name="reader">The reader to retrieve the value from.</param>
    /// <param name="index">The zero-based column ordinal.</param>
    /// <returns>The value of the column.</returns>
    public static byte[] GetAllBytes(this IDataRecord reader, int index)
    {
      long size = reader.GetBytes(index, 0, null, 0, 0);
      if (size > int.MaxValue)
      {
        throw new InvalidOperationException();
      }

      byte[] buf = new byte[size];
      reader.GetBytes(index, 0, buf, 0, (int)size);

      return buf;
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Byte"/> array or null of it the value is <see cref="DBNull"/>.
    /// </summary>
    /// <param name="reader">The reader to retrieve the value from.</param>
    /// <param name="name">The name of the field to find.</param>
    /// <returns>The value of the column.</returns>
    public static byte[] GetAllBytes(this IDataRecord reader, string name)
    {
      return reader.GetAllBytes(reader.GetOrdinal(name));
    }

    /// <summary>
    /// Reads a stream of bytes from the specified column offset into the buffer an array starting at the given buffer offset.
    /// </summary>
    /// <param name="reader">The reader to retrieve the value from.</param>
    /// <param name="index">The zero-based column ordinal.</param>
    /// <param name="fieldOffset">The index within the field from which to begin the read operation.</param>
    /// <param name="buffer">The buffer into which to read the stream of bytes.</param>
    /// <param name="bufferOffset">The index for <em>buffer</em> to begin the write operation.</param>
    /// <param name="length">The maximum length to copy into the buffer.</param>
    /// <returns>The actual number of bytes read.</returns>
    public static long GetNullableBytes(this IDataRecord reader, int index, long fieldOffset, byte[] buffer, int bufferOffset, int length)
    {
      if (reader.IsDBNull(index))
      {
        return 0;
      }
      else
      {
        return reader.GetBytes(index, fieldOffset, buffer, bufferOffset, length);
      }
    }

    /// <summary>
    /// Reads a stream of bytes from the specified column offset into the buffer an array starting at the given buffer offset.
    /// </summary>
    /// <param name="reader">The reader to retrieve the value from.</param>
    /// <param name="name">The name of the field to find.</param>
    /// <param name="fieldOffset">The index within the field from which to begin the read operation.</param>
    /// <param name="buffer">The buffer into which to read the stream of bytes.</param>
    /// <param name="bufferOffset">The index for <em>buffer</em> to begin the write operation.</param>
    /// <param name="length">The maximum length to copy into the buffer.</param>
    /// <returns>The actual number of bytes read.</returns>
    public static long GetNullableBytes(this IDataRecord reader, string name, long fieldOffset, byte[] buffer, int bufferOffset, int length)
    {
      return reader.GetNullableBytes(reader.GetOrdinal(name), fieldOffset, buffer, bufferOffset, length);
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Byte"/> array or null of it the value is <see cref="DBNull"/>.
    /// </summary>
    /// <param name="reader">The reader to retrieve the value from.</param>
    /// <param name="index">The zero-based column ordinal.</param>
    /// <returns>The value of the column.</returns>
    public static byte[] GetNullableAllBytes(this IDataRecord reader, int index)
    {
      if (reader.IsDBNull(index))
      {
        return null;
      }

      long size = reader.GetBytes(index, 0, null, 0, 0);
      if (size > int.MaxValue)
      {
        throw new InvalidOperationException();
      }

      byte[] buf = new byte[size];
      reader.GetBytes(index, 0, buf, 0, (int)size);

      return buf;
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Byte"/> array or null of it the value is <see cref="DBNull"/>.
    /// </summary>
    /// <param name="reader">The reader to retrieve the value from.</param>
    /// <param name="name">The name of the field to find.</param>
    /// <returns>The value of the column.</returns>
    public static byte[] GetNullableAllBytes(this IDataRecord reader, string name)
    {
      return reader.GetNullableAllBytes(reader.GetOrdinal(name));
    }

    #endregion

    #region GetChar

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Char"/>.
    /// </summary>
    /// <param name="reader">The reader to retrieve the value from.</param>
    /// <param name="name">The name of the field to find.</param>
    /// <returns>The value of the column.</returns>
    public static char GetChar(this IDataRecord reader, string name)
    {
      return reader.GetChar(reader.GetOrdinal(name));
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Char"/> or null of it the value is <see cref="DBNull"/>.
    /// </summary>
    /// <param name="reader">The reader to retrieve the value from.</param>
    /// <param name="index">The zero-based column ordinal.</param>
    /// <returns>The value of the column.</returns>
    public static char? GetNullableChar(this IDataRecord reader, int index)
    {
      if (reader.IsDBNull(index))
      {
        return null;
      }
      else
      {
        return reader.GetChar(index);
      }
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Char"/> or null of it the value is <see cref="DBNull"/>.
    /// </summary>
    /// <param name="reader">The reader to retrieve the value from.</param>
    /// <param name="name">The name of the field to find.</param>
    /// <returns>The value of the column.</returns>
    public static char? GetNullableChar(this IDataRecord reader, string name)
    {
      return reader.GetNullableChar(reader.GetOrdinal(name));
    }

    #endregion

    #region GetChars

    /// <summary>
    /// Reads a stream of chars from the specified column offset into the buffer an array starting at the given buffer offset.
    /// </summary>
    /// <param name="reader">The reader to retrieve the value from.</param>
    /// <param name="name">The name of the field to find.</param>
    /// <param name="fieldOffset">The index within the field from which to begin the read operation.</param>
    /// <param name="buffer">The buffer into which to read the stream of chars.</param>
    /// <param name="bufferOffset">The index for <em>buffer</em> to begin the write operation.</param>
    /// <param name="length">The maximum length to copy into the buffer.</param>
    /// <returns>The actual number of chars read.</returns>
    public static long GetChars(this IDataRecord reader, string name, long fieldOffset, char[] buffer, int bufferOffset, int length)
    {
      return reader.GetChars(reader.GetOrdinal(name), fieldOffset, buffer, bufferOffset, length);
    }

    /// <summary>
    /// Reads a stream of chars from the specified column offset into the buffer an array starting at the given buffer offset.
    /// </summary>
    /// <param name="reader">The reader to retrieve the value from.</param>
    /// <param name="index">The zero-based column ordinal.</param>
    /// <param name="fieldOffset">The index within the field from which to begin the read operation.</param>
    /// <param name="buffer">The buffer into which to read the stream of chars.</param>
    /// <param name="bufferOffset">The index for <em>buffer</em> to begin the write operation.</param>
    /// <param name="length">The maximum length to copy into the buffer.</param>
    /// <returns>The actual number of chars read.</returns>
    public static long GetNullableChars(this IDataRecord reader, int index, long fieldOffset, char[] buffer, int bufferOffset, int length)
    {
      if (reader.IsDBNull(index))
      {
        return 0;
      }
      else
      {
        return reader.GetChars(index, fieldOffset, buffer, bufferOffset, length);
      }
    }

    /// <summary>
    /// Reads a stream of chars from the specified column offset into the buffer an array starting at the given buffer offset.
    /// </summary>
    /// <param name="reader">The reader to retrieve the value from.</param>
    /// <param name="name">The name of the field to find.</param>
    /// <param name="fieldOffset">The index within the field from which to begin the read operation.</param>
    /// <param name="buffer">The buffer into which to read the stream of chars.</param>
    /// <param name="bufferOffset">The index for <em>buffer</em> to begin the write operation.</param>
    /// <param name="length">The maximum length to copy into the buffer.</param>
    /// <returns>The actual number of chars read.</returns>
    public static long GetNullableChars(this IDataRecord reader, string name, long fieldOffset, char[] buffer, int bufferOffset, int length)
    {
      return reader.GetNullableChars(reader.GetOrdinal(name), fieldOffset, buffer, bufferOffset, length);
    }

    #endregion

    #region GetData

    /// <summary>
    /// Gets an <see cref="IDataReader"/> to be used when the field points to more remote structured data.
    /// </summary>
    /// <param name="reader">The reader to retrieve the value from.</param>
    /// <param name="name">The name of the field to find.</param>
    /// <returns>An <see cref="IDataReader"/> to be used when the field points to more remote structured data.</returns>
    public static IDataReader GetData(this IDataRecord reader, string name)
    {
      return reader.GetData(reader.GetOrdinal(name));
    }

    #endregion

    #region GetDataTypeName

    /// <summary>
    /// Gets the name of the source data type.
    /// </summary>
    /// <param name="reader">The reader to retrieve the value from.</param>
    /// <param name="name">The name of the field to find.</param>
    /// <returns>The name of the back-end data type.</returns>
    public static string GetDataTypeName(this IDataRecord reader, string name)
    {
      return reader.GetDataTypeName(reader.GetOrdinal(name));
    }

    #endregion

    #region GetDateTime

    /// <summary>
    /// Gets the value of the specified column as a <see cref="DateTime"/>.
    /// </summary>
    /// <param name="reader">The reader to retrieve the value from.</param>
    /// <param name="name">The name of the field to find.</param>
    /// <returns>The value of the column.</returns>
    public static DateTime GetDateTime(this IDataRecord reader, string name)
    {
      return reader.GetDateTime(reader.GetOrdinal(name));
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="DateTime"/> or null of it the value is <see cref="DBNull"/>.
    /// </summary>
    /// <param name="reader">The reader to retrieve the value from.</param>
    /// <param name="index">The zero-based column ordinal.</param>
    /// <returns>The value of the column.</returns>
    public static DateTime? GetNullableDateTime(this IDataRecord reader, int index)
    {
      if (reader.IsDBNull(index))
      {
        return null;
      }
      else
      {
        return reader.GetDateTime(index);
      }
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="DateTime"/> or null of it the value is <see cref="DBNull"/>.
    /// </summary>
    /// <param name="reader">The reader to retrieve the value from.</param>
    /// <param name="name">The name of the field to find.</param>
    /// <returns>The value of the column.</returns>
    public static DateTime? GetNullableDateTime(this IDataRecord reader, string name)
    {
      return reader.GetNullableDateTime(reader.GetOrdinal(name));
    }

    #endregion

    #region GetDecimal

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Decimal"/>.
    /// </summary>
    /// <param name="reader">The reader to retrieve the value from.</param>
    /// <param name="name">The name of the field to find.</param>
    /// <returns>The value of the column.</returns>
    public static decimal GetDecimal(this IDataRecord reader, string name)
    {
      return reader.GetDecimal(reader.GetOrdinal(name));
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Decimal"/> or null of it the value is <see cref="DBNull"/>.
    /// </summary>
    /// <param name="reader">The reader to retrieve the value from.</param>
    /// <param name="index">The zero-based column ordinal.</param>
    /// <returns>The value of the column.</returns>
    public static decimal? GetNullableDecimal(this IDataRecord reader, int index)
    {
      if (reader.IsDBNull(index))
      {
        return null;
      }
      else
      {
        return reader.GetDecimal(index);
      }
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Decimal"/> or null of it the value is <see cref="DBNull"/>.
    /// </summary>
    /// <param name="reader">The reader to retrieve the value from.</param>
    /// <param name="name">The name of the field to find.</param>
    /// <returns>The value of the column.</returns>
    public static decimal? GetNullableDecimal(this IDataRecord reader, string name)
    {
      return reader.GetNullableDecimal(reader.GetOrdinal(name));
    }

    #endregion

    #region GetDouble

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Double"/>.
    /// </summary>
    /// <param name="reader">The reader to retrieve the value from.</param>
    /// <param name="name">The name of the field to find.</param>
    /// <returns>The value of the column.</returns>
    public static double GetDouble(this IDataRecord reader, string name)
    {
      return reader.GetDouble(reader.GetOrdinal(name));
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Double"/> or null of it the value is <see cref="DBNull"/>.
    /// </summary>
    /// <param name="reader">The reader to retrieve the value from.</param>
    /// <param name="index">The zero-based column ordinal.</param>
    /// <returns>The value of the column.</returns>
    public static double? GetNullableDouble(this IDataRecord reader, int index)
    {
      if (reader.IsDBNull(index))
      {
        return null;
      }
      else
      {
        return reader.GetDouble(index);
      }
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Double"/> or null of it the value is <see cref="DBNull"/>.
    /// </summary>
    /// <param name="reader">The reader to retrieve the value from.</param>
    /// <param name="name">The name of the field to find.</param>
    /// <returns>The value of the column.</returns>
    public static double? GetNullableDouble(this IDataRecord reader, string name)
    {
      return reader.GetNullableDouble(reader.GetOrdinal(name));
    }

    #endregion

    #region GetFieldType

    /// <summary>
    /// Gets the <see cref="Type"/> that is the data type of the object.
    /// </summary>
    /// <param name="reader">The reader to retrieve the value from.</param>
    /// <param name="name">The name of the field to find.</param>
    /// <returns>The <see cref="Type"/> that is the data type of the object.  If the type does not exist on the client, in the case of a User-Defined Type (UDT) returned from the database, returns null.</returns>
    public static Type GetFieldType(this IDataRecord reader, string name)
    {
      return reader.GetFieldType(reader.GetOrdinal(name));
    }

    #endregion

    #region GetFloat

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Single"/>.
    /// </summary>
    /// <param name="reader">The reader to retrieve the value from.</param>
    /// <param name="name">The name of the field to find.</param>
    /// <returns>The value of the column.</returns>
    [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1720:IdentifiersShouldNotContainTypeNames", MessageId = "float", Justification = "Named to be consistent with method we're overloading.")]
    public static float GetFloat(this IDataRecord reader, string name)
    {
      return reader.GetFloat(reader.GetOrdinal(name));
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Single"/> or null of it the value is <see cref="DBNull"/>.
    /// </summary>
    /// <param name="reader">The reader to retrieve the value from.</param>
    /// <param name="index">The zero-based column ordinal.</param>
    /// <returns>The value of the column.</returns>
    public static float? GetNullableFloat(this IDataRecord reader, int index)
    {
      if (reader.IsDBNull(index))
      {
        return null;
      }
      else
      {
        return reader.GetFloat(index);
      }
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Single"/> or null of it the value is <see cref="DBNull"/>.
    /// </summary>
    /// <param name="reader">The reader to retrieve the value from.</param>
    /// <param name="name">The name of the field to find.</param>
    /// <returns>The value of the column.</returns>
    public static float? GetNullableFloat(this IDataRecord reader, string name)
    {
      return reader.GetNullableFloat(reader.GetOrdinal(name));
    }

    #endregion

    #region GetGuid

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Guid"/>.
    /// </summary>
    /// <param name="reader">The reader to retrieve the value from.</param>
    /// <param name="name">The name of the field to find.</param>
    /// <returns>The value of the column.</returns>
    public static Guid GetGuid(this IDataRecord reader, string name)
    {
      return reader.GetGuid(reader.GetOrdinal(name));
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Guid"/> or null of it the value is <see cref="DBNull"/>.
    /// </summary>
    /// <param name="reader">The reader to retrieve the value from.</param>
    /// <param name="index">The zero-based column ordinal.</param>
    /// <returns>The value of the column.</returns>
    public static Guid? GetNullableGuid(this IDataRecord reader, int index)
    {
      if (reader.IsDBNull(index))
      {
        return null;
      }
      else
      {
        return reader.GetGuid(index);
      }
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Guid"/> or null of it the value is <see cref="DBNull"/>.
    /// </summary>
    /// <param name="reader">The reader to retrieve the value from.</param>
    /// <param name="name">The name of the field to find.</param>
    /// <returns>The value of the column.</returns>
    public static Guid? GetNullableGuid(this IDataRecord reader, string name)
    {
      return reader.GetNullableGuid(reader.GetOrdinal(name));
    }

    #endregion

    #region GetInt16

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Int16"/>.
    /// </summary>
    /// <param name="reader">The reader to retrieve the value from.</param>
    /// <param name="name">The name of the field to find.</param>
    /// <returns>The value of the column.</returns>
    public static short GetInt16(this IDataRecord reader, string name)
    {
      return reader.GetInt16(reader.GetOrdinal(name));
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Int16"/> or null of it the value is <see cref="DBNull"/>.
    /// </summary>
    /// <param name="reader">The reader to retrieve the value from.</param>
    /// <param name="index">The zero-based column ordinal.</param>
    /// <returns>The value of the column.</returns>
    public static short? GetNullableInt16(this IDataRecord reader, int index)
    {
      if (reader.IsDBNull(index))
      {
        return null;
      }
      else
      {
        return reader.GetInt16(index);
      }
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Int16"/> or null of it the value is <see cref="DBNull"/>.
    /// </summary>
    /// <param name="reader">The reader to retrieve the value from.</param>
    /// <param name="name">The name of the field to find.</param>
    /// <returns>The value of the column.</returns>
    public static short? GetNullableInt16(this IDataRecord reader, string name)
    {
      return reader.GetNullableInt16(reader.GetOrdinal(name));
    }

    #endregion

    #region GetInt32

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Int32"/>.
    /// </summary>
    /// <param name="reader">The reader to retrieve the value from.</param>
    /// <param name="name">The name of the field to find.</param>
    /// <returns>The value of the column.</returns>
    public static int GetInt32(this IDataRecord reader, string name)
    {
      return reader.GetInt32(reader.GetOrdinal(name));
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Int32"/> or null of it the value is <see cref="DBNull"/>.
    /// </summary>
    /// <param name="reader">The reader to retrieve the value from.</param>
    /// <param name="index">The zero-based column ordinal.</param>
    /// <returns>The value of the column.</returns>
    public static int? GetNullableInt32(this IDataRecord reader, int index)
    {
      if (reader.IsDBNull(index))
      {
        return null;
      }
      else
      {
        return reader.GetInt32(index);
      }
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Int32"/> or null of it the value is <see cref="DBNull"/>.
    /// </summary>
    /// <param name="reader">The reader to retrieve the value from.</param>
    /// <param name="name">The name of the field to find.</param>
    /// <returns>The value of the column.</returns>
    public static int? GetNullableInt32(this IDataRecord reader, string name)
    {
      return reader.GetNullableInt32(reader.GetOrdinal(name));
    }

    #endregion

    #region GetInt64

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Int64"/>.
    /// </summary>
    /// <param name="reader">The reader to retrieve the value from.</param>
    /// <param name="name">The name of the field to find.</param>
    /// <returns>The value of the column.</returns>
    public static long GetInt64(this IDataRecord reader, string name)
    {
      return reader.GetInt64(reader.GetOrdinal(name));
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Int64"/> or null of it the value is <see cref="DBNull"/>.
    /// </summary>
    /// <param name="reader">The reader to retrieve the value from.</param>
    /// <param name="index">The zero-based column ordinal.</param>
    /// <returns>The value of the column.</returns>
    public static long? GetNullableInt64(this IDataRecord reader, int index)
    {
      if (reader.IsDBNull(index))
      {
        return null;
      }
      else
      {
        return reader.GetInt64(index);
      }
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="Int64"/> or null of it the value is <see cref="DBNull"/>.
    /// </summary>
    /// <param name="reader">The reader to retrieve the value from.</param>
    /// <param name="name">The name of the field to find.</param>
    /// <returns>The value of the column.</returns>
    public static long? GetNullableInt64(this IDataRecord reader, string name)
    {
      return reader.GetNullableInt64(reader.GetOrdinal(name));
    }

    #endregion

    #region GetString

    /// <summary>
    /// Gets the value of the specified column as a <see cref="String"/>.
    /// </summary>
    /// <param name="reader">The reader to retrieve the value from.</param>
    /// <param name="name">The name of the field to find.</param>
    /// <returns>The value of the column.</returns>
    public static string GetString(this IDataRecord reader, string name)
    {
      return reader.GetString(reader.GetOrdinal(name));
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="String"/> or null of it the value is <see cref="DBNull"/>.
    /// </summary>
    /// <param name="reader">The reader to retrieve the value from.</param>
    /// <param name="index">The zero-based column ordinal.</param>
    /// <returns>The value of the column.</returns>
    public static string GetNullableString(this IDataRecord reader, int index)
    {
      if (reader.IsDBNull(index))
      {
        return null;
      }
      else
      {
        return reader.GetString(index);
      }
    }

    /// <summary>
    /// Gets the value of the specified column as a <see cref="String"/> or null of it the value is <see cref="DBNull"/>.
    /// </summary>
    /// <param name="reader">The reader to retrieve the value from.</param>
    /// <param name="name">The name of the field to find.</param>
    /// <returns>The value of the column.</returns>
    public static string GetNullableString(this IDataRecord reader, string name)
    {
      return reader.GetNullableString(reader.GetOrdinal(name));
    }

    #endregion

    #region GetValue

    /// <summary>
    /// Gets the value of the specified column in its native format.
    /// </summary>
    /// <param name="reader">The reader to retrieve the value from.</param>
    /// <param name="name">The name of the field to find.</param>
    /// <returns>The value of the column.</returns>
    public static object GetValue(this IDataRecord reader, string name)
    {
      return reader.GetValue(reader.GetOrdinal(name));
    }

    /// <summary>
    /// Gets the value of the specified column in its native format or null of it the value is <see cref="DBNull"/>.
    /// </summary>
    /// <param name="reader">The reader to retrieve the value from.</param>
    /// <param name="index">The zero-based column ordinal.</param>
    /// <returns>The value of the column.</returns>
    public static object GetNullableValue(this IDataRecord reader, int index)
    {
      if (reader.IsDBNull(index))
      {
        return null;
      }
      else
      {
        return reader.GetValue(index);
      }
    }

    /// <summary>
    /// Gets the value of the specified column in its native format or null of it the value is <see cref="DBNull"/>.
    /// </summary>
    /// <param name="reader">The reader to retrieve the value from.</param>
    /// <param name="name">The name of the field to find.</param>
    /// <returns>The value of the column.</returns>
    public static object GetNullableValue(this IDataRecord reader, string name)
    {
      return reader.GetNullableValue(reader.GetOrdinal(name));
    }

    #endregion

    #region IsDBNull

    /// <summary>
    /// Gets a value that indicates whether the column contains non-existent or missing values.
    /// </summary>
    /// <param name="reader">The reader to retrieve the value from.</param>
    /// <param name="name">The name of the field to find.</param>
    /// <returns><b>True</b> if the specified column value is equivalent to <see cref="DBNull"/>; otherwise false.</returns>
    public static bool IsDBNull(this IDataRecord reader, string name)
    {
      return reader.IsDBNull(reader.GetOrdinal(name));
    }

    #endregion
  }
}
