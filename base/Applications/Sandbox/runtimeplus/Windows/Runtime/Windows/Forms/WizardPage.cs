

namespace System.Extensions.Windows.Forms
{
  using System;
  using System.ComponentModel;
  using System.Drawing;
  using System.Windows.Forms;
  using System.Extensions.Windows.Forms.Design;

  /// <summary>
  /// Represents a single page in a <see cref="Wizard"/>.
  /// </summary>
  [Designer(typeof(WizardPageDesigner)), DefaultProperty("Text"), ToolboxItem(false), DesignTimeVisible(false)]
  public class WizardPage : Panel
  {
    #region Variables

    private Wizard owner;
    private bool isInitialized;

    private int? backIndex;
    private int? nextIndex;
    private bool isValid = true;

    private string backButtonText;
    private string nextButtonText;
    private string finishButtonText;

    #endregion

    /// <summary>
    /// Initializes a new instance of the <see cref="WizardPage"/> class.
    /// </summary>
    public WizardPage()
    {
      this.SetStyle(ControlStyles.CacheText, true);
      this.Text = null;
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="WizardPage"/> class with the specified text.
    /// </summary>
    /// <param name="text"></param>
    public WizardPage(string text)
      : this()
    {
      this.Text = text;
    }

    /// <summary>
    /// Occurs the first time the <see cref="WizardPage"/> is navigated to.
    /// </summary>
    public event EventHandler Initialize;

    /// <summary>
    /// Occurs when the <see cref="WizardPage"/> is navigated to.
    /// </summary>
    public event EventHandler LoadState;

    /// <summary>
    /// Occurs when the <see cref="WizardPage"/> is navigated away from or when the <b>Finish</b> button is clicked on the last page.
    /// </summary>
    public event EventHandler SaveState;

    /// <summary>
    /// Occurs when the IsValid property of this <see cref="WizardPage"/> changes.
    /// </summary>
    public event EventHandler IsValidChanged;

    /// <summary>
    /// Occurs when the BackButtonText property of this <see cref="WizardPage"/> changes.
    /// </summary>
    public event EventHandler BackButtonTextChanged;

    /// <summary>
    /// Occurs when the NextButtonText property of this <see cref="WizardPage"/> changes.
    /// </summary>
    public event EventHandler NextButtonTextChanged;

    /// <summary>
    /// Occurs when the FinishButtonText property of this <see cref="WizardPage"/> changes.
    /// </summary>
    public event EventHandler FinishButtonTextChanged;

    /// <summary>
    /// Raises the <see cref="Initialize"/> event.
    /// </summary>
    protected virtual void OnInitialize()
    {
      if (this.Initialize != null)
      {
        this.Initialize(this, EventArgs.Empty);
      }
    }

    /// <summary>
    /// Raises the <see cref="LoadState"/> event.
    /// </summary>
    protected virtual void OnLoadState()
    {
      if (this.LoadState != null)
      {
        this.LoadState(this, EventArgs.Empty);
      }
    }

    /// <summary>
    /// Raises the <see cref="SaveState"/> event.
    /// </summary>
    protected virtual void OnSaveState()
    {
      if (this.SaveState != null)
      {
        this.SaveState(this, EventArgs.Empty);
      }
    }

    /// <summary>
    /// Raises the <see cref="IsValidChanged"/> event.
    /// </summary>
    protected virtual void OnIsValidChanged()
    {
      if (this.IsValidChanged != null)
      {
        this.IsValidChanged(this, EventArgs.Empty);
      }
    }

    /// <summary>
    /// Raises the <see cref="BackButtonTextChanged"/> event.
    /// </summary>
    protected virtual void OnBackButtonTextChanged()
    {
      if (this.BackButtonTextChanged != null)
      {
        this.BackButtonTextChanged(this, EventArgs.Empty);
      }
    }

    /// <summary>
    /// Raises the <see cref="NextButtonTextChanged"/> event.
    /// </summary>
    protected virtual void OnNextButtonTextChanged()
    {
      if (this.NextButtonTextChanged != null)
      {
        this.NextButtonTextChanged(this, EventArgs.Empty);
      }
    }

    /// <summary>
    /// Raises the <see cref="FinishButtonTextChanged"/> event.
    /// </summary>
    protected virtual void OnFinishButtonTextChanged()
    {
      if (this.FinishButtonTextChanged != null)
      {
        this.FinishButtonTextChanged(this, EventArgs.Empty);
      }
    }

    /// <summary>
    /// Gets the Wizard that contains this WizardPage.
    /// </summary>
    [Browsable(false), DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
    public Wizard Owner
    {
      get { return this.owner; }
    }

    /// <summary>
    /// Gets or sets the overrided index of the page that the Back process will go to.
    /// </summary>
    [DefaultValue(null)]
    public virtual int? BackIndex
    {
      get { return this.backIndex; }
      set { this.backIndex = value; }
    }

    /// <summary>
    /// Gets or sets the overrided index of the page that the Next process will go to.
    /// </summary>
    [DefaultValue(null)]
    public virtual int? NextIndex
    {
      get { return this.nextIndex; }
      set { this.nextIndex = value; }
    }

    /// <summary>
    /// Gets or sets the overridden text of the back button.
    /// </summary>
    [DefaultValue(null)]
    public string BackButtonText
    {
      get
      {
        return this.backButtonText;
      }

      set
      {
        if (value != this.backButtonText)
        {
          this.backButtonText = value;
          this.OnBackButtonTextChanged();
        }
      }
    }

    /// <summary>
    /// Gets or sets the overridden text of the next button.
    /// </summary>
    [DefaultValue(null)]
    public string NextButtonText
    {
      get
      {
        return this.nextButtonText;
      }

      set
      {
        if (value != this.nextButtonText)
        {
          this.nextButtonText = value;
          this.OnNextButtonTextChanged();
        }
      }
    }

    /// <summary>
    /// Gets or sets the overridden text of the finish button.
    /// </summary>
    [DefaultValue(null)]
    public string FinishButtonText
    {
      get
      {
        return this.finishButtonText;
      }

      set
      {
        if (value != this.finishButtonText)
        {
          this.finishButtonText = value;
          this.OnFinishButtonTextChanged();
        }
      }
    }

    /// <summary>
    /// Gets or sets whether this <see cref="WizardPage"/> is in a valid state.
    /// </summary>
    [DefaultValue(true)]
    public virtual bool IsValid
    {
      get
      {
        return this.isValid;
      }

      set
      {
        if (value != this.isValid)
        {
          this.isValid = value;
          this.OnIsValidChanged();
        }
      }
    }

    /// <summary>
    /// Gets or sets the text to display for this page.
    /// </summary>
    [Browsable(true)]
    public override string Text
    {
      get { return base.Text; }
      set { base.Text = value; }
    }

    #region Hidden Properties/Events

    #region Properties

    /// <summary>
    /// This property is not meaningful for this control.
    /// </summary>
    [EditorBrowsable(EditorBrowsableState.Never), Browsable(false)]
    public override AnchorStyles Anchor
    {
      get { return base.Anchor; }
      set { base.Anchor = value; }
    }

    /// <summary>
    /// This property is not meaningful for this control.
    /// </summary>
    [EditorBrowsable(EditorBrowsableState.Never), Browsable(false), DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
    public override bool AutoSize
    {
      get { return base.AutoSize; }
      set { base.AutoSize = value; }
    }

    /// <summary>
    /// This property is not meaningful for this control.
    /// </summary>
    [EditorBrowsable(EditorBrowsableState.Never), Browsable(false), Localizable(false), DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
    public override AutoSizeMode AutoSizeMode
    {
      get { return AutoSizeMode.GrowOnly; }
      set { }
    }

    /// <summary>
    /// This property is not meaningful for this control.
    /// </summary>
    [EditorBrowsable(EditorBrowsableState.Never), Browsable(false), DefaultValue(DockStyle.Fill)]
    public override DockStyle Dock
    {
      get { return base.Dock; }
      set { base.Dock = value; }
    }

    /// <summary>
    /// This property is not meaningful for this control.
    /// </summary>
    [EditorBrowsable(EditorBrowsableState.Never), Browsable(false)]
    public new bool Enabled
    {
      get { return base.Enabled; }
      set { base.Enabled = value; }
    }

    /// <summary>
    /// This property is not meaningful for this control.
    /// </summary>
    [EditorBrowsable(EditorBrowsableState.Never), Browsable(false)]
    public new Point Location
    {
      get { return base.Location; }
      set { base.Location = value; }
    }

    private bool ShouldSerializeLocation()
    {
      return false;
    }

    /// <summary>
    /// This property is not meaningful for this control.
    /// </summary>
    [Browsable(false), EditorBrowsable(EditorBrowsableState.Never), DefaultValue(typeof(Size), "0, 0")]
    public override Size MaximumSize
    {
      get { return base.MaximumSize; }
      set { base.MaximumSize = value; }
    }

    /// <summary>
    /// This property is not meaningful for this control.
    /// </summary>
    [EditorBrowsable(EditorBrowsableState.Never), Browsable(false)]
    public override Size MinimumSize
    {
      get { return base.MinimumSize; }
      set { base.MinimumSize = value; }
    }

    /// <summary>
    /// This property is not meaningful for this control.
    /// </summary>
    [EditorBrowsable(EditorBrowsableState.Never), Browsable(false)]
    public new Size PreferredSize
    {
      get { return base.PreferredSize; }
    }

    /// <summary>
    /// This property is not meaningful for this control.
    /// </summary>
    [EditorBrowsable(EditorBrowsableState.Never), Browsable(false)]
    public new int TabIndex
    {
      get { return base.TabIndex; }
      set { base.TabIndex = value; }
    }

    private bool ShouldSerializeTabIndex()
    {
      return false;
    }

    /// <summary>
    /// This property is not meaningful for this control.
    /// </summary>
    [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
    public new bool TabStop
    {
      get { return base.TabStop; }
      set { base.TabStop = value; }
    }

    /// <summary>
    /// This property is not meaningful for this control.
    /// </summary>
    [EditorBrowsable(EditorBrowsableState.Never), Browsable(false)]
    public new bool Visible
    {
      get { return base.Visible; }
      set { base.Visible = value; }
    }

    #endregion

    #region Events

    /// <summary>
    /// This event is not meaningful for this control.
    /// </summary>
    [EditorBrowsable(EditorBrowsableState.Never)]
    public new event EventHandler AutoSizeChanged
    {
      add { base.AutoSizeChanged += value; }
      remove { base.AutoSizeChanged -= value; }
    }

    /// <summary>
    /// This event is not meaningful for this control.
    /// </summary>
    [EditorBrowsable(EditorBrowsableState.Never), Browsable(false)]
    public new event EventHandler DockChanged
    {
      add { base.DockChanged += value; }
      remove { base.DockChanged -= value; }
    }

    /// <summary>
    /// This event is not meaningful for this control.
    /// </summary>
    [EditorBrowsable(EditorBrowsableState.Never), Browsable(false)]
    public new event EventHandler EnabledChanged
    {
      add { base.EnabledChanged += value; }
      remove { base.EnabledChanged -= value; }
    }

    /// <summary>
    /// This event is not meaningful for this control.
    /// </summary>
    [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
    public new event EventHandler LocationChanged
    {
      add { base.LocationChanged += value; }
      remove { base.LocationChanged -= value; }
    }

    /// <summary>
    /// This event is not meaningful for this control.
    /// </summary>
    [EditorBrowsable(EditorBrowsableState.Never), Browsable(false)]
    public new event EventHandler TabIndexChanged
    {
      add { base.TabIndexChanged += value; }
      remove { base.TabIndexChanged -= value; }
    }

    /// <summary>
    /// This event is not meaningful for this control.
    /// </summary>
    [EditorBrowsable(EditorBrowsableState.Never), Browsable(false)]
    public new event EventHandler TabStopChanged
    {
      add { base.TabStopChanged += value; }
      remove { base.TabStopChanged -= value; }
    }

    /// <summary>
    /// This event is not meaningful for this control.
    /// </summary>
    [EditorBrowsable(EditorBrowsableState.Never), Browsable(false)]
    public new event EventHandler VisibleChanged
    {
      add { base.VisibleChanged += value; }
      remove { base.VisibleChanged -= value; }
    }

    #endregion

    #endregion

    /// <summary>
    /// Creates a new instance of the Controls collection for this instance.
    /// </summary>
    protected override ControlCollection CreateControlsInstance()
    {
      return new WizardPageControlCollection(this);
    }

    /// <summary>
    /// Initialize the component for the wizard.
    /// </summary>
    /// <param name="wizard">The owning wizard of this page.</param>
    public virtual void SetOwner(Wizard wizard)
    {
      this.owner = wizard;

      this.Dock = DockStyle.Fill;
      this.Padding = new Padding(19, 11, 19, 11);
      this.TabStop = false;
    }

    /// <summary>
    /// Cleans up the component when detached from the wizard.
    /// </summary>
    public virtual void Cleanup()
    {
      this.owner = null;
    }

    internal void RaiseLoadState()
    {
      this.OnLoadState();
    }

    internal void RaiseSaveState()
    {
      this.OnSaveState();
    }

    internal void RaiseInitialize()
    {
      if (!this.isInitialized)
      {
        this.OnInitialize();
        this.isInitialized = true;
      }
    }

    #region WizardPageControlCollection

    private class WizardPageControlCollection : Control.ControlCollection
    {
      public WizardPageControlCollection(WizardPage owner)
        : base(owner)
      {
      }

      public override void Add(Control value)
      {
        if (value is WizardPage)
        {
          throw new ArgumentException("Cannot add WizardPage to a WizardPage");
        }

        base.Add(value);
      }
    }

    #endregion
  }
}
