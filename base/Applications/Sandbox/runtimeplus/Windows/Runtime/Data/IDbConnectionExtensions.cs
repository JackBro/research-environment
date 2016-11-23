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
//
// </copyright>
*/
namespace System.Extensions.Data
{
  using System.Data;
  using System.Data.SqlClient;

  /// <summary>
  /// Extension methods for an <see cref="IDbConnection"/>.
  /// </summary>
  [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1709:IdentifiersShouldBeCasedCorrectly", MessageId = "Db")]
  public static class IDbConnectionExtensions
  {
    /// <summary>
    /// Creates and returns a <see cref="IDbCommand"/> object associated with the connection.
    /// </summary>
    /// <param name="connection">The connection to associate the command to.</param>
    /// <param name="commandText">The text of the command.</param>
    /// <returns>An <see cref="IDbCommand"/> object associated with the connection.</returns>
    public static IDbCommand CreateCommand(this IDbConnection connection, string commandText)
    {
      return CreateCommand(connection, commandText, CommandType.Text);
    }

    /// <summary>
    /// Creates and returns a <see cref="IDbCommand"/> object associated with the connection.
    /// </summary>
    /// <param name="connection">The connection to associate the command to.</param>
    /// <param name="commandText">The text of the command.</param>
    /// <param name="commandType">The type of the command.</param>
    /// <returns>An <see cref="IDbCommand"/> object associated with the connection.</returns>
    public static IDbCommand CreateCommand(this IDbConnection connection, string commandText, CommandType commandType)
    {
      var command = connection.CreateCommand();
      command.CommandText = commandText;
      command.CommandType = commandType;
      return command;
    }

    /// <summary>
    /// Creates an appropriate <see cref="IDbCommand"/> and executes a statement against the connection.
    /// </summary>
    /// <param name="connection">The connection to execute the command against.</param>
    /// <param name="commandText">The text of the command to execute.</param>
    /// <returns>The number of rows affected.</returns>
    public static int ExecuteCommandNonQuery(this IDbConnection connection, string commandText)
    {
      using (IDbCommand command = connection.CreateCommand(commandText))
      {
        return command.ExecuteNonQuery();
      }
    }

    /// <summary>
    /// Creates an appropriate <see cref="IDbCommand"/> and executes a statement against the connection.
    /// </summary>
    /// <param name="connection">The connection to execute the command against.</param>
    /// <param name="commandText">The text of the command to execute.</param>
    /// <param name="commandType">The type of the command to execute.</param>
    /// <returns>The number of rows affected.</returns>
    public static int ExecuteCommandNonQuery(this IDbConnection connection, string commandText, CommandType commandType)
    {
      using (IDbCommand command = connection.CreateCommand(commandText, commandType))
      {
        return command.ExecuteNonQuery();
      }
    }
  }
}
