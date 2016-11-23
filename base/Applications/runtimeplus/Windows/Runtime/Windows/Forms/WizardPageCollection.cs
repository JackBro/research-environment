

namespace System.Extensions.Windows.Forms
{
  using System;
  using System.Collections.ObjectModel;

  /// <summary>
  /// Contains the IWizardPage instances that are attached to a WizardForm.
  /// </summary>
  [Serializable]
  public class WizardPageCollection : Collection<WizardPage>
  {
    /// <summary>
    /// Stores the containing <see cref="Wizard"/> control.
    /// </summary>
    [NonSerialized()]
    private Wizard owner;

    /// <summary>
    /// Initializes a new instance of the <see cref="WizardPageCollection"/> class.
    /// </summary>
    /// <param name="owner">The owning <see cref="Wizard"/> of this collection.</param>
    internal WizardPageCollection(Wizard owner)
    {
      this.owner = owner;
    }

    /// <summary>
    /// Raised when an IWizardPage is added to the Dictionary.
    /// </summary>
    public event DataEventHandler<WizardPage> PageAdded;

    /// <summary>
    /// Raised when an IWizardPage is removed from the Dictionary.
    /// </summary>
    public event DataEventHandler<WizardPage> PageRemoved;

    /// <summary>
    /// Gets the Wizard control.
    /// </summary>
    public Wizard Owner
    {
      get { return this.owner; }
    }

    /// <summary>
    /// Removes all objects from the Dictionary.
    /// </summary>
    protected override void ClearItems()
    {
      while (this.Count > 0)
      {
        this.RemoveItem(0);
      }
    }

    /// <summary>
    /// Adds an item to the Dictionary.
    /// </summary>
    /// <param name="index">The position in the collection to add the item.</param>
    /// <param name="item">The item to add.</param>
    protected override void InsertItem(int index, WizardPage item)
    {
      base.InsertItem(index, item);
      this.OnPageAdded(new DataEventArgs<WizardPage>(item));
    }

    /// <summary>
    /// Removes an item from the Dictionary.
    /// </summary>
    /// <param name="index">The position of the item to remove.</param>
    protected override void RemoveItem(int index)
    {
      WizardPage page = this[index];
      base.RemoveItem(index);
      this.OnPageRemoved(new DataEventArgs<WizardPage>(page));
    }

    /// <summary>
    /// Replaces the item at the specified index.
    /// </summary>
    /// <param name="index">The index at which to replace the page.</param>
    /// <param name="item">The page to replace with.</param>
    protected override void SetItem(int index, WizardPage item)
    {
      this.RemoveItem(index);
      this.InsertItem(index, item);
    }

    /// <summary>
    /// Raises the PageAdded event when a page is added to the Dictionary.
    /// </summary>
    /// <param name="e">The details of the page that was added.</param>
    protected virtual void OnPageAdded(DataEventArgs<WizardPage> e)
    {
      if (this.PageAdded != null)
      {
        this.PageAdded(this, e);
      }
    }

    /// <summary>
    /// Raises the PageRemoved event when a page is removed from the Dictionary.
    /// </summary>
    /// <param name="e">The details of the page that was removed.</param>
    protected virtual void OnPageRemoved(DataEventArgs<WizardPage> e)
    {
      if (this.PageRemoved != null)
      {
        this.PageRemoved(this, e);
      }
    }
  }
}
