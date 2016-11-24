

namespace System.Extensions.Windows.Forms
{
  using System;
  using System.ComponentModel;
  using System.Drawing;
  using System.Extensions.Internal;
  using System.Extensions.Windows.Forms.Internal;
  using WF = System.Windows.Forms;

  /// <summary>
  /// Implementation of the <see cref="WF.TextBox"/> control that supports the 'cue banner' feature
  /// introduced in XP.
  /// </summary>
  [ToolboxBitmap(typeof(WF.TextBox))]
  public class TextBox : WF.TextBox
  {
    private CueBannerInfo cueBannerInfo = new CueBannerInfo();

    /// <summary>
    /// Gets or sets the text to display as the cue banner.
    /// </summary>
    [SRCategory("Category_Appearance")]
    [SRDescription("TextBox_CueBannerDescription")]
    [DefaultValue(CueBannerInfo.DefaultCueBanner)]
    [Localizable(true)]
    public string CueBanner
    {
      get
      {
        return this.cueBannerInfo.CueBanner;
      }

      set
      {
        if (this.cueBannerInfo.CueBanner != value)
        {
          this.cueBannerInfo.CueBanner = value;
          SetCueBanner(this, this.cueBannerInfo);
        }
      }
    }

    /// <summary>
    /// Gets or sets a value indicating whether the cue banner should be visible when the box has focus.
    /// </summary>
    [SRCategory("Category_Appearance")]
    [SRDescription("TextBox_ShowCueWhenFocusedDescription")]
    [DefaultValue(CueBannerInfo.DefaultShowCueWhenFocused)]
    public bool ShowCueWhenFocused
    {
      get
      {
        return this.cueBannerInfo.ShowCueWhenFocused;
      }

      set
      {
        if (this.cueBannerInfo.ShowCueWhenFocused != value)
        {
          this.cueBannerInfo.ShowCueWhenFocused = value;
          SetCueBanner(this, this.cueBannerInfo);
        }
      }
    }

    internal static void SetCueBanner(WF.TextBox textBox, string cueBanner, bool showWhenFocused)
    {
      if (WF.Application.RenderWithVisualStyles)
      {
        NativeMethods.EditSetCueBanner(textBox, cueBanner, showWhenFocused);
      }
    }

    internal static void SetCueBanner(WF.TextBox textBox, CueBannerInfo info)
    {
      SetCueBanner(textBox, info.CueBanner, info.ShowCueWhenFocused);
    }

    /// <summary>
    /// Overridden to set the cue banner when the control is created.
    /// </summary>
    /// <param name="e">EventArgs of the event.</param>
    protected override void OnHandleCreated(EventArgs e)
    {
      base.OnHandleCreated(e);
      SetCueBanner(this, this.cueBannerInfo);
    }
  }
}
