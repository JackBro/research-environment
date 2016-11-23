
namespace System.Extensions.Windows.Forms
{
  using System;
  using System.ComponentModel;
  using System.Drawing;
  using System.Windows.Forms;
  using System.Extensions.Windows.Forms.Design;

  /// <summary>
  /// A control to represent a wizard.
  /// </summary>
  [DefaultEvent("CurrentPageIndexChanged")]
  [Designer(typeof(WizardDesigner))]
  public class Wizard : Control
  {
    /// <summary>
    /// The default text of the back button.
    /// </summary>
    private const string DefaultBackButtonText = "&Back";

    /// <summary>
    /// The default text of the next button.
    /// </summary>
    private const string DefaultNextButtonText = "&Next";

    /// <summary>
    /// The default text of the finish button.
    /// </summary>
    private const string DefaultFinishButtonText = "&Finish";

    /// <summary>
    /// The default text of the cancel button.
    /// </summary>
    private const string DefaultCancelButtonText = "Cancel";

    /// <summary>
    /// The default text of the help button.
    /// </summary>
    private const string DefaultHelpButtonText = "&Help";

    /// <summary>
    /// The collection of pages added to this wizard.
    /// </summary>
    private WizardPageCollection wizardPages;

    /// <summary>
    /// The container for the wizard pages.
    /// </summary>
    private Panel pagePanel = new Panel();

    /// <summary>
    ///  The container for the header information.
    /// </summary>
    private Panel headerPanel = new Panel();

    /// <summary>
    ///  The header label.
    /// </summary>
    private Label headerLabel = new Label();

    /// <summary>
    /// The container for the buttons.
    /// </summary>
    private Panel buttonPanel = new Panel();

    /// <summary>
    /// The next button.
    /// </summary>
    private Button nextButton = new Button();

    /// <summary>
    /// The back button.
    /// </summary>
    private Button backButton = new Button();

    /// <summary>
    /// The cancel button.
    /// </summary>
    private Button cancelButton = new Button();

    /// <summary>
    /// The help button.
    /// </summary>
    private Button helpButton = new Button();

    /// <summary>
    /// The index of the current page.
    /// </summary>
    private int currentPageIndex;

    /// <summary>
    /// Whether to confirm the user cancellation of the wizard.
    /// </summary>
    private bool confirmCancel = true;

    /// <summary>
    /// The state to pass to wizard pages when added.
    /// </summary>
    private object state;

    /// <summary>
    /// Initializes a new instance of the <see cref="Wizard"/> class.
    /// </summary>
    public Wizard()
    {
      this.wizardPages = this.CreateWizardPageCollection();
      this.wizardPages.PageAdded += new DataEventHandler<WizardPage>(this.WizardPages_PageAdded);
      this.wizardPages.PageRemoved += new DataEventHandler<WizardPage>(this.WizardPages_PageRemoved);

      this.headerPanel.SuspendLayout();
      this.buttonPanel.SuspendLayout();
      this.SuspendLayout();

      this.pagePanel.BackColor = SystemColors.Window;
      this.pagePanel.Dock = DockStyle.Fill;
      this.pagePanel.TabIndex = 1;

      this.cancelButton.Anchor = AnchorStyles.Bottom | AnchorStyles.Right;
      this.cancelButton.DialogResult = DialogResult.Cancel;
      this.cancelButton.FlatStyle = FlatStyle.System;
      this.cancelButton.Location = new Point(417, 9);
      this.cancelButton.Name = "cancelButton";
      this.cancelButton.Size = new Size(68, 23);
      this.cancelButton.TabIndex = 2;
      this.cancelButton.Text = DefaultCancelButtonText;
      this.cancelButton.UseVisualStyleBackColor = true;
      this.cancelButton.Click += new EventHandler(this.CancelButton_Click);

      this.nextButton.Anchor = AnchorStyles.Bottom | AnchorStyles.Right;
      this.nextButton.FlatStyle = FlatStyle.System;
      this.nextButton.Location = new Point(342, 9);
      this.nextButton.Name = "nextButton";
      this.nextButton.Size = new Size(68, 23);
      this.nextButton.TabIndex = 0;
      this.nextButton.Text = DefaultFinishButtonText;
      this.nextButton.UseVisualStyleBackColor = true;
      this.nextButton.Click += new EventHandler(this.NextButton_Click);

      this.backButton.Anchor = AnchorStyles.Bottom | AnchorStyles.Right;
      this.backButton.FlatStyle = FlatStyle.System;
      this.backButton.Location = new Point(267, 9);
      this.backButton.Name = "backButton";
      this.backButton.Size = new Size(68, 23);
      this.backButton.TabIndex = 1;
      this.backButton.Text = DefaultBackButtonText;
      this.backButton.UseVisualStyleBackColor = true;
      this.backButton.Click += new EventHandler(this.BackButton_Click);

      this.helpButton.Anchor = AnchorStyles.Bottom | AnchorStyles.Left;
      this.helpButton.FlatStyle = FlatStyle.System;
      this.helpButton.Location = new Point(12, 9);
      this.helpButton.Name = "helpButton";
      this.helpButton.Size = new Size(68, 23);
      this.helpButton.TabIndex = 3;
      this.helpButton.Text = DefaultHelpButtonText;
      this.helpButton.UseVisualStyleBackColor = true;
      this.helpButton.Click += new EventHandler(this.HelpButton_Click);

      this.buttonPanel.Dock = DockStyle.Bottom;
      this.buttonPanel.Location = new Point(0, 0);
      this.buttonPanel.Name = "buttonPanel";
      this.buttonPanel.Size = new Size(495, 41);
      this.buttonPanel.TabIndex = 2;
      this.buttonPanel.Paint += new PaintEventHandler(this.ButtonPanel_Paint);
      this.buttonPanel.Controls.AddRange(new Control[] { this.helpButton, this.backButton, this.nextButton, this.cancelButton });

      this.headerLabel.AutoSize = true;
      this.headerLabel.Font = new Font("Segoe UI", 12F, FontStyle.Regular, GraphicsUnit.Point, (byte)0);
      this.headerLabel.ForeColor = Color.FromArgb(15, 49, 150);
      this.headerLabel.Location = new Point(21, 17);
      this.headerLabel.Name = "headerLabel";
      this.headerLabel.TabIndex = 0;
      this.headerLabel.Text = "Page Title";

      this.headerPanel.BackColor = SystemColors.Window;
      this.headerPanel.Dock = DockStyle.Top;
      this.headerPanel.Location = new Point(0, 0);
      this.headerPanel.Name = "headerPanel";
      this.headerPanel.Size = new Size(495, 45);
      this.headerPanel.TabIndex = 0;
      this.headerPanel.Controls.AddRange(new Control[] { this.headerLabel });

      this.ClientSize = new Size(495, 100);
      this.Controls.Add(this.pagePanel);
      this.Controls.Add(this.headerPanel);
      this.Controls.Add(this.buttonPanel);
      this.Dock = DockStyle.Fill;
      this.TabStop = false;
      this.ParentChanged += new EventHandler(this.Wizard_ParentChanged);
      this.headerPanel.ResumeLayout(false);
      this.buttonPanel.ResumeLayout(false);
      this.ResumeLayout(false);

      this.UpdateButtons();
    }

    /// <summary>
    /// Occurs when the current page index changes.
    /// </summary>
    public event EventHandler CurrentPageIndexChanged;

    /// <summary>
    /// Occurs when the wizard is cancelled.
    /// </summary>
    public event CancelEventHandler Cancelling;

    /// <summary>
    /// Occurs after the wizard has been canceled before the form is closed.
    /// </summary>
    public event EventHandler Canceled;

    /// <summary>
    /// Occurs when the <b>Finish</b> button is clicked.
    /// </summary>
    public event EventHandler Finish;

    /// <summary>
    /// Gets or sets a value indicating whether the help button is visible.
    /// </summary>
    [DefaultValue(true)]
    public bool ShowHelpButton
    {
      get { return this.helpButton.Visible; }
      set { this.helpButton.Visible = value; }
    }

    /// <summary>
    /// Gets or sets a value indicating whether the back button is visible.
    /// </summary>
    [DefaultValue(true)]
    public bool ShowBackButton
    {
      get { return this.backButton.Visible; }
      set { this.backButton.Visible = value; }
    }

    /// <summary>
    /// Gets or sets the state object of the wizard, will be passed to IWizardPages when attached.
    /// </summary>
    [Browsable(false)]
    public object State
    {
      get { return this.state; }
      set { this.state = value; }
    }

    /// <summary>
    /// Gets or sets the current page index.
    /// </summary>
    [DefaultValue(0), DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
    public int CurrentPageIndex
    {
      get { return this.currentPageIndex; }
      set { this.DoUpdatePage(value, false); }
    }

    /// <summary>
    /// Gets or sets the current IWizardPage.
    /// </summary>
    [Browsable(false), DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
    public WizardPage CurrentPage
    {
      get
      {
        if (this.currentPageIndex >= 0 && this.currentPageIndex < this.wizardPages.Count)
        {
          return this.wizardPages[this.currentPageIndex];
        }

        return null;
      }

      set
      {
        int pageIndex = this.WizardPages.IndexOf(value);
        if (pageIndex == -1)
        {
          throw new ArgumentOutOfRangeException("value", "Cannot change to a WizardPage that is not part of the wizard.");
        }

        this.CurrentPageIndex = pageIndex;
      }
    }

    /// <summary>
    /// Gets or sets a value indicating whether the wizard needs to confirm a cancellation.
    /// </summary>
    [Category("Wizard")]
    [Description("Determines whether the user will be asked to confirm a cancel action.")]
    [DefaultValue(true)]
    public bool ConfirmCancel
    {
      get { return this.confirmCancel; }
      set { this.confirmCancel = value; }
    }

    /// <summary>
    /// Gets the collection of WizardPages.
    /// </summary>
    [TypeConverter(typeof(WizardPageCollectionConverter)), DesignerSerializationVisibility(DesignerSerializationVisibility.Content)]
    public WizardPageCollection WizardPages
    {
      get { return this.wizardPages; }
    }

    /// <summary>
    /// Gets or sets which control borders are docked to its parent control and how a control is resized with its parent.
    /// </summary>
    [DefaultValue(DockStyle.Fill)]
    public override DockStyle Dock
    {
      get { return base.Dock; }
      set { base.Dock = value; }
    }

    /// <summary>
    /// Sets the focus to the next/finish button.
    /// </summary>
    public void SetFocusToNextFinish()
    {
      this.nextButton.Focus();
    }

    /// <summary>
    /// Raises the Cancelling event when the user requests to close the form.
    /// </summary>
    /// <param name="e">EventArgs of the event.</param>
    protected virtual void OnCancelling(CancelEventArgs e)
    {
      if (this.Cancelling != null)
      {
        this.Cancelling(this, e);
      }

      if (!e.Cancel && this.ConfirmCancel && MessageBoxHelper.Show(this, "Are you sure you want to cancel?", FindForm().Text, MessageBoxButtons.YesNo, MessageBoxIcon.Question) == DialogResult.No)
      {
        e.Cancel = true;
      }
    }

    /// <summary>
    /// Raises the Canceled event right before the form has been closed.
    /// </summary>
    protected virtual void OnCanceled()
    {
      if (this.Canceled != null)
      {
        this.Canceled(this, EventArgs.Empty);
      }
    }

    /// <summary>
    /// Raises the CurrentPageIndexChanged event after the new page is loaded.
    /// </summary>
    protected virtual void OnCurrentPageIndexChanged()
    {
      if (this.CurrentPageIndexChanged != null)
      {
        this.CurrentPageIndexChanged(this, EventArgs.Empty);
      }
    }

    /// <summary>
    /// Raises the Finish event when the <b>Finish</b> button is clicked.
    /// </summary>
    protected virtual void OnFinish()
    {
      if (this.Finish != null)
      {
        this.Finish(this, EventArgs.Empty);
      }
    }

    /// <summary>
    /// Creates the WizardPageCollection.
    /// </summary>
    /// <returns>A <see cref="WizardPageCollection"/> to store the wizard pages.</returns>
    protected virtual WizardPageCollection CreateWizardPageCollection()
    {
      return new WizardPageCollection(this);
    }

    /// <summary>
    /// Called to update which page is shown.
    /// </summary>
    /// <param name="newIndex">The index of the page that will be shown.</param>
    /// <param name="forceRefresh">Whether to forcefully cause a refresh even if it doesn't seem to need one.</param>
    protected virtual void DoUpdatePage(int newIndex, bool forceRefresh)
    {
      if (FindForm() == null)
      {
        return;
      }

      if (this.wizardPages.Count == 0)
      {
        this.UpdateButtons();
      }
      else
      {
        if (newIndex < 0 || newIndex >= this.wizardPages.Count)
        {
          throw new ArgumentOutOfRangeException("newIndex", "The new index must be a valid index of the WizardPages collection property.");
        }

        if (newIndex != this.CurrentPageIndex || forceRefresh)
        {
          this.CurrentPage.RaiseSaveState();

          this.currentPageIndex = newIndex;

          if (!this.pagePanel.Controls.Contains(this.CurrentPage))
          {
            this.pagePanel.Controls.Add(this.CurrentPage);
          }

          this.CurrentPage.RaiseInitialize();
          this.CurrentPage.RaiseLoadState();
          this.headerLabel.Text = this.CurrentPage.Text;
          this.CurrentPage.BringToFront();

          this.UpdateButtons();

          this.OnCurrentPageIndexChanged();
        }
      }
    }

    /// <summary>
    /// Called when the control is added to a form.
    /// </summary>
    /// <param name="sender">Sender of the event.</param>
    /// <param name="e">EventArgs of the event.</param>
    private void Wizard_ParentChanged(object sender, EventArgs e)
    {
      Form parentForm = FindForm();
      if (parentForm != null)
      {
        parentForm.AcceptButton = this.nextButton;
        parentForm.CancelButton = this.cancelButton;
        parentForm.FormClosing += new FormClosingEventHandler(this.ParentForm_FormClosing);
        parentForm.FormClosed += new FormClosedEventHandler(this.ParentForm_FormClosed);

        this.DoUpdatePage(this.CurrentPageIndex, true);
      }
    }

    private void ParentForm_FormClosed(object sender, FormClosedEventArgs e)
    {
      if (this.CurrentPage != null)
      {
        this.CurrentPage.Hide();
      }

      // allow a cleanup for all the pages
      this.wizardPages.Clear();
    }

    private void ParentForm_FormClosing(object sender, FormClosingEventArgs e)
    {
      if (FindForm().DialogResult != DialogResult.OK &&
        e.CloseReason == CloseReason.UserClosing &&
        !e.Cancel)
      {
        if (!this.DoCancel())
        {
          e.Cancel = true;
        }
      }
    }

    private void HelpButton_Click(object sender, EventArgs e)
    {
      throw new NotImplementedException();
    }

    private void BackButton_Click(object sender, EventArgs e)
    {
      if (this.currentPageIndex <= 0)
      {
        return;
      }

      if (this.DesignMode)
      {
        this.CurrentPageIndex--;
        return;
      }

      if (this.CurrentPage.BackIndex.HasValue)
      {
        this.CurrentPageIndex = this.CurrentPage.BackIndex.Value;
      }
      else
      {
        this.CurrentPageIndex--;
      }
    }

    private void NextButton_Click(object sender, EventArgs e)
    {
      if (this.currentPageIndex >= this.wizardPages.Count - 1)
      {
        // this is the finish button
        if (this.DesignMode)
        {
          return;
        }

        this.OnFinish();
      }
      else
      {
        if (this.DesignMode)
        {
          this.CurrentPageIndex++;
          return;
        }

        if (this.CurrentPage.NextIndex.HasValue)
        {
          this.CurrentPageIndex = this.CurrentPage.NextIndex.Value;
        }
        else
        {
          this.CurrentPageIndex++;
        }
      }
    }

    private void CancelButton_Click(object sender, EventArgs e)
    {
      FindForm().Close();
    }

    private void ButtonPanel_Paint(object sender, PaintEventArgs e)
    {
      e.Graphics.DrawLine(SystemPens.ControlDark, 0, 0, ((Control)sender).Width, 0);
    }

    private void WizardPages_PageRemoved(object sender, DataEventArgs<WizardPage> e)
    {
      WizardPage page = e.Data;

      page.Cleanup();
      page.TextChanged -= new EventHandler(this.Page_TextChanged);
      page.IsValidChanged -= new EventHandler(this.Page_ButtonsChanged);
      page.BackButtonTextChanged -= new EventHandler(this.Page_ButtonsChanged);
      page.NextButtonTextChanged -= new EventHandler(this.Page_ButtonsChanged);
      page.FinishButtonTextChanged -= new EventHandler(this.Page_ButtonsChanged);

      if (this.currentPageIndex >= this.wizardPages.Count)
      {
        this.currentPageIndex = this.wizardPages.Count - 1;
      }

      if (this.currentPageIndex < 0)
      {
        this.currentPageIndex = 0;
      }

      this.DoUpdatePage(this.currentPageIndex, page.Visible);

      this.pagePanel.Controls.Remove(page);
    }

    private void WizardPages_PageAdded(object sender, DataEventArgs<WizardPage> e)
    {
      WizardPage page = e.Data;

      if (string.IsNullOrEmpty(page.Text))
      {
        page.Text = "Page Title";
      }

      page.TextChanged += new EventHandler(this.Page_TextChanged);
      page.IsValidChanged += new EventHandler(this.Page_ButtonsChanged);
      page.BackButtonTextChanged += new EventHandler(this.Page_ButtonsChanged);
      page.NextButtonTextChanged += new EventHandler(this.Page_ButtonsChanged);
      page.FinishButtonTextChanged += new EventHandler(this.Page_ButtonsChanged);

      page.SetOwner(this);
      this.UpdateButtons();

      if (this.wizardPages.Count == 1)
      {
        this.DoUpdatePage(this.CurrentPageIndex, true);
      }
    }

    private void Page_ButtonsChanged(object sender, EventArgs e)
    {
      this.UpdateButtons();
    }

    private void Page_TextChanged(object sender, EventArgs e)
    {
      WizardPage page = (WizardPage)sender;
      if (page == this.CurrentPage)
      {
        this.headerLabel.Text = page.Text;
      }
    }

    private void UpdateButtons()
    {
      if (this.WizardPages.Count == 0)
      {
        this.buttonPanel.Visible = false;
        this.headerPanel.Visible = false;
      }
      else
      {
        this.buttonPanel.Visible = true;
        this.headerPanel.Visible = true;
      }

      WizardPage page = this.CurrentPage;
      string backButtonText = page != null && !string.IsNullOrEmpty(page.BackButtonText) ? page.BackButtonText : DefaultBackButtonText;
      string nextButtonText = page != null && !string.IsNullOrEmpty(page.NextButtonText) ? page.NextButtonText : DefaultNextButtonText;
      string finishButtonText = page != null && !string.IsNullOrEmpty(page.FinishButtonText) ? page.FinishButtonText : DefaultFinishButtonText;

      this.backButton.Text = backButtonText;
      if (this.CurrentPageIndex > 0)
      {
        this.backButton.Enabled = true;
        if (this.CurrentPageIndex != (this.wizardPages.Count - 1))
        {
          this.nextButton.Text = nextButtonText;
        }
        else
        {
          this.nextButton.Text = finishButtonText;
        }
      }
      else
      {
        this.backButton.Enabled = false;
        if (this.wizardPages.Count > 0 && this.CurrentPageIndex != (this.wizardPages.Count - 1))
        {
          this.nextButton.Text = nextButtonText;
        }
        else
        {
          this.nextButton.Text = finishButtonText;
        }
      }

      if (this.CurrentPage != null)
      {
        this.nextButton.Enabled = this.CurrentPage.IsValid;
      }
    }

    private bool DoCancel()
    {
      CancelEventArgs cancelling = new CancelEventArgs();
      this.OnCancelling(cancelling);
      if (!cancelling.Cancel)
      {
        this.OnCanceled();

        /*      if (currentPage != null)
                currentPage.OnClosing(EventArgs.Empty);

              currentPage.UpdateWizardSettingsRequired -= new EventHandler(WizadPage_UpdateWizardSettingsRequired);

              pages.PageAdded -= new WizardPageCollectionPageAddedDelegate(WizardPage_PageAdded);
              pages.PageRemoved -= new WizardPageCollectionPageRemovedDelegate(WizardPage_PageRemoved); */

        return true;
      }

      return false;
    }
  }
}
