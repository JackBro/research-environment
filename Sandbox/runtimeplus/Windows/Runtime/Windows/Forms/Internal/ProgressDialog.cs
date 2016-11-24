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

namespace System.Extensions.Windows.Forms.Internal
{
  using System;
  using System.Drawing;
  using System.Windows.Forms;

  /// <summary>
  /// Represents a dialog that will show progress of a long running task.
  /// </summary>
  internal partial class ProgressDialog : Form
  {
    /// <summary>
    /// Initializes a new instance of the <see cref="ProgressDialog"/> class.
    /// </summary>
    public ProgressDialog()
    {
      this.InitializeComponent();

      if (!Application.RenderWithVisualStyles)
      {
        this.progressBar1.Visible = false;
        this.Height -= 8;
        this.messageLabel.Height = 45;
      }
    }

    /// <summary>
    /// Gets or sets the header text of the form.
    /// </summary>
    public string HeaderText
    {
      get { return this.headerLabel.Text; }
      set { this.headerLabel.Text = value; }
    }

    /// <summary>
    /// Gets or sets the message text of the form.
    /// </summary>
    public string MessageText
    {
      get { return this.messageLabel.Text; }
      set { this.messageLabel.Text = value; }
    }

    /// <summary>
    /// Gets or sets a value indicating whether the form will allow cancellation.
    /// </summary>
    public bool AllowCancel
    {
      get
      {
        return this.cancelButton.Enabled;
      }

      set
      {
        if (this.InvokeRequired)
        {
          this.BeginInvoke(new MethodInvoker(delegate
          {
            this.AllowCancel = value;
          }));
        }
        else
        {
          this.cancelButton.Enabled = value;
        }
      }
    }

    /// <summary>
    /// Called when the button panel needs to be drawn so we can draw the line.
    /// </summary>
    /// <param name="sender">The sending object.</param>
    /// <param name="e">The data for the event.</param>
    private void ButtonPanel_Paint(object sender, PaintEventArgs e)
    {
      e.Graphics.DrawLine(SystemPens.ControlDark, 0, 0, ((Control)sender).Width, 0);
    }

    /// <summary>
    /// Called when the cancel button is clicked.
    /// </summary>
    /// <param name="sender">The sending object.</param>
    /// <param name="e">The data for the event.</param>
    private void CancelButton_Click(object sender, EventArgs e)
    {
      this.cancelButton.Enabled = false;

      DialogResult = DialogResult.Abort;
      this.Close();
    }
  }
}
