//-------------------------------------------------
// PrintWithMargins.cs (c) 2006 by Charles Petzold
//-------------------------------------------------
using System;
using System.Globalization;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using System.Windows.Media;
using System.Printing;

namespace Petzold.PrintWithMargins
{
    public class PrintWithMargins : Window
    {
        // Private fields to save information from PrintDialog.
        PrintQueue prnqueue;        
        PrintTicket prntkt;
        Thickness marginPage = new Thickness(96);

        [STAThread]
        public static void Main()
        {
            Application app = new Application();
            app.Run(new PrintWithMargins());
        }
        public PrintWithMargins()
        {
            Title = "Print with Margins";
            FontSize = 24;

            // Create StackPanel as content of Window.
            StackPanel stack = new StackPanel();
            Content = stack;

            // Create Button for Page Setup.
            Button btn = new Button();
            btn.Content = "Page Set_up...";
            btn.HorizontalAlignment = HorizontalAlignment.Center;
            btn.Margin = new Thickness(24);
            btn.Click += SetupOnClick;
            stack.Children.Add(btn);

            // Create Print button.
            btn = new Button();
            btn.Content = "_Print...";
            btn.HorizontalAlignment = HorizontalAlignment.Center;
            btn.Margin = new Thickness(24);
            btn.Click += PrintOnClick;
            stack.Children.Add(btn);
        }
        // Page Setup button: Invoke PageMarginsDialog.
        void SetupOnClick(object sender, RoutedEventArgs args)
        {
            // Create dialog and initialize PageMargins property.
            PageMarginsDialog dlg = new PageMarginsDialog();
            dlg.Owner = this;
            dlg.PageMargins = marginPage;

            if (dlg.ShowDialog().GetValueOrDefault())
            {
                // Save page margins from dialog box.
                marginPage = dlg.PageMargins;
            }
        }
        // Print button: Invoke PrintDialog.
        void PrintOnClick(object sender, RoutedEventArgs args)
        {
            PrintDialog dlg = new PrintDialog();

            // Set PrintQueue and PrintTicket from fields.
            if (prnqueue != null)
                dlg.PrintQueue = prnqueue;

            if (prntkt != null)
                dlg.PrintTicket = prntkt;

            if (dlg.ShowDialog().GetValueOrDefault())
            {
                // Save PrintQueue and PrintTicket from dialog box.
                prnqueue = dlg.PrintQueue;
                prntkt = dlg.PrintTicket;

                // Create DrawingVisual and open DrawingContext.
                DrawingVisual vis = new DrawingVisual();
                DrawingContext dc = vis.RenderOpen();
                Pen pn = new Pen(Brushes.Black, 1);

                // Rectangle describes page minus margins.
                Rect rectPage = new Rect(marginPage.Left, marginPage.Top,
                                    dlg.PrintableAreaWidth - 
                                        (marginPage.Left + marginPage.Right),
                                    dlg.PrintableAreaHeight - 
                                        (marginPage.Top + marginPage.Bottom));

                // Draw rectangle to reflect user's margins.
                dc.DrawRectangle(null, pn, rectPage);

                // Create formatted text object showing PrintableArea properties.
                FormattedText formtxt = new FormattedText(
                    String.Format("Hello, Printer! {0} x {1}", 
                                  dlg.PrintableAreaWidth / 96, 
                                  dlg.PrintableAreaHeight / 96),
                    CultureInfo.CurrentCulture,
                    FlowDirection.LeftToRight,
                    new Typeface(new FontFamily("Times New Roman"), 
                                 FontStyles.Italic, FontWeights.Normal, 
                                 FontStretches.Normal), 
                    48, Brushes.Black);

                // Get physical size of formatted text string.
                Size sizeText = new Size(formtxt.Width, formtxt.Height);

                // Calculate point to center text within margins.
                Point ptText = 
                    new Point(rectPage.Left + 
                                    (rectPage.Width - formtxt.Width) / 2,
                              rectPage.Top + 
                                    (rectPage.Height - formtxt.Height) / 2);

                // Draw text and surrounding rectangle.
                dc.DrawText(formtxt, ptText);
                dc.DrawRectangle(null, pn, new Rect(ptText, sizeText));

                // Close DrawingContext.
                dc.Close();

                // Finally, print the page(s).
                dlg.PrintVisual(vis, Title);
            }
        }
    }
}
