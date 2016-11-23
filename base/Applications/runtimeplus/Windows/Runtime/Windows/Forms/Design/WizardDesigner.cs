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
  using System.Collections;
  using System.ComponentModel;
  using System.ComponentModel.Design;
  using System.Drawing;
  using System.Windows.Forms;
  using System.Windows.Forms.Design;

  internal class WizardDesigner : ParentControlDesigner
  {
    private DesignerActionListCollection actionList = new DesignerActionListCollection();
    private WizardDesignerActionList wizardActionList;
    private bool forwardDragEventsToPage;
    private bool currentlySelected;

    public override DesignerActionListCollection ActionLists
    {
      get { return this.actionList; }
    }

    public override ICollection AssociatedComponents
    {
      get { return ((Wizard)Control).WizardPages; }
    }

    public override bool CanParent(Control control)
    {
      WizardPage wizardPage = control as WizardPage;
      Wizard wizard = (Wizard)Control;

      return wizardPage != null && !wizard.WizardPages.Contains(wizardPage);
    }

    public override bool CanParent(ControlDesigner controlDesigner)
    {
      return controlDesigner is WizardPageDesigner && !((Wizard)Control).WizardPages.Contains((WizardPage)controlDesigner.Control);
    }

    protected override bool GetHitTest(Point point)
    {
      if (!this.currentlySelected)
      {
        return false;
      }

      Wizard wizard = (Wizard)Control;
      if (wizard.WizardPages.Count <= 0)
      {
        return false;
      }

      Control nextButton = wizard.Controls["buttonPanel"].Controls["nextButton"];
      if (nextButton.Visible && nextButton.Enabled && nextButton.ClientRectangle.Contains(nextButton.PointToClient(point)))
      {
        return true;
      }

      Control backButton = wizard.Controls["buttonPanel"].Controls["backButton"];
      if (backButton.Visible && backButton.Enabled && backButton.ClientRectangle.Contains(backButton.PointToClient(point)))
      {
        return true;
      }

      return false;
    }

    protected override void Dispose(bool disposing)
    {
      if (!disposing)
      {
        base.Dispose(disposing);
      }

      ISelectionService service = (ISelectionService)GetService(typeof(ISelectionService));
      if (service == null)
      {
        service.SelectionChanged -= new EventHandler(this.Service_SelectionChanged);
      }

      Wizard wizard = (Wizard)Control;
      wizard.CurrentPageIndexChanged -= new EventHandler(this.Wizard_CurrentPageIndexChanged);
      wizard.WizardPages.PageAdded -= new DataEventHandler<WizardPage>(this.WizardPages_PageAdded);
      wizard.WizardPages.PageRemoved -= new DataEventHandler<WizardPage>(this.WizardPages_PageRemoved);

      IDesignerHost host = (IDesignerHost)GetService(typeof(IDesignerHost));
      if (host == null)
      {
        foreach (WizardPage page in wizard.WizardPages)
        {
          host.DestroyComponent(page);
        }
      }
    }

    public override void Initialize(IComponent component)
    {
      base.Initialize(component);
      this.AutoResizeHandles = true;
      ISelectionService service = (ISelectionService)GetService(typeof(ISelectionService));
      if (service != null)
      {
        service.SelectionChanged += new EventHandler(this.Service_SelectionChanged);
      }

      Wizard wizard = (Wizard)Control;
      this.wizardActionList = new WizardDesignerActionList(wizard);
      this.actionList.Add(this.wizardActionList);
      wizard.CurrentPageIndexChanged += new EventHandler(this.Wizard_CurrentPageIndexChanged);
      wizard.WizardPages.PageAdded += new DataEventHandler<WizardPage>(this.WizardPages_PageAdded);
      wizard.WizardPages.PageRemoved += new DataEventHandler<WizardPage>(this.WizardPages_PageRemoved);
    }

    public override void InitializeNewComponent(IDictionary defaultValues)
    {
      base.InitializeNewComponent(defaultValues);
      Wizard wizard = (Wizard)Control;
      if (wizard == null)
      {
        return;
      }

      IDesignerHost service = (IDesignerHost)GetService(typeof(IDesignerHost));
      if (service == null)
      {
        return;
      }

      WizardPage wizardPage = (WizardPage)service.CreateComponent(typeof(WizardPage));
      wizardPage.Text = "Welcome Page";
      wizardPage.Name = "welcomeWizardPage";
      wizard.WizardPages.Add(wizardPage);
    }

    protected override void OnDragComplete(DragEventArgs de)
    {
      if (this.forwardDragEventsToPage)
      {
        this.forwardDragEventsToPage = false;
        this.GetCurrentWizardPageDesigner().OnDragCompleteInternal(de);
      }
      else
      {
        base.OnDragComplete(de);
      }
    }

    protected override void OnDragDrop(DragEventArgs de)
    {
      if (this.forwardDragEventsToPage)
      {
        this.forwardDragEventsToPage = false;
        WizardPageDesigner currentWizardPageDesigner = this.GetCurrentWizardPageDesigner();
        if (currentWizardPageDesigner != null)
        {
          currentWizardPageDesigner.OnDragDropInternal(de);
        }
      }
      else
      {
        de.Effect = DragDropEffects.None;
      }
    }

    protected override void OnDragEnter(DragEventArgs de)
    {
      Wizard wizard = (Wizard)Control;

      if (wizard.WizardPages.Count <= 0)
      {
        base.OnDragEnter(de);
      }
      else
      {
        WizardPage page = wizard.WizardPages[wizard.CurrentPageIndex];
        if (!page.ClientRectangle.Contains(page.PointToClient(new Point(de.X, de.Y))))
        {
          base.OnDragEnter(de);
        }
        else
        {
          this.GetWizardPageDesigner(page).OnDragEnterInternal(de);
          this.forwardDragEventsToPage = true;
        }
      }
    }

    protected override void OnDragLeave(EventArgs e)
    {
      if (this.forwardDragEventsToPage)
      {
        this.forwardDragEventsToPage = false;
        WizardPageDesigner currentWizardPageDesigner = this.GetCurrentWizardPageDesigner();
        if (currentWizardPageDesigner != null)
        {
          currentWizardPageDesigner.OnDragLeaveInternal(e);
        }
      }
      else
      {
        base.OnDragLeave(e);
      }
    }

    protected override void OnDragOver(DragEventArgs de)
    {
      Wizard wizard = (Wizard)Control;
      if (wizard.WizardPages.Count <= 0)
      {
        de.Effect = DragDropEffects.None;
      }
      else
      {
        WizardPage page = wizard.WizardPages[wizard.CurrentPageIndex];
        WizardPageDesigner wizardPageDesigner = this.GetWizardPageDesigner(page);
        if (!page.ClientRectangle.Contains(page.PointToClient(new Point(de.X, de.Y))))
        {
          if (!this.forwardDragEventsToPage)
          {
            de.Effect = DragDropEffects.None;
          }
          else
          {
            this.forwardDragEventsToPage = false;
            wizardPageDesigner.OnDragLeaveInternal(EventArgs.Empty);
            base.OnDragEnter(de);
          }
        }
        else
        {
          if (!this.forwardDragEventsToPage)
          {
            base.OnDragLeave(EventArgs.Empty);
            wizardPageDesigner.OnDragEnterInternal(de);
            this.forwardDragEventsToPage = true;
          }
          else
          {
            wizardPageDesigner.OnDragOverInternal(de);
          }
        }
      }
    }

    protected override void OnGiveFeedback(GiveFeedbackEventArgs e)
    {
      if (this.forwardDragEventsToPage)
      {
        WizardPageDesigner currentWizardPageDesigner = this.GetCurrentWizardPageDesigner();
        if (currentWizardPageDesigner != null)
        {
          currentWizardPageDesigner.OnGiveFeedbackInternal(e);
        }
      }
      else
      {
        base.OnGiveFeedback(e);
      }
    }

    protected override void OnPaintAdornments(PaintEventArgs pe)
    {
      Wizard wizard = (Wizard)Control;

      if (wizard.WizardPages.Count == 0)
      {
        using (StringFormat sf = new StringFormat())
        {
          sf.Alignment = StringAlignment.Center;
          sf.LineAlignment = StringAlignment.Center;
          pe.Graphics.DrawString("There are no wizard pages to display", SystemFonts.DefaultFont, SystemBrushes.WindowText, wizard.ClientRectangle, sf);
        }

        using (Pen pen = new Pen(Brushes.Black, 1))
        {
          pen.DashStyle = System.Drawing.Drawing2D.DashStyle.Dash;
          Rectangle rect = new Rectangle(0, 0, wizard.Width - 1, wizard.Height - 1);
          pe.Graphics.DrawRectangle(pen, rect);
        }
      }
    }

    private void Service_SelectionChanged(object sender, EventArgs e)
    {
      this.currentlySelected = false;
      ISelectionService service = (ISelectionService)GetService(typeof(ISelectionService));
      if (service == null)
      {
        return;
      }

      Wizard wizard = (Wizard)Control;
      foreach (object selectedComponent in service.GetSelectedComponents())
      {
        if (selectedComponent == Control)
        {
          this.currentlySelected = true;
        }

        WizardPage wizardPage = GetParentOfComponent<WizardPage>(selectedComponent);
        if (wizardPage != null && wizardPage.Owner == wizard)
        {
          this.currentlySelected = false;
          wizard.CurrentPage = wizardPage;
          this.RefreshComponent();
        }
      }
    }

    internal static T GetParentOfComponent<T>(object component) where T : Control, new()
    {
      Control parent = component as Control;
      if (parent == null)
      {
        return null;
      }

      while (parent != null && !(parent is T))
      {
        parent = parent.Parent;
      }

      return (T)parent;
    }

    private void Wizard_CurrentPageIndexChanged(object sender, EventArgs e)
    {
      this.RefreshComponent();
    }

    private void WizardPages_PageAdded(object sender, DataEventArgs<WizardPage> e)
    {
      this.RefreshComponent();
    }

    private void WizardPages_PageRemoved(object sender, DataEventArgs<WizardPage> e)
    {
      this.RefreshComponent();
    }

    private WizardPageDesigner GetCurrentWizardPageDesigner()
    {
      Wizard wizard = (Wizard)Control;
      if (wizard.WizardPages.Count <= 0)
      {
        return null;
      }

      IDesignerHost host = (IDesignerHost)GetService(typeof(IDesignerHost));
      if (host == null)
      {
        return null;
      }

      WizardPage page = wizard.WizardPages[wizard.CurrentPageIndex];
      return (WizardPageDesigner)host.GetDesigner(page);
    }

    private WizardPageDesigner GetWizardPageDesigner(WizardPage page)
    {
      IDesignerHost service = (IDesignerHost)GetService(typeof(IDesignerHost));
      if (service == null)
      {
        return null;
      }

      return (WizardPageDesigner)service.GetDesigner(page);
    }

    private void RefreshComponent()
    {
      DesignerActionUIService service = (DesignerActionUIService)GetService(typeof(DesignerActionUIService));
      if (service == null)
      {
        return;
      }

      service.Refresh(Control);
    }
  }
}
