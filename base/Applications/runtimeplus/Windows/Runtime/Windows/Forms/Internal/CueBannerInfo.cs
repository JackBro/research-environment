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
  /// <summary>
  /// An info class to store the details of the cue banner for a TextBox.
  /// </summary>
  internal class CueBannerInfo
  {
    /// <summary>
    /// The default value for the CueBanner property of a TextBox.
    /// </summary>
    public const string DefaultCueBanner = "";

    /// <summary>
    /// The default value for the ShowCueWhenFocused property of a TextBox.
    /// </summary>
    public const bool DefaultShowCueWhenFocused = true;

    /// <summary>
    /// Initializes a new instance of the <see cref="CueBannerInfo"/> class.
    /// </summary>
    public CueBannerInfo()
    {
      this.CueBanner = DefaultCueBanner;
      this.ShowCueWhenFocused = DefaultShowCueWhenFocused;
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="CueBannerInfo"/> class.
    /// </summary>
    /// <param name="cueBanner">The text to show as the cue banner.</param>
    /// <param name="showCueWhenFocused">Whether the cue banner is shown when the TextBox has focus.</param>
    public CueBannerInfo(string cueBanner, bool showCueWhenFocused)
    {
      this.CueBanner = cueBanner;
      this.ShowCueWhenFocused = showCueWhenFocused;
    }

    /// <summary>
    /// Gets or sets the text to show as the cue banner for the TextBox.
    /// </summary>
    public string CueBanner { get; set; }

    /// <summary>
    /// Gets or sets a value indicating whether the cue banner is shown when the TextBox has focus.
    /// </summary>
    public bool ShowCueWhenFocused { get; set; }
  }
}
