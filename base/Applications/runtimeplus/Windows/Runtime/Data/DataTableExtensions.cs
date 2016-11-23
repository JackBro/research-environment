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
/// 

namespace System.Extensions.Data
{
  using System;
  using System.Data;
  using System.Globalization;
  using System.IO;
  using System.Threading;
  using System.Threading.Tasks;
  using System.Linq;

  /// <summary>
  /// Extension methods for a <see cref="DataTable"/>.
  /// </summary>
  public static class DataTableExtensions
  {
    /// <summary>
    /// Reads CSV data into the <see cref="DataTable"/> using the specified file name.
    /// </summary>
    /// <param name="table">The table to read the CSV data into.</param>
    /// <param name="fileName">The name of the file from which to read the data.</param>
    /// <param name="hasHeader">Whether the CSV document has a header row.</param>
    public static void ReadCsv(this DataTable table, string fileName, bool hasHeader)
    {
      using (StreamReader reader = new StreamReader(fileName))
      {
        ReadCsv(table, reader, hasHeader);
      }
    }

    /// <summary>
    /// Reads CSV data into the <see cref="DataTable"/> using the specified <see cref="Stream"/>.
    /// </summary>
    /// <param name="table">The table to read the CSV data into.</param>
    /// <param name="stream">The stream from which to read the data.</param>
    /// <param name="hasHeader">Whether the CSV document has a header row.</param>
    public static void ReadCsv(this DataTable table, Stream stream, bool hasHeader)
    {
      using (StreamReader reader = new StreamReader(stream))
      {
        ReadCsv(table, reader, hasHeader);
      }
    }

    /// <summary>
    /// Reads CSV data into the <see cref="DataTable"/> using the specified <see cref="TextReader"/>.
    /// </summary>
    /// <param name="table">The table to read the CSV data into.</param>
    /// <param name="reader">The reader from which to read the data.</param>
    /// <param name="hasHeader">Whether the CSV document has a header row.</param>
    public static void ReadCsv(this DataTable table, TextReader reader, bool hasHeader)
    {
      string line = reader.ReadLine();
      if (string.IsNullOrEmpty(line))
      {
        return;
      }

      string[] parts = line.Split(',');

      if (hasHeader)
      {
        foreach(var part in parts.AsParallel())
        {
          table.Columns.Add(part, typeof(string));
        }

        line = reader.ReadLine();
        if (string.IsNullOrEmpty(line))
        {
          return;
        }

        parts = line.Split(',');
      }
      else
      {
        foreach (var part in parts.AsParallel())
        {
          table.Columns.Add().DataType = typeof(string);
        }
      }

      while (true)
      {
        DataRow row = table.NewRow();
        row.ItemArray = parts;
        table.Rows.Add(row);
      }
    }

    /// <summary>
    /// Writes the <see cref="DataTable"/> to a CSV file using the specified file name.
    /// </summary>
    /// <param name="table">The table that contains the data to write.</param>
    /// <param name="fileName">The name of the file where the data should be written.</param>
    /// <param name="hasHeader">Whether the CSV document should have a header row.</param>
    public static void WriteCsv(this DataTable table, string fileName, bool hasHeader)
    {
      using (StreamWriter writer = new StreamWriter(fileName))
      {
        WriteCsv(table, writer, hasHeader);
      }
    }

    /// <summary>
    /// Reads CSV data into the <see cref="DataTable"/> using the specified <see cref="Stream"/>.
    /// </summary>
    /// <param name="table">The table that contains the data to write.</param>
    /// <param name="stream">The stream where the data ahould be written.</param>
    /// <param name="hasHeader">Whether the CSV document should have a header row.</param>
    public static void WriteCsv(this DataTable table, Stream stream, bool hasHeader)
    {
      using (StreamWriter writer = new StreamWriter(stream))
      {
        WriteCsv(table, writer, hasHeader);
      }
    }

    /// <summary>
    /// Reads CSV data into the <see cref="DataTable"/> using the specified <see cref="TextReader"/>.
    /// </summary>
    /// <param name="table">The table that contains the data to write.</param>
    /// <param name="writer">The <see cref="TextWriter"/> where the data ahould be written.</param>
    /// <param name="hasHeader">Whether the CSV document should have a header row.</param>
    public static void WriteCsv(this DataTable table, TextWriter writer, bool hasHeader)
    {
      if (hasHeader)
      {
        for (int index = 0; index < table.Columns.Count; ++index)
        {
          if (index > 0)
          {
            writer.Write(",");
          }

          writer.Write(QuoteText(table.Columns[index].ColumnName));
        }

        writer.WriteLine();
      }

      Parallel.For(0, table.Rows.Count, rowIndex =>
      {
          for (int colIndex = 0; colIndex < table.Columns.Count; ++colIndex)
          {
              if (colIndex > 0)
              {
                  writer.Write(",");
              }

              writer.Write(QuoteText(Convert.ToString(table.Rows[rowIndex][colIndex], CultureInfo.InvariantCulture)));
          }

          writer.WriteLine();
      });
    }

    /// <summary>
    /// Called to quote text so the CSV is well formatted.
    /// </summary>
    /// <param name="value">The value to quote.</param>
    /// <returns>The quoted text.</returns>
    private static string QuoteText(object value)
    {
      if (value == null || Convert.IsDBNull(value))
      {
        return string.Empty;
      }

      string strValue = value.ToString();
      if (strValue.Contains(","))
      {
        return "\"" + strValue + "\"";
      }
      else
      {
        return strValue;
      }
    }
  }
}
