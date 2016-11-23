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
  /// Extension methods for an <see cref="IDbCommand"/>.
  /// </summary>
  [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1709:IdentifiersShouldBeCasedCorrectly", MessageId = "Db", Justification = "Modeling the poorly cased .NET BCL")]
  public static class IDbCommandExtensions
  {
    /// <summary>
    /// Executes the query and returns the first column of the first row in the resultset returned by the query.
    /// </summary>
    /// <param name="command">The command to execute.</param>
    /// <returns>The value of the field.</returns>
    public static bool? ExecuteBoolean(this IDbCommand command)
    {
      object result = command.ExecuteScalar();
      if (result == null || Convert.IsDBNull(result))
      {
        return null;
      }
      else
      {
        return (bool)result;
      }
    }

    /// <summary>
    /// Executes the query and returns the first column of the first row in the resultset returned by the query.
    /// </summary>
    /// <param name="command">The command to execute.</param>
    /// <returns>The value of the field.</returns>
    public static byte? ExecuteByte(this IDbCommand command)
    {
      object result = command.ExecuteScalar();
      if (result == null || Convert.IsDBNull(result))
      {
        return null;
      }
      else
      {
        return (byte)result;
      }
    }

    /// <summary>
    /// Executes the query and returns the first column of the first row in the resultset returned by the query.
    /// </summary>
    /// <param name="command">The command to execute.</param>
    /// <returns>The value of the field.</returns>
    public static byte[] ExecuteBytes(this IDbCommand command)
    {
      object result = command.ExecuteScalar();
      if (result == null || Convert.IsDBNull(result))
      {
        return null;
      }
      else
      {
        return (byte[])result;
      }
    }

    /// <summary>
    /// Executes the query and returns the first column of the first row in the resultset returned by the query.
    /// </summary>
    /// <param name="command">The command to execute.</param>
    /// <returns>The value of the field.</returns>
    public static char? ExecuteChar(this IDbCommand command)
    {
      object result = command.ExecuteScalar();
      if (result == null || Convert.IsDBNull(result))
      {
        return null;
      }
      else
      {
        return (char)result;
      }
    }

    /// <summary>
    /// Executes the query and returns the first column of the first row in the resultset returned by the query.
    /// </summary>
    /// <param name="command">The command to execute.</param>
    /// <returns>The value of the field.</returns>
    public static char[] ExecuteChars(this IDbCommand command)
    {
      object result = command.ExecuteScalar();
      if (result == null || Convert.IsDBNull(result))
      {
        return null;
      }
      else
      {
        return (char[])result;
      }
    }

    /// <summary>
    /// Executes the query and returns the first column of the first row in the resultset returned by the query.
    /// </summary>
    /// <param name="command">The command to execute.</param>
    /// <returns>The value of the field.</returns>
    public static DateTime? ExecuteDateTime(this IDbCommand command)
    {
      object result = command.ExecuteScalar();
      if (result == null || Convert.IsDBNull(result))
      {
        return null;
      }
      else
      {
        return (DateTime)result;
      }
    }

    /// <summary>
    /// Executes the query and returns the first column of the first row in the resultset returned by the query.
    /// </summary>
    /// <param name="command">The command to execute.</param>
    /// <returns>The value of the field.</returns>
    public static decimal? ExecuteDecimal(this IDbCommand command)
    {
      object result = command.ExecuteScalar();
      if (result == null || Convert.IsDBNull(result))
      {
        return null;
      }
      else
      {
        return (decimal)result;
      }
    }

    /// <summary>
    /// Executes the query and returns the first column of the first row in the resultset returned by the query.
    /// </summary>
    /// <param name="command">The command to execute.</param>
    /// <returns>The value of the field.</returns>
    public static double? ExecuteDouble(this IDbCommand command)
    {
      object result = command.ExecuteScalar();
      if (result == null || Convert.IsDBNull(result))
      {
        return null;
      }
      else
      {
        return (double)result;
      }
    }

    /// <summary>
    /// Executes the query and returns the first column of the first row in the resultset returned by the query.
    /// </summary>
    /// <param name="command">The command to execute.</param>
    /// <returns>The value of the field.</returns>
    public static Guid? ExecuteGuid(this IDbCommand command)
    {
      object result = command.ExecuteScalar();
      if (result == null || Convert.IsDBNull(result))
      {
        return null;
      }
      else
      {
        return (Guid)result;
      }
    }

    /// <summary>
    /// Executes the query and returns the first column of the first row in the resultset returned by the query.
    /// </summary>
    /// <param name="command">The command to execute.</param>
    /// <returns>The value of the field.</returns>
    public static short? ExecuteInt16(this IDbCommand command)
    {
      object result = command.ExecuteScalar();
      if (result == null || Convert.IsDBNull(result))
      {
        return null;
      }
      else
      {
        return (short)result;
      }
    }

    /// <summary>
    /// Executes the query and returns the first column of the first row in the resultset returned by the query.
    /// </summary>
    /// <param name="command">The command to execute.</param>
    /// <returns>The value of the field.</returns>
    public static int? ExecuteInt32(this IDbCommand command)
    {
      object result = command.ExecuteScalar();
      if (result == null || Convert.IsDBNull(result))
      {
        return null;
      }
      else
      {
        return (int)result;
      }
    }

    /// <summary>
    /// Executes the query and returns the first column of the first row in the resultset returned by the query.
    /// </summary>
    /// <param name="command">The command to execute.</param>
    /// <returns>The value of the field.</returns>
    public static long? ExecuteInt64(this IDbCommand command)
    {
      object result = command.ExecuteScalar();
      if (result == null || Convert.IsDBNull(result))
      {
        return null;
      }
      else
      {
        return (long)result;
      }
    }

    /// <summary>
    /// Executes the query and returns the first column of the first row in the resultset returned by the query.
    /// </summary>
    /// <param name="command">The command to execute.</param>
    /// <returns>The value of the field.</returns>
    public static float? ExecuteSingle(this IDbCommand command)
    {
      object result = command.ExecuteScalar();
      if (result == null || Convert.IsDBNull(result))
      {
        return null;
      }
      else
      {
        return (float)result;
      }
    }

    /// <summary>
    /// Executes the query and returns the first column of the first row in the resultset returned by the query.
    /// </summary>
    /// <param name="command">The command to execute.</param>
    /// <returns>The value of the field.</returns>
    public static string ExecuteString(this IDbCommand command)
    {
      object result = command.ExecuteScalar();
      if (result == null || Convert.IsDBNull(result))
      {
        return null;
      }
      else
      {
        return (string)result;
      }
    }
  }
}
