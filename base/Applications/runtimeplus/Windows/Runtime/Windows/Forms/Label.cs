

namespace System.Extensions.Windows.Forms
{
  using System;
  using System.Collections.Generic;
  using System.ComponentModel;
  using System.Drawing;
  using System.Extensions.Internal;
  using System.Extensions.Windows.Forms.Internal;
  using WF = System.Windows.Forms;
  using System.Globalization;

  /// <summary>
  /// A thin wrapper around the <see cref="WF.Label"/> control providing formattable (string.Format) text.
  /// </summary>
  [ToolboxBitmap(typeof(WF.Label))]
  public class Label : WF.Label
  {
    /// <summary>
    /// Stores the text format string.
    /// </summary>
    private string textFormat = string.Empty;

    /// <summary>
    /// Stores the arguments for formatting.
    /// </summary>
    private List<object> formatArguments = new List<object>();

    /// <summary>
    /// Gets or sets the format string to use when determining the text.
    /// </summary>
    [SRCategory("Category_Appearance")]
    [SRDescription("Label_TextFormat")]
    [DefaultValue("")]
    [Localizable(true)]
    public string TextFormat
    {
      get
      {
        return this.textFormat;
      }

      set
      {
        if (!string.Equals(this.textFormat, value))
        {
          this.textFormat = value;
          this.UpdateText();
        }
      }
    }

    /// <summary>
    /// Gets the format arguments to use when determining the text.
    /// </summary>
    public List<object> FormatArguments
    {
      get { return this.formatArguments; }
    }

    /// <summary>
    /// Formats the text using the <see cref="TextFormat"/> and the supplied arguments.
    /// </summary>
    /// <param name="args">The arguments to use during formatting.</param>
    public void FormatText(params object[] args)
    {
      this.formatArguments.Clear();
      this.formatArguments.AddRange(args);
      this.UpdateText();
    }

    /// <summary>
    /// Updates the text based on the stored text format and arguments.
    /// </summary>
    private void UpdateText()
    {
      try
      {
        string newText = this.textFormat;
        if (this.formatArguments.Count > 0)
        {
          newText = string.Format(CultureInfo.CurrentCulture, this.textFormat, this.formatArguments.ToArray());
        }

        if (!string.Equals(newText, this.Text))
        {
          this.Text = newText;
        }
      }
      catch (FormatException)
      {
        // eat this error
      }
    }
  }
}
