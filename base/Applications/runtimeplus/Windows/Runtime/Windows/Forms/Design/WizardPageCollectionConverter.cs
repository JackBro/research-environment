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

namespace System.Extensions.Windows.Forms.Design
{
  using System;
  using System.ComponentModel;
  using System.Globalization;

  internal class WizardPageCollectionConverter : TypeConverter
  {
    public override object ConvertTo(ITypeDescriptorContext context, CultureInfo culture, object value, Type destinationType)
    {
      WizardPageCollection pages = (WizardPageCollection)value;
      if (pages.Count != 0)
      {
        if (pages.Count != 1)
        {
          return pages.Count + " pages";
        }
        else
        {
          return "1 page";
        }
      }
      else
      {
        return "Empty";
      }
    }
  }
}
