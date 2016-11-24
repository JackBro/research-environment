

namespace System.Extensions.Windows.Forms
{
  using System.Collections.Generic;
  using System.ComponentModel;
  using System.Extensions.Windows.Forms.Internal;
  using WF = System.Windows.Forms;
  using System.Extensions.Internal;

  /// <summary>
  /// Provider to expose visual styles on stock Windows Forms controls in the designer.
  /// </summary>
  [ProvideProperty("CueBanner", typeof(WF.TextBox))]
  [ProvideProperty("ShowCueWhenFocused", typeof(WF.TextBox))]
  public class VisualStylesProvider : Component, IExtenderProvider
  {
    /// <summary>
    /// Stores the relationship between the textboxes and the information.
    /// </summary>
    private Dictionary<WF.TextBox, CueBannerInfo> cueBannerInfos;

    /// <summary>
    /// Initializes a new instance of the <see cref="VisualStylesProvider"/> class without a specified container.
    /// </summary>
    public VisualStylesProvider()
    {
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="VisualStylesProvider"/> class with a specified container.
    /// </summary>
    /// <param name="parent">The container of the <see cref="VisualStylesProvider"/>.</param>
    public VisualStylesProvider(IContainer parent)
      : this()
    {
      parent.Add(this);
    }

    /// <summary>
    /// Gets the relationship between the textboxes and the information.
    /// </summary>
    private Dictionary<WF.TextBox, CueBannerInfo> CueBannerInfos
    {
      get
      {
        if (this.cueBannerInfos == null)
        {
          this.cueBannerInfos = new Dictionary<WF.TextBox, CueBannerInfo>();
        }

        return this.cueBannerInfos;
      }
    }

    /// <summary>
    /// Determines if the <see cref="VisualStylesProvider"/> can offer extender properties to the specified target component.
    /// </summary>
    /// <param name="extendee">The target object to add properties to.</param>
    /// <returns><b>true</b> if the <see cref="VisualStylesProvider"/> can offer one or more extender properties; <b>false</b> otherwise.</returns>
    public bool CanExtend(object extendee)
    {
      if (extendee is WF.TextBox && !(extendee is TextBox))
      {
        return true;
      }

      if (extendee is WF.Button)
      {
        return true;
      }

      return false;
    }

    /// <summary>
    /// Retrieves the cue banner text associated with the specified <see cref="WF.TextBox"/>.
    /// </summary>
    /// <param name="textBox">The <see cref="WF.TextBox"/> for which to retrieve the cue banner.</param>
    /// <returns>The cue banner for the specified <see cref="WF.TextBox"/>.</returns>
    [SRCategory("Category_Appearance")]
    [SRDescription("TextBox_CueBannerDescription")]
    [DefaultValue(CueBannerInfo.DefaultCueBanner)]
    [Localizable(true)]
    public string GetCueBanner(WF.TextBox textBox)
    {
      if (this.CueBannerInfos.ContainsKey(textBox))
      {
        return this.CueBannerInfos[textBox].CueBanner;
      }
      else
      {
        return CueBannerInfo.DefaultCueBanner;
      }
    }

    /// <summary>
    /// Assigns the cue banner text to the specified <see cref="WF.TextBox"/>.
    /// </summary>
    /// <param name="textBox">The <see cref="WF.TextBox"/> to assign the cue banner to.</param>
    /// <param name="cueBanner">The cue banner text to display when the <see cref="WF.TextBox"/> is empty.</param>
    public void SetCueBanner(WF.TextBox textBox, string cueBanner)
    {
      if (!this.CueBannerInfos.ContainsKey(textBox))
      {
        this.CueBannerInfos.Add(textBox, new CueBannerInfo(cueBanner, CueBannerInfo.DefaultShowCueWhenFocused));
      }

      this.CueBannerInfos[textBox].CueBanner = cueBanner;

      TextBox.SetCueBanner(textBox, this.CueBannerInfos[textBox].CueBanner, this.CueBannerInfos[textBox].ShowCueWhenFocused);
    }

    /// <summary>
    /// Retrieves whether the cue banner is shown when the <see cref="WF.TextBox"/> has focus.
    /// </summary>
    /// <param name="textBox">The <see cref="WF.TextBox"/> for which to retrieve the property.</param>
    /// <returns>The value indicating whether the cue banner is shown when the specified <see cref="WF.TextBox"/> has focus.</returns>
    [SRCategory("Category_Appearance")]
    [SRDescription("TextBox_ShowCueWhenFocusedDescription")]
    [DefaultValue(CueBannerInfo.DefaultShowCueWhenFocused)]
    public bool GetShowCueWhenFocused(WF.TextBox textBox)
    {
      if (this.CueBannerInfos.ContainsKey(textBox))
      {
        return this.CueBannerInfos[textBox].ShowCueWhenFocused;
      }
      else
      {
        return CueBannerInfo.DefaultShowCueWhenFocused;
      }
    }

    /// <summary>
    /// Assigns whether the cue banner is shown when the <see cref="WF.TextBox"/> has focus.
    /// </summary>
    /// <param name="textBox">The <see cref="WF.TextBox"/> to assign the property to.</param>
    /// <param name="showCueWhenFocused">Whether the cue banner is shown when the <see cref="WF.TextBox"/> has focus.</param>
    public void SetShowCueWhenFocused(WF.TextBox textBox, bool showCueWhenFocused)
    {
      if (!this.CueBannerInfos.ContainsKey(textBox))
      {
        this.CueBannerInfos.Add(textBox, new CueBannerInfo(CueBannerInfo.DefaultCueBanner, showCueWhenFocused));
      }
      else
      {
        this.CueBannerInfos[textBox].ShowCueWhenFocused = showCueWhenFocused;
      }

      TextBox.SetCueBanner(textBox, this.CueBannerInfos[textBox].CueBanner, this.CueBannerInfos[textBox].ShowCueWhenFocused);
    }
  }
}
