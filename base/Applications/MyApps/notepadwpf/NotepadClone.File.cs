//--------------------------------------------------
// NotepadClone.File.cs (c) 2006 by Charles Petzold
//--------------------------------------------------
using Microsoft.Win32;
using System;
using System.IO;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using System.Windows.Media;
using System.Printing;

namespace Petzold.NotepadClone
{
    public partial class NotepadClone : Window
    {
        // Filter for File Open and Save dialog boxes.
        protected string strFilter =
                        "Text Documents(*.txt)|*.txt|All Files(*.*)|*.*";

        void AddFileMenu(Menu menu)
        {
            // Create top-level File item.
            MenuItem itemFile = new MenuItem();
            itemFile.Header = "_File";
            menu.Items.Add(itemFile);

            // New menu item.
            MenuItem itemNew = new MenuItem();
            itemNew.Header = "_New";
            itemNew.Command = ApplicationCommands.New;
            itemFile.Items.Add(itemNew);
            CommandBindings.Add(
                new CommandBinding(ApplicationCommands.New, NewOnExecute));

            // Open menu item.
            MenuItem itemOpen = new MenuItem();
            itemOpen.Header = "_Open...";
            itemOpen.Command = ApplicationCommands.Open;
            itemFile.Items.Add(itemOpen);
            CommandBindings.Add(
                new CommandBinding(ApplicationCommands.Open, OpenOnExecute));

            // Save menu item.
            MenuItem itemSave = new MenuItem();
            itemSave.Header = "_Save";
            itemSave.Command = ApplicationCommands.Save;
            itemFile.Items.Add(itemSave);
            CommandBindings.Add(
                new CommandBinding(ApplicationCommands.Save, SaveOnExecute));

            // Save As menu item.
            MenuItem itemSaveAs = new MenuItem();
            itemSaveAs.Header = "Save _As...";
            itemSaveAs.Command = ApplicationCommands.SaveAs;
            itemFile.Items.Add(itemSaveAs);
            CommandBindings.Add(
                new CommandBinding(ApplicationCommands.SaveAs, SaveAsOnExecute));

            // Separators and printing items.
            itemFile.Items.Add(new Separator());
            AddPrintMenuItems(itemFile);
            itemFile.Items.Add(new Separator());

            // Exit menu item.
            MenuItem itemExit = new MenuItem();
            itemExit.Header = "E_xit";
            itemExit.Click += ExitOnClick;
            itemFile.Items.Add(itemExit);
        }
        // File New command: Start with empty TextBox.
        protected virtual void NewOnExecute(object sender, 
                                            ExecutedRoutedEventArgs args)
        {
            if (!OkToTrash())
                return;

            txtbox.Text = "";
            strLoadedFile = null;
            isFileDirty = false;
            UpdateTitle();
        }
        // File Open command: Display dialog box and load file.
        void OpenOnExecute(object sender, ExecutedRoutedEventArgs args)
        {
            if (!OkToTrash())
                return;

            OpenFileDialog dlg = new OpenFileDialog();
            dlg.Filter = strFilter;

            if ((bool)dlg.ShowDialog(this))
            {
                LoadFile(dlg.FileName);
            }
        }
        // File Save command: Possibly execute SaveAsExecute.
        void SaveOnExecute(object sender, ExecutedRoutedEventArgs args)
        {
            if (strLoadedFile == null || strLoadedFile.Length == 0)
                DisplaySaveDialog("");
            else
                SaveFile(strLoadedFile);
        }
        // File Save As command; display dialog box and save file.
        void SaveAsOnExecute(object sender, ExecutedRoutedEventArgs args)
        {
            DisplaySaveDialog(strLoadedFile);
        }
        // Display Save dialog box and return true if file is saved.
        bool DisplaySaveDialog(string strFileName)
        {
            SaveFileDialog dlg = new SaveFileDialog();
            dlg.Filter = strFilter;
            dlg.FileName = strFileName;

            if ((bool)dlg.ShowDialog(this))
            {
                SaveFile(dlg.FileName);
                return true;
            }
            return false;           // for OkToTrash.
        }
        // File Exit command: Just close the window.
        void ExitOnClick(object sender, RoutedEventArgs args)
        {
            Close();
        }
        // OkToTrash returns true if the TextBox contents need not be saved.
        bool OkToTrash()
        {
            if (!isFileDirty)
                return true;

            MessageBoxResult result =
                MessageBox.Show("The text in the file " + strLoadedFile + 
                                " has changed\n\n" +
                                "Do you want to save the changes?", 
                                strAppTitle,
                                MessageBoxButton.YesNoCancel, 
                                MessageBoxImage.Question,
                                MessageBoxResult.Yes);

            if (result == MessageBoxResult.Cancel)
                return false;

            else if (result == MessageBoxResult.No)
                return true;

            else // result == MessageBoxResult.Yes
            {
                if (strLoadedFile != null && strLoadedFile.Length > 0)
                    return SaveFile(strLoadedFile);

                return DisplaySaveDialog("");
            }
        }
        // LoadFile method possibly displays message box if error. 
        void LoadFile(string strFileName)
        {
            try
            {
                txtbox.Text = File.ReadAllText(strFileName);
            }
            catch (Exception exc)
            {
                MessageBox.Show("Error on File Open: " + exc.Message, strAppTitle,
                                MessageBoxButton.OK, MessageBoxImage.Asterisk);
                return;
            }
            strLoadedFile = strFileName;
            UpdateTitle();
            txtbox.SelectionStart = 0;
            txtbox.SelectionLength = 0;
            isFileDirty = false;
        }
        // SaveFile method possibly displays message box if error.
        bool SaveFile(string strFileName)
        {
            try
            {
                File.WriteAllText(strFileName, txtbox.Text);
            }
            catch (Exception exc)
            {
                MessageBox.Show("Error on File Save" + exc.Message, strAppTitle,
                                MessageBoxButton.OK, MessageBoxImage.Asterisk);
                return false;
            }
            strLoadedFile = strFileName;
            UpdateTitle();
            isFileDirty = false;
            return true;
        }
    }
}
