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
  using System.Drawing.Design;
  using System.Windows.Forms;
  using System.Windows.Forms.Design;

  internal class WizardPageCollectionEnumEditor : UITypeEditor
  {
    public override object EditValue(ITypeDescriptorContext context, IServiceProvider provider, object value)
    {
      IWindowsFormsEditorService service = (IWindowsFormsEditorService)provider.GetService(typeof(IWindowsFormsEditorService));
      if (service == null)
      {
        return value;
      }

      WizardPageCollection pages = (WizardPageCollection)value;
      ListBox box = new ListBox();
      box.Tag = new object[] { context, provider, value };
      box.Dock = DockStyle.Fill;
      box.HorizontalScrollbar = true;
      box.SelectedIndexChanged += new EventHandler(this.Box_SelectedIndexChanged);

      foreach (WizardPage page in pages)
      {
        box.Items.Add(page);
      }

      service.DropDownControl(box);
      return value;
    }

    public override UITypeEditorEditStyle GetEditStyle(ITypeDescriptorContext context)
    {
      return UITypeEditorEditStyle.DropDown;
    }

    private void Box_SelectedIndexChanged(object sender, EventArgs e)
    {
      ListBox box = (ListBox)sender;
      IServiceProvider provider = (IServiceProvider)((object[])box.Tag)[1];
      WizardPageCollection pages = (WizardPageCollection)((object[])box.Tag)[2];
      IWindowsFormsEditorService service = (IWindowsFormsEditorService)provider.GetService(typeof(IWindowsFormsEditorService));
      if (service == null)
      {
        return;
      }

      pages.Owner.CurrentPageIndex = box.SelectedIndex;
      service.CloseDropDown();
      box.Dispose();
    }
  }
}
