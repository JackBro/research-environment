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
  using System.ComponentModel;
  using System.ComponentModel.Design;
  using System.Drawing.Design;
  using System.Windows.Forms;

  internal class WizardDesignerActionList : DesignerActionList
  {
    public WizardDesignerActionList(IComponent component)
      : base(component)
    {
    }

    [/*Editor(typeof(WizardPageCollectionEditor), typeof(UITypeEditor)),*/ TypeConverter(typeof(WizardPageCollectionConverter))]
    [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode")]
    public WizardPageCollection WizardPages
    {
      get
      {
        Wizard wizard = Wizard;
        if (wizard == null)
        {
          return null;
        }

        return wizard.WizardPages;
      }
    }

    [TypeConverter(typeof(WizardPageCollectionEnumConverter)), Editor(typeof(WizardPageCollectionEnumEditor), typeof(UITypeEditor))]
    [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode")]
    public WizardPageCollection WizardPagesEnum
    {
      get { return this.WizardPages; }
    }

    private Wizard Wizard
    {
      get { return (Wizard)Component; }
    }

    public override DesignerActionItemCollection GetSortedActionItems()
    {
      DesignerActionItemCollection items = new DesignerActionItemCollection();
      Wizard wizard = Wizard;
      if (wizard == null)
      {
        return items;
      }

      items.Add(new DesignerActionHeaderItem("Maintain Pages", "Maintain"));
      items.Add(new DesignerActionPropertyItem("WizardPages", "Wizard Pages", "Maintain"));
      items.Add(new DesignerActionMethodItem(this, "AddPage", "Add Page", "Maintain", true));
      if (wizard.WizardPages.Count > 0)
      {
        items.Add(new DesignerActionMethodItem(this, "RemovePage", "Remove Page", "Maintain", "Description Goes here", true));
        items.Add(new DesignerActionMethodItem(this, "RemoveAllPages", "Remove All Pages", "Maintain", true));
        items.Add(new DesignerActionHeaderItem("Navigate Pages", "Navigate"));
        items.Add(new DesignerActionPropertyItem("WizardPagesEnum", "Go to Page", "Navigate"));
        if (wizard.CurrentPageIndex > 0)
        {
          items.Add(new DesignerActionMethodItem(this, "PreviousPage", "Previous Page", "Navigate", true));
        }

        if (wizard.CurrentPageIndex < wizard.WizardPages.Count - 1)
        {
          items.Add(new DesignerActionMethodItem(this, "NextPage", "Next Page", "Navigate", true));
        }
      }

      return items;
    }

    [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode")]
    private void AddPage()
    {
      Wizard wizard = Wizard;
      if (wizard == null)
      {
        return;
      }

      IDesignerHost service = (IDesignerHost)GetService(typeof(IDesignerHost));
      if (service == null)
      {
        return;
      }

      WizardPage page = (WizardPage)service.CreateComponent(typeof(WizardPage));
      if (wizard.WizardPages.Count != 0)
      {
        wizard.WizardPages.Insert(wizard.CurrentPageIndex + 1, page);
        this.NextPage();
      }
      else
      {
        wizard.WizardPages.Add(page);
        DeselectComponent();
        SelectComponent();
      }
    }
    
    [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode")]
    private void NextPage()
    {
      Wizard wizard = Wizard;
      if (wizard != null)
      {
        wizard.CurrentPageIndex++;
      }

      DeselectComponent();
      SelectComponent();
    }

    [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode")]
    private void PreviousPage()
    {
      Wizard wizard = Wizard;
      if (wizard != null)
      {
        wizard.CurrentPageIndex--;
      }

      DeselectComponent();
      SelectComponent();
    }

    [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode")]
    private void RemoveAllPages()
    {
      Wizard wizard = Wizard;
      if (wizard == null)
      {
        return;
      }

      if (MessageBoxHelper.Show(wizard.FindForm(), "Are you sure you want to remove all the pages?", "Remove Confirmation", MessageBoxButtons.YesNo, MessageBoxIcon.Question) != DialogResult.Yes)
      {
        return;
      }

      IDesignerHost service = (IDesignerHost)GetService(typeof(IDesignerHost));
      if (service == null)
      {
        return;
      }

      while (wizard.WizardPages.Count > 0)
      {
        WizardPage page = wizard.WizardPages[wizard.CurrentPageIndex];
        wizard.WizardPages.Remove(page);
        service.DestroyComponent((Component)page);
      }

      DeselectComponent();
      SelectComponent();
    }

    [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode")]
    private void RemovePage()
    {
      Wizard wizard = Wizard;
      if (wizard == null)
      {
        return;
      }

      if (MessageBoxHelper.Show(wizard.FindForm(), "Are you sure you want to remove the page?", "Remove Confirmation", MessageBoxButtons.YesNo, MessageBoxIcon.Question) != DialogResult.Yes)
      {
        return;
      }

      IDesignerHost service = (IDesignerHost)GetService(typeof(IDesignerHost));
      if (service == null)
      {
        return;
      }

      WizardPage page = wizard.WizardPages[wizard.CurrentPageIndex];
      wizard.WizardPages.Remove(page);
      service.DestroyComponent((Component)page);

      SelectComponent();
    }
  }
}
