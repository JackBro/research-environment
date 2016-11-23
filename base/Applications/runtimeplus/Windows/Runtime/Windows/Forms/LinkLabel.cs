

namespace System.Extensions.Windows.Forms
{
  using System.Diagnostics;
  using System.Drawing;
  using WF = System.Windows.Forms;

  /// <summary>
  /// A thin wrapper around the <see cref="WF.LinkLabel"/> control providing support for automatic process execution.
  /// </summary>
  [ToolboxBitmap(typeof(WF.LinkLabel))]
  public class LinkLabel : WF.LinkLabel
  {
    /// <summary>
    /// Stores the name of the process to launch.
    /// </summary>
    private string processStart;

    /// <summary>
    /// Initializes a new instance of the <see cref="LinkLabel"/> class.
    /// </summary>
    public LinkLabel()
    {
      this.LinkColor = SystemColors.HotTrack;
    }

    /// <summary>
    /// Gets or sets the string to execute on click.  Can be a file, application, or url.
    /// </summary>
    public string ProcessStart
    {
      get { return this.processStart; }
      set { this.processStart = value; }
    }

    /// <summary>
    /// Intercepts the link click to open the process if specified.
    /// </summary>
    /// <param name="e">A <see cref="WF.LinkLabelLinkClickedEventArgs"/> that contains the event data.</param>
    protected override void OnLinkClicked(WF.LinkLabelLinkClickedEventArgs e)
    {
      if (string.IsNullOrEmpty(this.processStart))
      {
        base.OnLinkClicked(e);
      }
      else
      {
        Process.Start(this.processStart);
      }
    }
  }
}
