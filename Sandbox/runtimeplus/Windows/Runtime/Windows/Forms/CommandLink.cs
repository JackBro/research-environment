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

namespace System.Extensions.Windows.Forms
{
  using System;
  using System.ComponentModel;
  using System.Drawing;
  using System.Drawing.Drawing2D;
  using System.Runtime.InteropServices;
  using System.Security.Permissions;
  using System.Windows.Forms;
  using System.Extensions.Internal;

  /// <summary>
  /// The icon to show on the <see cref="CommandLink"/>.
  /// </summary>
  public enum CommandLinkIconStyle
  {
    /// <summary>
    /// Shows the standard icon of the selection arrow.
    /// </summary>
    Standard,

    /// <summary>
    /// Shows the shield icon prevalent with UAC on Vista.
    /// </summary>
    Shield
  }

  /// <summary>
  /// An implementation of the <see cref="CommandLink"/>.  Uses native support on Vista with Visual Styles enabled.  Will provide a rendered approximation as needed.
  /// </summary>
  [ToolboxBitmap(typeof(Button))]
  public class CommandLink : Button
  {
    /// <summary>
    /// The icon to show.
    /// </summary>
    private CommandLinkIconStyle iconStyle;

    /// <summary>
    /// The note to show.
    /// </summary>
    private string note = string.Empty;

    #region No Visual Style Support - Fields

    /// <summary>
    /// The <see cref="Label"/> that will draw the main text.
    /// </summary>
    private Label textLabel;

    /// <summary>
    /// The <see cref="Label"/> that will draw the note text.
    /// </summary>
    private Label noteLabel;

    /// <summary>
    /// The <see cref="PictureBox"/> that will draw the arrow.
    /// </summary>
    private PictureBox iconPictureBox;

    /// <summary>
    /// The <see cref="Color"/> of the text.
    /// </summary>
    private Color textColor;

    /// <summary>
    /// The <see cref="Color"/> of the active text.
    /// </summary>
    private Color activeTextColor;

    /// <summary>
    /// Whether the mouse is currently over this control.
    /// </summary>
    private bool mouseOver;

    /// <summary>
    /// Whether the mouse is currently pressed down on this control.
    /// </summary>
    private bool mouseDown;

    #endregion

    /// <summary>
    /// Initializes a new instance of the <see cref="CommandLink"/> class.
    /// </summary>
    public CommandLink()
    {
      if (NativeMethods.CanDrawVistaStyles)
      {
        this.FlatStyle = FlatStyle.System;
      }
      else
      {
        this.CreateNoStylesControls();
      }
    }

    /// <summary>
    /// Gets or sets the <see cref="FlatStyle"/> for the control.  This property is not meaningful for this control.
    /// </summary>
    [Browsable(false)]
    [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
    [EditorBrowsable(EditorBrowsableState.Never)]
    public new FlatStyle FlatStyle
    {
      get { return base.FlatStyle; }
      set { base.FlatStyle = value; }
    }

    /// <summary>
    /// Gets or sets the icon to show on the <see cref="CommandLink"/>.
    /// </summary>
    [SRCategory("Category_Appearance")]
    [Description("Specifies the icon to draw")]
    [DefaultValue(CommandLinkIconStyle.Standard)]
    public CommandLinkIconStyle Style
    {
      get
      {
        return this.iconStyle;
      }

      set
      {
        if (this.iconStyle != value)
        {
          this.iconStyle = value;
          this.UpdateIcon();
        }
      }
    }

    /// <summary>
    /// Gets or sets the note to display below the main button text.
    /// </summary>
    [SRCategory("Category_Appearance")]
    [Description("Specifies the supporting note text")]
    [DefaultValue("")]
    public string Note
    {
      get
      {
        return this.note;
      }

      set
      {
        if (!string.Equals(this.note, value))
        {
          this.note = value;
          this.UpdateNote();
        }
      }
    }

    /// <summary>
    /// Gets the <see cref="CreateParams"/> for this control.  Overridden to support native drawing if available.
    /// </summary>
    protected override CreateParams CreateParams
    {
      [SecurityPermission(SecurityAction.LinkDemand, Flags = SecurityPermissionFlag.UnmanagedCode)]
      get
      {
        CreateParams cp = base.CreateParams;
        if (NativeMethods.CanDrawVistaStyles)
        {
          cp.Style |= NativeMethods.BS_COMMANDLINK;
        }

        return cp;
      }
    }

    /// <summary>
    /// Gets the default <see cref="Size"/> this control should have.  Overridden to accomodate increased CommandLink size.
    /// </summary>
    protected override Size DefaultSize
    {
      get { return new Size(180, 58); }
    }

    /// <summary>
    /// Overridden to support drawing regardless of native support.
    /// </summary>
    /// <param name="pevent">The pevent parameter.</param>
    protected override void OnPaint(PaintEventArgs pevent)
    {
      if (NativeMethods.CanDrawVistaStyles)
      {
        base.OnPaint(pevent);
      }
      else
      {
        Graphics g = pevent.Graphics;

        Rectangle borderRectangle = ClientRectangle;
        borderRectangle.Width -= 1;
        borderRectangle.Height -= 1;

        Pen borderPen = null;
        Brush backgroundBrush = null;

        if (this.mouseDown)
        {
          borderPen = new Pen(Color.FromArgb(198, 198, 198));
          backgroundBrush = new SolidBrush(Color.FromArgb(246, 246, 246));
        }
        else if (this.mouseOver)
        {
          // hover mode
          borderPen = new Pen(Color.FromArgb(198, 198, 198));
          backgroundBrush = new LinearGradientBrush(ClientRectangle, BackColor, Color.FromArgb(246, 246, 246), LinearGradientMode.Vertical);
        }
        else if (IsDefault || Focused)
        {
          // button is the default, draw blue border
          borderPen = new Pen(Color.FromArgb(198, 244, 255));
          borderRectangle.Width -= 2;
          borderRectangle.Height -= 2;
          borderRectangle.X += 1;
          borderRectangle.Y += 1;
        }

        if (backgroundBrush == null)
        {
          backgroundBrush = new SolidBrush(BackColor);
        }

        g.FillRectangle(backgroundBrush, ClientRectangle);
        backgroundBrush.Dispose();

        if (borderPen != null)
        {
          borderPen.LineJoin = LineJoin.Round;
          g.DrawRectangle(borderPen, borderRectangle);
          borderPen.Dispose();
        }

        if (Focused)
        {
          borderRectangle.Inflate(-2, -2);
          using (Pen focusPen = new Pen(Color.Black))
          {
            focusPen.DashStyle = DashStyle.Dot;
            g.DrawRectangle(focusPen, borderRectangle);
          }
        }
      }
    }

    /// <summary>
    /// Update the note for the control, will delegates to windows or custom renderer as needed.
    /// </summary>
    private void UpdateNote()
    {
      if (NativeMethods.CanDrawVistaStyles)
      {
        HandleRef handle = new HandleRef(this, Handle);
        NativeMethods.SendMessage(handle, NativeMethods.BCM_SETNOTE, IntPtr.Zero, this.note);
      }
      else
      {
        this.UpdateNoteNoStyles();
      }
    }

    /// <summary>
    /// Update the icon for the control, will delegte to windows or custom renderer as needed.
    /// </summary>
    private void UpdateIcon()
    {
      if (NativeMethods.CanDrawVistaStyles)
      {
        HandleRef handle = new HandleRef(this, Handle);
        NativeMethods.SendMessage(handle, NativeMethods.BCM_SETSHIELD, IntPtr.Zero, new IntPtr(this.iconStyle == CommandLinkIconStyle.Shield ? 1 : 0));
      }
      else
      {
        this.UpdateIconNoStyles();
      }
    }

    #region No Visual Styles Support

    /// <summary>
    /// Creates all of the controls necessary to draw this manually.
    /// </summary>
    private void CreateNoStylesControls()
    {
      this.textColor = Color.FromArgb(21, 28, 85);
      this.activeTextColor = Color.FromArgb(7, 74, 229);

      this.textLabel = new Label();
      this.noteLabel = new Label();
      this.iconPictureBox = new PictureBox();
      ((ISupportInitialize)this.iconPictureBox).BeginInit();
      SuspendLayout();

      this.textLabel.AutoSize = true;
      this.textLabel.BackColor = Color.Transparent;
      this.textLabel.Font = new Font("Segoe UI", 12F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, (byte)0);
      this.textLabel.ForeColor = this.textColor;
      this.textLabel.Location = new Point(26, 10);
      this.textLabel.Name = "textLabel";
      this.textLabel.Size = new Size(0, 21);
      this.textLabel.TabIndex = 0;
      this.textLabel.UseCompatibleTextRendering = true;
      this.textLabel.Click += new EventHandler(this.ChildControl_Click);
      this.textLabel.MouseDown += new MouseEventHandler(this.CommandLink_MouseDown);
      this.textLabel.MouseUp += new MouseEventHandler(this.CommandLink_MouseUp);
      this.textLabel.MouseEnter += new EventHandler(this.CommandLink_MouseEnter);
      this.textLabel.MouseLeave += new EventHandler(this.CommandLink_MouseLeave);
      this.textLabel.MouseMove += new MouseEventHandler(this.CommandLink_MouseMove);

      this.noteLabel.BackColor = Color.Transparent;
      this.noteLabel.Font = new Font("Segoe UI", 9F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, (byte)0);
      this.noteLabel.ForeColor = this.textColor;
      this.noteLabel.Location = new Point(27, 32);
      this.noteLabel.Name = "noteLabel";
      this.noteLabel.Size = new Size(370, 36);
      this.noteLabel.TabIndex = 1;
      this.noteLabel.UseCompatibleTextRendering = true;
      this.noteLabel.Click += new EventHandler(this.ChildControl_Click);
      this.noteLabel.MouseDown += new MouseEventHandler(this.CommandLink_MouseDown);
      this.noteLabel.MouseUp += new MouseEventHandler(this.CommandLink_MouseUp);
      this.noteLabel.MouseEnter += new EventHandler(this.CommandLink_MouseEnter);
      this.noteLabel.MouseLeave += new EventHandler(this.CommandLink_MouseLeave);
      this.noteLabel.MouseMove += new MouseEventHandler(this.CommandLink_MouseMove);

      this.iconPictureBox.BackColor = Color.Transparent;
      this.iconPictureBox.Location = new Point(10, 13);
      this.iconPictureBox.Name = "iconPictureBox";
      this.iconPictureBox.Size = new Size(16, 16);
      this.iconPictureBox.TabIndex = 2;
      this.iconPictureBox.TabStop = false;
      this.iconPictureBox.Image = System.Extensions.Properties.Resources.CommandLink_RestingArrow;
      this.iconPictureBox.Click += new EventHandler(this.ChildControl_Click);
      this.iconPictureBox.MouseDown += new MouseEventHandler(this.CommandLink_MouseDown);
      this.iconPictureBox.MouseUp += new MouseEventHandler(this.CommandLink_MouseUp);
      this.iconPictureBox.MouseEnter += new EventHandler(this.CommandLink_MouseEnter);
      this.iconPictureBox.MouseLeave += new EventHandler(this.CommandLink_MouseLeave);
      this.iconPictureBox.MouseMove += new MouseEventHandler(this.CommandLink_MouseMove);

      this.FlatStyle = FlatStyle.Standard;
      this.MouseDown += new MouseEventHandler(this.CommandLink_MouseDown);
      this.MouseUp += new MouseEventHandler(this.CommandLink_MouseUp);
      this.MouseEnter += new EventHandler(this.CommandLink_MouseEnter);
      this.MouseLeave += new EventHandler(this.CommandLink_MouseLeave);
      this.MouseMove += new MouseEventHandler(this.CommandLink_MouseMove);
      this.TextChanged += new EventHandler(this.CommandLink_TextChanged);
      this.Controls.Add(this.iconPictureBox);
      Controls.Add(this.noteLabel);
      Controls.Add(this.textLabel);
      UseVisualStyleBackColor = false;
      ((System.ComponentModel.ISupportInitialize)this.iconPictureBox).EndInit();
      ResumeLayout(false);
      PerformLayout();
    }

    /// <summary>
    /// Updates the label to the control's text when drawing manually.
    /// </summary>
    /// <param name="sender">Sender of the event.</param>
    /// <param name="e">EventArgs of the event.</param>
    private void CommandLink_TextChanged(object sender, EventArgs e)
    {
      this.textLabel.Text = Text;
    }

    /// <summary>
    /// Updates the icon when drawing manually.
    /// </summary>
    private void UpdateIconNoStyles()
    {
      if (this.iconStyle == CommandLinkIconStyle.Shield)
      {
          this.iconPictureBox.Image = System.Extensions.Properties.Resources.Shield;
      }
      else
      {
        if (this.mouseOver)
        {
            this.iconPictureBox.Image = System.Extensions.Properties.Resources.CommandLink_SelectedArrow;
        }
        else
        {
            this.iconPictureBox.Image = System.Extensions.Properties.Resources.CommandLink_RestingArrow;
        }

        Invalidate();
      }
    }

    /// <summary>
    /// Updates the notes text when drawing manually.
    /// </summary>
    private void UpdateNoteNoStyles()
    {
      this.noteLabel.Text = this.note;
    }

    /// <summary>
    /// Passes click events when drawing manually.
    /// </summary>
    /// <param name="sender">Sender of the event.</param>
    /// <param name="e">EventArgs of the event.</param>
    private void ChildControl_Click(object sender, EventArgs e)
    {
      OnClick(e);
    }

    /// <summary>
    /// Tracks when the MouseDown event occurs when drawing manually.
    /// </summary>
    /// <param name="sender">Sender of the event.</param>
    /// <param name="e">EventAgs of the event.</param>
    private void CommandLink_MouseDown(object sender, MouseEventArgs e)
    {
      this.mouseDown = true;
      this.Invalidate();
    }

    /// <summary>
    /// Tracks when the MouseUp event occurs when drawing manually.
    /// </summary>
    /// <param name="sender">Sender of the event.</param>
    /// <param name="e">EventAgs of the event.</param>
    private void CommandLink_MouseUp(object sender, MouseEventArgs e)
    {
      this.mouseDown = false;
      Invalidate();
    }

    /// <summary>
    /// Tracks when the MouseMove event occurs when drawing manually.
    /// </summary>
    /// <param name="sender">Sender of the event.</param>
    /// <param name="e">EventAgs of the event.</param>
    private void CommandLink_MouseMove(object sender, MouseEventArgs e)
    {
      this.CommandLink_MouseEnter(sender, EventArgs.Empty);
    }

    /// <summary>
    /// Tracks when the MouseEnter event occurs when drawing manually.
    /// </summary>
    /// <param name="sender">Sender of the event.</param>
    /// <param name="e">EventAgs of the event.</param>
    private void CommandLink_MouseEnter(object sender, EventArgs e)
    {
      if (!this.mouseOver)
      {
        Point p = this.PointToClient(Control.MousePosition);
        this.mouseOver = ClientRectangle.Contains(p);
        if (this.mouseOver)
        {
          this.textLabel.ForeColor = this.activeTextColor;
          this.noteLabel.ForeColor = this.activeTextColor;
          this.UpdateIconNoStyles();
          this.Invalidate();
        }
      }
    }

    /// <summary>
    /// Tracks when the MouseLeave event occurs when drawing manually.
    /// </summary>
    /// <param name="sender">Sender of the event.</param>
    /// <param name="e">EventAgs of the event.</param>
    private void CommandLink_MouseLeave(object sender, EventArgs e)
    {
      if (this.mouseOver)
      {
        Point p = this.PointToClient(Control.MousePosition);
        this.mouseOver = ClientRectangle.Contains(p);
        if (!this.mouseOver)
        {
          this.textLabel.ForeColor = this.textColor;
          this.noteLabel.ForeColor = this.textColor;
          this.UpdateIconNoStyles();
          this.Invalidate();
        }
      }
    }

    #endregion
  }
}
