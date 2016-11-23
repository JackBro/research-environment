//--------------------------------------------------
// FindReplaceDialog.cs (c) 2006 by Charles Petzold
//--------------------------------------------------
using System;
using System.Windows;
using System.Windows.Controls;

namespace Petzold.NotepadClone
{
    abstract class FindReplaceDialog : Window
    {
        // Public events.
        public event EventHandler FindNext;
        public event EventHandler Replace;
        public event EventHandler ReplaceAll;

        // Protected fields.
        protected Label lblReplace;
        protected TextBox txtboxFind, txtboxReplace;
        protected CheckBox checkMatch;
        protected GroupBox groupDirection;
        protected RadioButton radioDown, radioUp;
        protected Button btnFind, btnReplace, btnAll;

        // Public properties.
        public string FindWhat
        {
            set { txtboxFind.Text = value; }
            get { return txtboxFind.Text; }
        }
        public string ReplaceWith
        {
            set { txtboxReplace.Text = value; }
            get { return txtboxReplace.Text; }
        }
        public bool MatchCase
        {
            set { checkMatch.IsChecked = value; }
            get { return (bool)checkMatch.IsChecked; }
        }
        public Direction Direction
        {
            set 
            {
                if (value == Direction.Down)
                    radioDown.IsChecked = true;
                else
                    radioUp.IsChecked = true;
            }
            get 
            { 
                return (bool)radioDown.IsChecked ? Direction.Down : Direction.Up;
            }
        }

        // Protected constructor (because class is abstract).
        protected FindReplaceDialog(Window owner)
        {
            // Set common dialog box properties.
            ShowInTaskbar = false;
            WindowStyle = WindowStyle.ToolWindow;
            SizeToContent = SizeToContent.WidthAndHeight;
            WindowStartupLocation = WindowStartupLocation.CenterOwner;
            Owner = owner;

            // Create Grid with three auto-sized rows and columns.
            Grid grid = new Grid();
            Content = grid;

            for (int i = 0; i < 3; i++)
            {
                RowDefinition rowdef = new RowDefinition();
                rowdef.Height = GridLength.Auto;
                grid.RowDefinitions.Add(rowdef);

                ColumnDefinition coldef = new ColumnDefinition();
                coldef.Width = GridLength.Auto;
                grid.ColumnDefinitions.Add(coldef);
            }

            // Find what: Label and TextBox.
            Label lbl = new Label();
            lbl.Content = "Fi_nd what:";
            lbl.VerticalAlignment = VerticalAlignment.Center;
            lbl.Margin = new Thickness(12);
            grid.Children.Add(lbl);
            Grid.SetRow(lbl, 0);
            Grid.SetColumn(lbl, 0);

            txtboxFind = new TextBox();
            txtboxFind.Margin = new Thickness(12);
            txtboxFind.TextChanged += FindTextBoxOnTextChanged;
            grid.Children.Add(txtboxFind);
            Grid.SetRow(txtboxFind, 0);
            Grid.SetColumn(txtboxFind, 1);

            // Replace with: Label and TextBox.
            lblReplace = new Label();
            lblReplace.Content = "Re_place with:";
            lblReplace.VerticalAlignment = VerticalAlignment.Center;
            lblReplace.Margin = new Thickness(12);
            grid.Children.Add(lblReplace);
            Grid.SetRow(lblReplace, 1);
            Grid.SetColumn(lblReplace, 0);

            txtboxReplace = new TextBox();
            txtboxReplace.Margin = new Thickness(12);
            grid.Children.Add(txtboxReplace);
            Grid.SetRow(txtboxReplace, 1);
            Grid.SetColumn(txtboxReplace, 1);

            // Match Case CheckBox.
            checkMatch = new CheckBox();
            checkMatch.Content = "Match _case";
            checkMatch.VerticalAlignment = VerticalAlignment.Center;
            checkMatch.Margin = new Thickness(12);
            grid.Children.Add(checkMatch);
            Grid.SetRow(checkMatch, 2);
            Grid.SetColumn(checkMatch, 0);

            // Direction GroupBox and two RadioButtons.
            groupDirection = new GroupBox();
            groupDirection.Header = "Direction";
            groupDirection.Margin = new Thickness(12);
            groupDirection.HorizontalAlignment = HorizontalAlignment.Left;
            grid.Children.Add(groupDirection);
            Grid.SetRow(groupDirection, 2);
            Grid.SetColumn(groupDirection, 1);

            StackPanel stack = new StackPanel();
            stack.Orientation = Orientation.Horizontal;
            groupDirection.Content = stack;

            radioUp = new RadioButton();
            radioUp.Content = "_Up";
            radioUp.Margin = new Thickness(6);
            stack.Children.Add(radioUp);

            radioDown = new RadioButton();
            radioDown.Content = "_Down";
            radioDown.Margin = new Thickness(6);
            stack.Children.Add(radioDown);

            // Create StackPanel for the buttons.
            stack = new StackPanel();
            stack.Margin = new Thickness(6);
            grid.Children.Add(stack);

            Grid.SetRow(stack, 0);
            Grid.SetColumn(stack, 2);
            Grid.SetRowSpan(stack, 3);

            // Four buttons.
            btnFind = new Button();
            btnFind.Content = "_Find Next";
            btnFind.Margin = new Thickness(6);
            btnFind.IsDefault = true;
            btnFind.Click += FindNextOnClick;
            stack.Children.Add(btnFind);

            btnReplace = new Button();
            btnReplace.Content = "_Replace";
            btnReplace.Margin = new Thickness(6);
            btnReplace.Click += ReplaceOnClick;
            stack.Children.Add(btnReplace);

            btnAll = new Button();
            btnAll.Content = "Replace _All";
            btnAll.Margin = new Thickness(6);
            btnAll.Click += ReplaceAllOnClick;
            stack.Children.Add(btnAll);

            Button btn = new Button();
            btn.Content = "Cancel";
            btn.Margin = new Thickness(6);
            btn.IsCancel = true;
            btn.Click += CancelOnClick;
            stack.Children.Add(btn);

            txtboxFind.Focus();
        }
        // Enable the first three buttons only if there's some text to find.
        void FindTextBoxOnTextChanged(object sender, TextChangedEventArgs args)
        {
            TextBox txtbox = args.Source as TextBox;
            btnFind.IsEnabled =
            btnReplace.IsEnabled = 
            btnAll.IsEnabled = (txtbox.Text.Length > 0);
        }
        // The FindNextOnClick method calls the OnFindNext method,
        //      which fires the FindNext event.
        void FindNextOnClick(object sender, RoutedEventArgs args)
        {
            OnFindNext(new EventArgs());
        }
        protected virtual void OnFindNext(EventArgs args)
        {
            if (FindNext != null)
                FindNext(this, args);
        }
        // The ReplaceOnClick method calls the OnReplace method,
        //      which fires the Replace event.
        void ReplaceOnClick(object sender, RoutedEventArgs args)
        {
            OnReplace(new EventArgs());
        }
        protected virtual void OnReplace(EventArgs args)
        {
            if (Replace != null)
                Replace(this, args);
        }
        // The ReplaceAllOnClick method calls the OnReplaceAll method,
        //      which fires the ReplaceAll event.
        void ReplaceAllOnClick(object sender, RoutedEventArgs args)
        {
            OnReplaceAll(new EventArgs());
        }
        protected virtual void OnReplaceAll(EventArgs args)
        {
            if (ReplaceAll != null)
                ReplaceAll(this, args);
        }
        // The Cancel button just closes the dialog box.
        void CancelOnClick(object sender, RoutedEventArgs args)
        {
            Close();
        }
    }
}
