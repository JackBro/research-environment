//---------------------------------------------------
// NotepadClone.Print.cs (c) 2006 by Charles Petzold
//---------------------------------------------------
using Petzold.PrintWithMargins;     // for PageMarginsDialog.
using System;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using System.Windows.Media;
using System.Printing;

namespace Petzold.NotepadClone
{
    public partial class NotepadClone : Window
    {
        // Fields for printing.
        PrintQueue prnqueue;
        PrintTicket prntkt;
        Thickness marginPage = new Thickness(96);

        void AddPrintMenuItems(MenuItem itemFile)
        {
            // Page Setup menu item.
            MenuItem itemSetup = new MenuItem();
            itemSetup.Header = "Page Set_up...";
            itemSetup.Click += PageSetupOnClick;
            itemFile.Items.Add(itemSetup);

            // Print menu item.
            MenuItem itemPrint = new MenuItem();
            itemPrint.Header = "_Print...";
            itemPrint.Command = ApplicationCommands.Print;
            itemFile.Items.Add(itemPrint);
            CommandBindings.Add(
                new CommandBinding(ApplicationCommands.Print, PrintOnExecuted));
        }
        void PageSetupOnClick(object sender, RoutedEventArgs args)
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
        void PrintOnExecuted(object sender, ExecutedRoutedEventArgs args)
        {
            PrintDialog dlg = new PrintDialog();

            // Get the PrintQueue and PrintTicket from previous invocations.
            if (prnqueue != null)
                dlg.PrintQueue = prnqueue;

            if (prntkt != null)
                dlg.PrintTicket = prntkt;

            if (dlg.ShowDialog().GetValueOrDefault())
            {
                // Save PrintQueue and PrintTicket from dialog box.
                prnqueue = dlg.PrintQueue;
                prntkt = dlg.PrintTicket;

                // Create a PlainTextDocumentPaginator object.
                PlainTextDocumentPaginator paginator = 
                    new PlainTextDocumentPaginator();

                // Set the paginator properties.
                paginator.PrintTicket = prntkt;
                paginator.Text = txtbox.Text;
                paginator.Header = strLoadedFile;
                paginator.Typeface = 
                    new Typeface(txtbox.FontFamily, txtbox.FontStyle,
                                 txtbox.FontWeight, txtbox.FontStretch);
                paginator.FaceSize = txtbox.FontSize;
                paginator.TextWrapping = txtbox.TextWrapping;
                paginator.Margins = marginPage;
                paginator.PageSize = new Size(dlg.PrintableAreaWidth,
                                              dlg.PrintableAreaHeight);
                // Print the document.
                dlg.PrintDocument(paginator, Title);
            }
        }
    }
}
