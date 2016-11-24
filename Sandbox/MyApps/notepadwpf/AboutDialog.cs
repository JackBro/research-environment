//--------------------------------------------
// AboutDialog.cs (c) 2006 by Charles Petzold
//--------------------------------------------
using System;
using System.Diagnostics;       // for Process class
using System.Reflection;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Documents;
using System.Windows.Media;

namespace Petzold.NotepadClone
{
    class AboutDialog : Window
    {
        public AboutDialog(Window owner)
        {
            // Get attributes from assembly.
            // Get this executing assembly to access attributes.
            Assembly asmbly = Assembly.GetExecutingAssembly();

            // Get the AssemblyTitle attribute for the program name.
            AssemblyTitleAttribute title = 
                (AssemblyTitleAttribute)asmbly.GetCustomAttributes(
                    typeof(AssemblyTitleAttribute), false)[0];
            string strTitle = title.Title;

            // Get the AssemblyFileVersion attribute.
            AssemblyFileVersionAttribute version = 
                (AssemblyFileVersionAttribute)asmbly.GetCustomAttributes(
                    typeof(AssemblyFileVersionAttribute), false)[0];
            string strVersion = version.Version.Substring(0, 3);

            // Get the AssemblyCopyright attribute.
            AssemblyCopyrightAttribute copy = 
                (AssemblyCopyrightAttribute)asmbly.GetCustomAttributes(
                    typeof(AssemblyCopyrightAttribute), false)[0];
            string strCopyright = copy.Copyright;

            // Standard window properties for dialog boxes.
            Title = "About " + strTitle;
            ShowInTaskbar = false;
            SizeToContent = SizeToContent.WidthAndHeight;
            ResizeMode = ResizeMode.NoResize;
            Left = owner.Left + 96;
            Top = owner.Top + 96;

            // Create StackPanel as content of window.
            StackPanel stackMain = new StackPanel();
            Content = stackMain;

            // Create TextBlock for program name.
            TextBlock txtblk = new TextBlock();
            txtblk.Text = strTitle + " Version " + strVersion;
            txtblk.FontFamily = new FontFamily("Times New Roman");
            txtblk.FontSize = 32;       // 24 points
            txtblk.FontStyle = FontStyles.Italic;
            txtblk.Margin = new Thickness(24);
            txtblk.HorizontalAlignment = HorizontalAlignment.Center;
            stackMain.Children.Add(txtblk);

            // Create TextBlock for copyright.
            txtblk = new TextBlock();
            txtblk.Text = strCopyright;
            txtblk.FontSize = 20;       // 15 points.
            txtblk.HorizontalAlignment = HorizontalAlignment.Center;
            stackMain.Children.Add(txtblk);

            // Create TextBlock for Web site link.
            Run run = new Run("www.charlespetzold.com");
            Hyperlink link = new Hyperlink(run);
            link.Click += LinkOnClick;
            txtblk = new TextBlock(link);
            txtblk.FontSize = 20;
            txtblk.HorizontalAlignment = HorizontalAlignment.Center;
            stackMain.Children.Add(txtblk);

            // Create OK button.
            Button btn = new Button();
            btn.Content = "OK";
            btn.IsDefault = true;
            btn.IsCancel = true;
            btn.HorizontalAlignment = HorizontalAlignment.Center;
            btn.MinWidth = 48;
            btn.Margin = new Thickness(24);
            btn.Click += OkOnClick;
            stackMain.Children.Add(btn);

            btn.Focus();
        }
        // Event handlers.
        void LinkOnClick(object sender, RoutedEventArgs args)
        {
            Process.Start("http://www.charlespetzold.com");
        }
        void OkOnClick(object sender, RoutedEventArgs args)
        {
            DialogResult = true;
        }
    }
}
