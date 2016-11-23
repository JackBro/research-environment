

namespace System.Extensions.Windows.Forms
{
  using System;
  using System.ComponentModel;
  using System.Drawing;
  using System.Windows.Forms;
  using System.Extensions.Internal;
  using System.Extensions.Windows.Forms.Internal;

  /// <summary>
  /// Displays a progress dialog to the user while a background operation executes.
  /// </summary>
  [ToolboxBitmap(typeof(BackgroundWorker)), DefaultEvent("DoWork"), SRDescription("ProgressDialog_Description")]
  public sealed class ProgressDialog : Component
  {
    /// <summary>
    /// Key for the Event container for the DoWork event.
    /// </summary>
    private static readonly object doWorkKey = new object();

    /// <summary>
    /// Key for the Event container for the RunWorkerCompleted event.
    /// </summary>
    private static readonly object runWorkerCompletedKey = new object();

    /// <summary>
    /// Key for the Event container for the ProgressChanged event.
    /// </summary>
    private static readonly object progressChangedKey = new object();

    /// <summary>
    /// The dialog to show.
    /// </summary>
    private Internal.ProgressDialog dialog;

    /// <summary>
    /// The worker thread.
    /// </summary>
    private BackgroundWorker worker;

    /// <summary>
    /// The data association with a completed operation.
    /// </summary>
    private RunWorkerCompletedEventArgs completedData;

    /// <summary>
    /// Initializes a new instance of the <see cref="ProgressDialog"/> class.
    /// </summary>
    public ProgressDialog()
    {
      this.Reset();
    }

    /// <summary>
    /// Occurs when RunWorker is called.
    /// </summary>
    [SRCategory("Category_Asynchronous"), SRDescription("ProgressDialog_DoWork")]
    public event DoWorkEventHandler DoWork
    {
      add { this.Events.AddHandler(doWorkKey, value); }
      remove { this.Events.AddHandler(doWorkKey, value); }
    }

    /// <summary>
    /// Occurs when ReportProgress is called.
    /// </summary>
    [SRCategory("Category_Asynchronous"), SRDescription("ProgressDialog_ProgressChanged")]
    public event ProgressChangedEventHandler ProgressChanged
    {
      add { this.Events.AddHandler(progressChangedKey, value); }
      remove { this.Events.AddHandler(progressChangedKey, value); }
    }

    /// <summary>
    /// Occurs when the background operation has completed, has been canceled, or has raised an exception.
    /// </summary>
    [SRCategory("Category_Asynchronous"), SRDescription("ProgressDialog_RunWorkerCompleted")]
    public event RunWorkerCompletedEventHandler RunWorkerCompleted
    {
      add { this.Events.AddHandler(runWorkerCompletedKey, value); }
      remove { this.Events.AddHandler(runWorkerCompletedKey, value); }
    }

    /// <summary>
    /// Gets or sets a value indicating whether the <see cref="ProgressDialog"/> can report progress updProgressDialog_WorkerReportsProgressates.
    /// </summary>
    [DefaultValue(false), SRCategory("Category_Asynchronous"), SRDescription("ProgressDialog_WorkerReportsProgress")]
    public bool WorkerReportsProgress
    {
      get { return this.worker.WorkerReportsProgress; }
      set { this.worker.WorkerReportsProgress = value; }
    }

    /// <summary>
    /// Gets or sets a value indicating whether the <see cref="ProgressDialog"/> supports asynchronous cancellation.
    /// </summary>
    [DefaultValue(false), SRCategory("Category_Asynchronous"), SRDescription("ProgressDialog_WorkerSupportsCancellation")]
    public bool WorkerSupportsCancellation
    {
      get
      {
        return this.worker.WorkerSupportsCancellation;
      }

      set
      {
        this.worker.WorkerSupportsCancellation = value;
        this.dialog.AllowCancel = value;
      }
    }

    /// <summary>
    /// Gets or sets the title text of the dialog.
    /// </summary>
    [DefaultValue(""), SRCategory("Category_Appearance"), SRDescription("ProgressDialog_TitleText")]
    public string TitleText
    {
      get { return this.dialog.Text; }
      set { this.dialog.Text = value; }
    }

    /// <summary>
    /// Gets or sets the header text of the dialog.
    /// </summary>
    [DefaultValue(""), SRCategory("Category_Appearance"), SRDescription("ProgressDialog_HeaderText")]
    public string HeaderText
    {
      get { return this.dialog.HeaderText; }
      set { this.dialog.HeaderText = value; }
    }

    /// <summary>
    /// Gets or sets the message text of the form.
    /// </summary>
    [DefaultValue(""), SRCategory("Category_Appearance"), SRDescription("ProgressDialog_MessageText")]
    public string MessageText
    {
      get { return this.dialog.MessageText; }
      set { this.dialog.MessageText = value; }
    }

    /// <summary>
    /// Gets a value indicating whether the application or the user has requested cancellation of a background operation.
    /// </summary>
    [Browsable(false)]
    public bool CancellationPending
    {
      get { return this.worker.CancellationPending; }
    }

    /// <summary>
    /// Gets a value indicating whether the <see cref="ProgressDialog"/> is running an asynchronous operation is executing.
    /// </summary>
    [Browsable(false)]
    public bool IsBusy
    {
      get { return this.worker.IsBusy; }
    }

    /// <summary>
    /// Gets the <see cref="Exception"/> that occurred during the asynchronous operation, <b>null</b> otherwise.
    /// </summary>
    [Browsable(false)]
    public Exception Error
    {
      get { return this.completedData == null ? null : this.completedData.Error; }
    }

    /// <summary>
    /// Gets the <see cref="DoWorkEventArgs.Result"/> of the asynchronous operation.
    /// </summary>
    [Browsable(false)]
    public object Result
    {
      get { return this.completedData == null ? null : this.completedData.Result; }
    }

    /// <summary>
    /// Requests cancellation of a pending background operation.
    /// </summary>
    /// <exception cref="InvalidOperationException"><see cref="WorkerSupportsCancellation"/> is false.</exception>
    public void CancelAsync()
    {
      this.worker.CancelAsync();
    }

    /// <summary>
    /// Raises the <see cref="ProgressChanged"/> event.
    /// </summary>
    /// <param name="percentProgress">THe percentage, from 0 to 100, of the background operation that is complete.</param>
    /// <exception cref="InvalidOperationException">The <see cref="WorkerReportsProgress"/> property is set to false.</exception>
    public void ReportProgress(int percentProgress)
    {
      this.worker.ReportProgress(percentProgress, null);
    }

    /// <summary>
    /// Raises the <see cref="ProgressChanged"/> event.
    /// </summary>
    /// <param name="percentProgress">THe percentage, from 0 to 100, of the background operation that is complete.</param>
    /// <param name="userState">The state object passed to <see cref="ReportProgress(int,object)"/>.</param>
    /// <exception cref="InvalidOperationException">The <see cref="WorkerReportsProgress"/> property is set to false.</exception>
    public void ReportProgress(int percentProgress, object userState)
    {
      this.worker.ReportProgress(percentProgress, userState);
    }

    /// <summary>
    /// Starts executing of a background operation.
    /// </summary>
    /// <param name="owner">The owning form that is launching this dialog.</param>
    /// <exception cref="InvalidOperationException"><see cref="IsBusy"/> is true.</exception>
    public void RunWorker(Form owner)
    {
      this.RunWorker(owner, null);
    }

    /// <summary>
    /// Starts executing of a background operation.
    /// </summary>
    /// <param name="owner">The owning form that is launching this dialog.</param>
    /// <param name="argument">A parameter for use by the background operation to be executed in the <see cref="DoWork"/> event handler.</param>
    /// <returns><b>True</b> if the background operation completed without being cancelled or throwing an error.</returns>
    /// <exception cref="InvalidOperationException"><see cref="IsBusy"/> is true.</exception>
    public bool RunWorker(Form owner, object argument)
    {
      this.dialog.Owner = owner;
      this.dialog.Icon = owner.Icon;

      this.worker.RunWorkerAsync(argument);

      this.dialog.AllowCancel = this.worker.WorkerSupportsCancellation;
      this.dialog.ShowDialog();

      return this.Error == null && !this.completedData.Cancelled;
    }

    /// <summary>
    /// Resets all properties to their default values.
    /// </summary>
    public void Reset()
    {
      if (this.dialog != null)
      {
        this.dialog.Dispose();
      }

      this.dialog = new Internal.ProgressDialog();
      this.dialog.FormClosing += new FormClosingEventHandler(this.Dialog_FormClosing);

      if (this.worker != null)
      {
        this.worker.Dispose();
      }

      this.worker = new BackgroundWorker();
      this.worker.DoWork += new DoWorkEventHandler(this.Worker_DoWork);
      this.worker.ProgressChanged += new ProgressChangedEventHandler(this.Worker_ProgressChanged);
      this.worker.RunWorkerCompleted += new RunWorkerCompletedEventHandler(this.Worker_RunWorkerCompleted);
    }

    /// <summary>
    /// Called when the actual progress dialog is trying to close (via Cancel button, ALT+F4, etc.)
    /// </summary>
    /// <param name="sender">The sending object.</param>
    /// <param name="e">The data for the event.</param>
    private void Dialog_FormClosing(object sender, FormClosingEventArgs e)
    {
      // user clicked the cancel button
      if (this.dialog.DialogResult == DialogResult.Abort)
      {
        if (this.worker.WorkerSupportsCancellation)
        {
          e.Cancel = true;
          this.CancelAsync();
        }
      }
      else
      {
        if (this.dialog.DialogResult != DialogResult.OK && e.CloseReason == CloseReason.UserClosing)
        {
          e.Cancel = true;
        }
      }
    }

    /// <summary>
    /// Handles the internal BackgroundWorker DoWork event.
    /// </summary>
    /// <param name="sender">The internal BackgroundWorker.</param>
    /// <param name="e">The EventArgs that holds the data.</param>
    private void Worker_DoWork(object sender, DoWorkEventArgs e)
    {
      this.OnDoWork(e);
    }

    /// <summary>
    /// Handles the internal BackgroundWorker ProgressChanged event.
    /// </summary>
    /// <param name="sender">The internal BackgroundWorker.</param>
    /// <param name="e">The EventArgs that holds the data.</param>
    private void Worker_ProgressChanged(object sender, ProgressChangedEventArgs e)
    {
      // TODO: notify the form of the progress change
      this.OnProgressChanged(e);
    }

    /// <summary>
    /// Handles the internal BackgroundWorker RunWorkerCompleted event.
    /// </summary>
    /// <param name="sender">The internal BackgroundWorker.</param>
    /// <param name="e">The EventArgs that holds the data.</param>
    private void Worker_RunWorkerCompleted(object sender, RunWorkerCompletedEventArgs e)
    {
      this.completedData = e;

      // close the progress dialog
      this.dialog.AllowCancel = false;
      this.dialog.DialogResult = DialogResult.OK;
      this.dialog.Close();

      this.OnRunWorkerCompleted(e);
    }

    /// <summary>
    /// Raises the <see cref="DoWork"/> event.
    /// </summary>
    /// <param name="e">An <see cref="EventArgs"/> that contains the event data.</param>
    private void OnDoWork(DoWorkEventArgs e)
    {
      DoWorkEventHandler handler = (DoWorkEventHandler)this.Events[doWorkKey];
      if (handler != null)
      {
        handler(this, e);
      }
    }

    /// <summary>
    /// Raises the <see cref="ProgressChanged"/> event.
    /// </summary>
    /// <param name="e">An <see cref="EventArgs"/> that contains the event data.</param>
    private void OnProgressChanged(ProgressChangedEventArgs e)
    {
      ProgressChangedEventHandler handler = (ProgressChangedEventHandler)this.Events[progressChangedKey];
      if (handler != null)
      {
        handler(this, e);
      }
    }

    /// <summary>
    /// Raises the <see cref="RunWorkerCompleted"/> event.
    /// </summary>
    /// <param name="e">An <see cref="EventArgs"/> that contains the event data.</param>
    private void OnRunWorkerCompleted(RunWorkerCompletedEventArgs e)
    {
      RunWorkerCompletedEventHandler handler = (RunWorkerCompletedEventHandler)this.Events[runWorkerCompletedKey];
      if (handler != null)
      {
        handler(this, e);
      }
    }
  }
}
