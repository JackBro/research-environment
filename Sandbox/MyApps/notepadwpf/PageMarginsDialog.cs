//--------------------------------------------------
// PageMarginsDialog.cs (c) 2006 by Charles Petzold
//--------------------------------------------------
using System;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Controls.Primitives;

namespace Petzold.PrintWithMargins
{
    class PageMarginsDialog : Window
    {
        // Internal enumeration to refer to the paper sides.
        enum Side
        {
            Left, Right, Top, Bottom
        }

        // Four TextBox controls for numeric input.
        TextBox[] txtbox = new TextBox[4];
        Button btnOk;

        // Public property of type Thickness for page margins.
        public Thickness PageMargins
        {
            set
            {
                txtbox[(int)Side.Left].Text = (value.Left / 96).ToString("F3");
                txtbox[(int)Side.Right].Text = 
                                            (value.Right / 96).ToString("F3");
                txtbox[(int)Side.Top].Text = (value.Top / 96).ToString("F3");
                txtbox[(int)Side.Bottom].Text = 
                                            (value.Bottom / 96).ToString("F3");
            }
            get
            {
                return new Thickness(
                            Double.Parse(txtbox[(int)Side.Left].Text) * 96,
                            Double.Parse(txtbox[(int)Side.Top].Text) * 96,
                            Double.Parse(txtbox[(int)Side.Right].Text) * 96,
                            Double.Parse(txtbox[(int)Side.Bottom].Text) * 96);
            }
        }

        // Constructor.
        public PageMarginsDialog()
        {
            // Standard settings for dialog boxes.
            Title = "Page Setup";
            ShowInTaskbar = false;
            WindowStyle = WindowStyle.ToolWindow;
            WindowStartupLocation = WindowStartupLocation.CenterOwner;
            SizeToContent = SizeToContent.WidthAndHeight;
            ResizeMode = ResizeMode.NoResize;

            // Make StackPanel content of Window.
            StackPanel stack = new StackPanel();
            Content = stack;

            // Make GroupBox a child of StackPanel.
            GroupBox grpbox = new GroupBox();
            grpbox.Header = "Margins (inches)";
            grpbox.Margin = new Thickness(12);
            stack.Children.Add(grpbox);

            // Make Grid the content of the GroupBox.
            Grid grid = new Grid();
            grid.Margin = new Thickness(6);
            grpbox.Content = grid;

            // Two rows and four columns.
            for (int i = 0; i < 2; i++)
            {
                RowDefinition rowdef = new RowDefinition();
                rowdef.Height = GridLength.Auto;
                grid.RowDefinitions.Add(rowdef);
            }

            for (int i = 0; i < 4; i++)
            {
                ColumnDefinition coldef = new ColumnDefinition();
                coldef.Width = GridLength.Auto;
                grid.ColumnDefinitions.Add(coldef);
            }

            // Put Label and TextBox controls in Grid.
            for (int i = 0; i < 4; i++)
            {
                Label lbl = new Label();
                lbl.Content = "_" + Enum.GetName(typeof(Side), i) + ":";
                lbl.Margin = new Thickness(6);
                lbl.VerticalAlignment = VerticalAlignment.Center;
                grid.Children.Add(lbl);
                Grid.SetRow(lbl, i / 2);
                Grid.SetColumn(lbl, 2 * (i % 2));

                txtbox[i] = new TextBox();
                txtbox[i].TextChanged += TextBoxOnTextChanged;
                txtbox[i].MinWidth = 48;
                txtbox[i].Margin = new Thickness(6);
                grid.Children.Add(txtbox[i]);
                Grid.SetRow(txtbox[i], i / 2);
                Grid.SetColumn(txtbox[i], 2 * (i % 2) + 1);
            }

            // Use UniformGrid for OK and Cancel buttons.
            UniformGrid unigrid = new UniformGrid();
            unigrid.Rows = 1;
            unigrid.Columns = 2;
            stack.Children.Add(unigrid);

            btnOk = new Button();
            btnOk.Content = "OK";
            btnOk.IsDefault = true;
            btnOk.IsEnabled = false;
            btnOk.MinWidth = 60;
            btnOk.Margin = new Thickness(12);
            btnOk.HorizontalAlignment = HorizontalAlignment.Center;
            btnOk.Click += OkButtonOnClick;
            unigrid.Children.Add(btnOk);

            Button btnCancel = new Button();
            btnCancel.Content = "Cancel";
            btnCancel.IsCancel = true;
            btnCancel.MinWidth = 60;
            btnCancel.Margin = new Thickness(12);
            btnCancel.HorizontalAlignment = HorizontalAlignment.Center;
            unigrid.Children.Add(btnCancel);
        }
        // Enable OK button only if the TextBox controls have numeric values.
        void TextBoxOnTextChanged(object sender, TextChangedEventArgs args)
        {
            double result;

            btnOk.IsEnabled = 
                Double.TryParse(txtbox[(int)Side.Left].Text, out result) &&
                Double.TryParse(txtbox[(int)Side.Right].Text, out result) &&
                Double.TryParse(txtbox[(int)Side.Top].Text, out result) &&
                Double.TryParse(txtbox[(int)Side.Bottom].Text, out result);
        }
        // Dismiss dialog on OK click.
        void OkButtonOnClick(object sender, RoutedEventArgs args)
        {
            DialogResult = true;
        }
    }
}
