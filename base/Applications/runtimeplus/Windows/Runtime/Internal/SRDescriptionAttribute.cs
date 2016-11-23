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

namespace System.Extensions.Internal
{
  using System;
  using System.ComponentModel;
  using System.Extensions.Properties;

  /// <summary>
  /// Provides a localizable way to get designer descriptions.
  /// </summary>
  [AttributeUsage(AttributeTargets.All)]
  public class SRDescriptionAttribute : DescriptionAttribute
  {
    private bool replaced;
    
	/// <summary>
    /// 
	/// </summary>
	/// <param name="description"></param>
    public SRDescriptionAttribute(string description)
      : base(description)
    {
    }
	/// <summary>
	/// 
	/// </summary>
    public override string Description
    {
      get
      {
        if (!this.replaced)
        {
          this.replaced = true;
          this.DescriptionValue = Resources.ResourceManager.GetString(base.Description);
        }

        return base.Description;
      }
    }
  }
}
