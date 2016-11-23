

namespace System.Extensions.Windows.Forms
{
  using System.Globalization;
  using System.Windows.Forms;

  /// <summary>
  /// A class to help utilize the stock MessageBox class.
  /// </summary>
  public static class MessageBoxHelper
  {
    /// <summary>
    /// Displays a message box.
    /// </summary>
    /// <param name="parent">The control that will own the modal dialog box.</param>
    /// <param name="text">The text to display in the message box.</param>
    /// <returns>One of the <see cref="DialogResult"/> values.</returns>
    public static DialogResult Show(Control parent, string text)
    {
      return MessageBox.Show(parent, text, string.Empty, MessageBoxButtons.OK, MessageBoxIcon.None, MessageBoxDefaultButton.Button1, EnsureRightToLeft(parent));
    }

    /// <summary>
    /// Displays a message box.
    /// </summary>
    /// <param name="parent">The control that will own the modal dialog box.</param>
    /// <param name="text">The text to display in the message box.</param>
    /// <param name="caption">The text to display in the title bar of the message box.</param>
    /// <returns>One of the <see cref="DialogResult"/> values.</returns>
    public static DialogResult Show(Control parent, string text, string caption)
    {
      return MessageBox.Show(parent, text, caption, MessageBoxButtons.OK, MessageBoxIcon.None, MessageBoxDefaultButton.Button1, EnsureRightToLeft(parent));
    }

    /// <summary>
    /// Displays a message box.
    /// </summary>
    /// <param name="parent">The control that will own the modal dialog box.</param>
    /// <param name="text">The text to display in the message box.</param>
    /// <param name="caption">The text to display in the title bar of the message box.</param>
    /// <param name="buttons">One of the <see cref="MessageBoxButtons"/> values that specifies which buttons to display in the message box.</param>
    /// <returns>One of the <see cref="DialogResult"/> values.</returns>
    public static DialogResult Show(Control parent, string text, string caption, MessageBoxButtons buttons)
    {
      return MessageBox.Show(parent, text, caption, buttons, MessageBoxIcon.None, MessageBoxDefaultButton.Button1, EnsureRightToLeft(parent));
    }

    /// <summary>
    /// Displays a message box.
    /// </summary>
    /// <param name="parent">The control that will own the modal dialog box.</param>
    /// <param name="text">The text to display in the message box.</param>
    /// <param name="caption">The text to display in the title bar of the message box.</param>
    /// <param name="buttons">One of the <see cref="MessageBoxButtons"/> values that specifies which buttons to display in the message box.</param>
    /// <param name="icon">One of the <see cref="MessageBoxIcon"/> values that specifies which icon to display in the message box.</param>
    /// <returns>One of the <see cref="DialogResult"/> values.</returns>
    public static DialogResult Show(Control parent, string text, string caption, MessageBoxButtons buttons, MessageBoxIcon icon)
    {
      return MessageBox.Show(parent, text, caption, buttons, icon, MessageBoxDefaultButton.Button1, EnsureRightToLeft(parent));
    }

    /// <summary>
    /// Displays a message box.
    /// </summary>
    /// <param name="parent">The control that will own the modal dialog box.</param>
    /// <param name="text">The text to display in the message box.</param>
    /// <param name="caption">The text to display in the title bar of the message box.</param>
    /// <param name="buttons">One of the <see cref="MessageBoxButtons"/> values that specifies which buttons to display in the message box.</param>
    /// <param name="icon">One of the <see cref="MessageBoxIcon"/> values that specifies which icon to display in the message box.</param>
    /// <param name="defaultButton">One of the <see cref="MessageBoxDefaultButton"/> values that specifies the default button for the message box.</param>
    /// <returns>One of the <see cref="DialogResult"/> values.</returns>
    public static DialogResult Show(Control parent, string text, string caption, MessageBoxButtons buttons, MessageBoxIcon icon, MessageBoxDefaultButton defaultButton)
    {
      return MessageBox.Show(parent, text, caption, buttons, icon, defaultButton, EnsureRightToLeft(parent));
    }

    /// <summary>
    /// Displays a message box.
    /// </summary>
    /// <param name="parent">The control that will own the modal dialog box.</param>
    /// <param name="text">The text to display in the message box.</param>
    /// <param name="caption">The text to display in the title bar of the message box.</param>
    /// <param name="buttons">One of the <see cref="MessageBoxButtons"/> values that specifies which buttons to display in the message box.</param>
    /// <param name="icon">One of the <see cref="MessageBoxIcon"/> values that specifies which icon to display in the message box.</param>
    /// <param name="defaultButton">One of the <see cref="MessageBoxDefaultButton"/> values that specifies the default button for the message box.</param>
    /// <param name="options">One of the <see cref="MessageBoxOptions"/> values that specifies which display and association options will be used for the message box. You may pass in 0 if you wish to use the defaults.</param>
    /// <returns>One of the <see cref="DialogResult"/> values.</returns>
    public static DialogResult Show(Control parent, string text, string caption, MessageBoxButtons buttons, MessageBoxIcon icon, MessageBoxDefaultButton defaultButton, MessageBoxOptions options)
    {
      return MessageBox.Show(parent, text, caption, buttons, icon, defaultButton, EnsureRightToLeft(parent, options));
    }

    /// <summary>
    /// Displays a message box.
    /// </summary>
    /// <param name="parent">The control that will own the modal dialog box.</param>
    /// <param name="text">The text to display in the message box.</param>
    /// <param name="caption">The text to display in the title bar of the message box.</param>
    /// <param name="buttons">One of the <see cref="MessageBoxButtons"/> values that specifies which buttons to display in the message box.</param>
    /// <param name="icon">One of the <see cref="MessageBoxIcon"/> values that specifies which icon to display in the message box.</param>
    /// <param name="defaultButton">One of the <see cref="MessageBoxDefaultButton"/> values that specifies the default button for the message box.</param>
    /// <param name="options">One of the <see cref="MessageBoxOptions"/> values that specifies which display and association options will be used for the message box. You may pass in 0 if you wish to use the defaults.</param>
    /// <param name="displayHelpButton">A value indicating whether to display the help button on the message box.</param>
    /// <returns>One of the <see cref="DialogResult"/> values.</returns>
    public static DialogResult Show(Control parent, string text, string caption, MessageBoxButtons buttons, MessageBoxIcon icon, MessageBoxDefaultButton defaultButton, MessageBoxOptions options, bool displayHelpButton)
    {
      return MessageBox.Show(text, caption, buttons, icon, defaultButton, EnsureRightToLeft(parent, options), displayHelpButton);
    }

    /// <summary>
    /// Displays a message box.
    /// </summary>
    /// <param name="parent">The control that will own the modal dialog box.</param>
    /// <param name="text">The text to display in the message box.</param>
    /// <param name="caption">The text to display in the title bar of the message box.</param>
    /// <param name="buttons">One of the <see cref="MessageBoxButtons"/> values that specifies which buttons to display in the message box.</param>
    /// <param name="icon">One of the <see cref="MessageBoxIcon"/> values that specifies which icon to display in the message box.</param>
    /// <param name="defaultButton">One of the <see cref="MessageBoxDefaultButton"/> values that specifies the default button for the message box.</param>
    /// <param name="options">One of the <see cref="MessageBoxOptions"/> values that specifies which display and association options will be used for the message box. You may pass in 0 if you wish to use the defaults.</param>
    /// <param name="helpFilePath">The path and name of the Help file to display when the user clicks the Help button.</param>
    /// <returns>One of the <see cref="DialogResult"/> values.</returns>
    public static DialogResult Show(Control parent, string text, string caption, MessageBoxButtons buttons, MessageBoxIcon icon, MessageBoxDefaultButton defaultButton, MessageBoxOptions options, string helpFilePath)
    {
      return MessageBox.Show(parent, text, caption, buttons, icon, defaultButton, EnsureRightToLeft(parent, options), helpFilePath);
    }

    /// <summary>
    /// Displays a message box.
    /// </summary>
    /// <param name="parent">The control that will own the modal dialog box.</param>
    /// <param name="text">The text to display in the message box.</param>
    /// <param name="caption">The text to display in the title bar of the message box.</param>
    /// <param name="buttons">One of the <see cref="MessageBoxButtons"/> values that specifies which buttons to display in the message box.</param>
    /// <param name="icon">One of the <see cref="MessageBoxIcon"/> values that specifies which icon to display in the message box.</param>
    /// <param name="defaultButton">One of the <see cref="MessageBoxDefaultButton"/> values that specifies the default button for the message box.</param>
    /// <param name="options">One of the <see cref="MessageBoxOptions"/> values that specifies which display and association options will be used for the message box. You may pass in 0 if you wish to use the defaults.</param>
    /// <param name="helpFilePath">The path and name of the Help file to display when the user clicks the Help button.</param>
    /// <param name="keyword">The Help keyword to display when the user clicks the Help button.</param>
    /// <returns>One of the <see cref="DialogResult"/> values.</returns>
    public static DialogResult Show(Control parent, string text, string caption, MessageBoxButtons buttons, MessageBoxIcon icon, MessageBoxDefaultButton defaultButton, MessageBoxOptions options, string helpFilePath, string keyword)
    {
      return MessageBox.Show(parent, text, caption, buttons, icon, defaultButton, EnsureRightToLeft(parent, options), helpFilePath, keyword);
    }

    /// <summary>
    /// Displays a message box.
    /// </summary>
    /// <param name="parent">The control that will own the modal dialog box.</param>
    /// <param name="text">The text to display in the message box.</param>
    /// <param name="caption">The text to display in the title bar of the message box.</param>
    /// <param name="buttons">One of the <see cref="MessageBoxButtons"/> values that specifies which buttons to display in the message box.</param>
    /// <param name="icon">One of the <see cref="MessageBoxIcon"/> values that specifies which icon to display in the message box.</param>
    /// <param name="defaultButton">One of the <see cref="MessageBoxDefaultButton"/> values that specifies the default button for the message box.</param>
    /// <param name="options">One of the <see cref="MessageBoxOptions"/> values that specifies which display and association options will be used for the message box. You may pass in 0 if you wish to use the defaults.</param>
    /// <param name="helpFilePath">The path and name of the Help file to display when the user clicks the Help button.</param>
    /// <param name="navigator">One of the <see cref="HelpNavigator"/> values.</param>
    /// <returns>One of the <see cref="DialogResult"/> values.</returns>
    public static DialogResult Show(Control parent, string text, string caption, MessageBoxButtons buttons, MessageBoxIcon icon, MessageBoxDefaultButton defaultButton, MessageBoxOptions options, string helpFilePath, HelpNavigator navigator)
    {
      return MessageBox.Show(parent, text, caption, buttons, icon, defaultButton, EnsureRightToLeft(parent, options), helpFilePath, navigator);
    }

    /// <summary>
    /// Displays a message box.
    /// </summary>
    /// <param name="parent">The control that will own the modal dialog box.</param>
    /// <param name="text">The text to display in the message box.</param>
    /// <param name="caption">The text to display in the title bar of the message box.</param>
    /// <param name="buttons">One of the <see cref="MessageBoxButtons"/> values that specifies which buttons to display in the message box.</param>
    /// <param name="icon">One of the <see cref="MessageBoxIcon"/> values that specifies which icon to display in the message box.</param>
    /// <param name="defaultButton">One of the <see cref="MessageBoxDefaultButton"/> values that specifies the default button for the message box.</param>
    /// <param name="options">One of the <see cref="MessageBoxOptions"/> values that specifies which display and association options will be used for the message box. You may pass in 0 if you wish to use the defaults.</param>
    /// <param name="helpFilePath">The path and name of the Help file to display when the user clicks the Help button.</param>
    /// <param name="navigator">One of the <see cref="HelpNavigator"/> values.</param>
    /// <param name="helpParameter">The numeric ID of the Help topic to display when the user clicks the Help button.</param>
    /// <returns>One of the <see cref="DialogResult"/> values.</returns>
    public static DialogResult Show(Control parent, string text, string caption, MessageBoxButtons buttons, MessageBoxIcon icon, MessageBoxDefaultButton defaultButton, MessageBoxOptions options, string helpFilePath, HelpNavigator navigator, object helpParameter)
    {
      return MessageBox.Show(parent, text, caption, buttons, icon, defaultButton, EnsureRightToLeft(parent, options), helpFilePath, navigator, helpParameter);
    }

    /// <summary>
    /// Initiailizes a <see cref="MessageBoxOptions"/> with the right to left settings if needed.
    /// </summary>
    /// <param name="control">The control that will own the message box.</param>
    /// <returns>A <see cref="MessageBoxOptions"/> with right to left options set if needed.</returns>
    private static MessageBoxOptions EnsureRightToLeft(Control control)
    {
      return EnsureRightToLeft(control, (MessageBoxOptions)0);
    }

    /// <summary>
    /// Combines the specified <see cref="MessageBoxOptions"/> with the right to left settings if needed.
    /// </summary>
    /// <param name="control">The control that will own the message box.</param>
    /// <param name="options">The <see cref="MessageBoxOptions"/> to combine with.</param>
    /// <returns>A <see cref="MessageBoxOptions"/> with right to left options set if needed.</returns>
    private static MessageBoxOptions EnsureRightToLeft(Control control, MessageBoxOptions options)
    {
      if (control.RightToLeft == RightToLeft.Inherit)
      {
        Control parent = control.Parent;

        while (parent != null)
        {
          if (parent.RightToLeft != RightToLeft.Inherit)
          {
            if (parent.RightToLeft == RightToLeft.Yes)
            {
              return MessageBoxOptions.RightAlign | MessageBoxOptions.RtlReading | options;
            }
          }
        }

        if (CultureInfo.CurrentUICulture.TextInfo.IsRightToLeft)
        {
          return MessageBoxOptions.RightAlign | MessageBoxOptions.RtlReading | options;
        }
      }
      else if (control.RightToLeft == RightToLeft.Yes)
      {
        return MessageBoxOptions.RightAlign | MessageBoxOptions.RtlReading | options;
      }

      return options;
    }
  }
}