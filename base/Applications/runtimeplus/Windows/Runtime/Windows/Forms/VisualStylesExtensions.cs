
namespace System.Extensions.Windows.Forms
{
  using WF = System.Windows.Forms;

  /// <summary>
  /// Extension methods to expose visual styles on stock Windows Forms controls.
  /// </summary>
  public static class VisualStylesExtensions
  {
    /// <summary>
    /// Sets the cue banner for a specified 
    /// </summary>
    /// <param name="textBox">The <see cref="WF.TextBox"/> control to set.</param>
    /// <param name="text">The cue banner text to show.</param>
    /// <param name="showWhenFocused">Whether to show the cue banner when the control has focus.</param>
    public static void SetCueBanner(this WF.TextBox textBox, string text, bool showWhenFocused)
    {
      TextBox.SetCueBanner(textBox, text, showWhenFocused);
    }
  }
}
