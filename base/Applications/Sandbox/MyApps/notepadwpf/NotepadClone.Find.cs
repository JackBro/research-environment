//--------------------------------------------------
// NotepadClone.Find.cs (c) 2006 by Charles Petzold
//--------------------------------------------------
using System;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;

namespace Petzold.NotepadClone
{
    public partial class NotepadClone
    {
        string strFindWhat = "", strReplaceWith = "";
        StringComparison strcomp = StringComparison.OrdinalIgnoreCase;
        Direction dirFind = Direction.Down;

        void AddFindMenuItems(MenuItem itemEdit)
        {
            // Find menu item.
            MenuItem itemFind = new MenuItem();
            itemFind.Header = "_Find...";
            itemFind.Command = ApplicationCommands.Find;
            itemEdit.Items.Add(itemFind);
            CommandBindings.Add(new CommandBinding(
                ApplicationCommands.Find, FindOnExecute, FindCanExecute));

            // The Find Next item requires a custom RoutedUICommand.
            InputGestureCollection coll = new InputGestureCollection();
            coll.Add(new KeyGesture(Key.F3));
            RoutedUICommand commFindNext = 
                new RoutedUICommand("Find _Next", "FindNext", GetType(), coll);

            MenuItem itemNext = new MenuItem();
            itemNext.Command = commFindNext;
            itemEdit.Items.Add(itemNext);
            CommandBindings.Add(
                new CommandBinding(commFindNext, FindNextOnExecute, 
                                                 FindNextCanExecute));

            MenuItem itemReplace = new MenuItem();
            itemReplace.Header = "_Replace...";
            itemReplace.Command = ApplicationCommands.Replace;
            itemEdit.Items.Add(itemReplace);
            CommandBindings.Add(new CommandBinding(
                ApplicationCommands.Replace, ReplaceOnExecute, FindCanExecute));
        }
        // CanExecute method for Find and Replace.
        void FindCanExecute(object sender, CanExecuteRoutedEventArgs args)
        {
            args.CanExecute = (txtbox.Text.Length > 0 && OwnedWindows.Count == 0);
        }
        void FindNextCanExecute(object sender, CanExecuteRoutedEventArgs args)
        {
            args.CanExecute = (txtbox.Text.Length > 0 && strFindWhat.Length > 0);
        }
        // Event handler for Find menu item.
        void FindOnExecute(object sender, ExecutedRoutedEventArgs args)
        {
            // Create dialog box.
            FindDialog dlg = new FindDialog(this);

            // Initialize properties.
            dlg.FindWhat = strFindWhat;
            dlg.MatchCase = strcomp == StringComparison.Ordinal;
            dlg.Direction = dirFind;

            // Install event handler and show dialog.
            dlg.FindNext += FindDialogOnFindNext;
            dlg.Show();
        }
        // Event handler for Find Next menu item.
        // F3 key invokes dialog box if there's no string to find yet.
        void FindNextOnExecute(object sender, ExecutedRoutedEventArgs args)
        {
            if (strFindWhat == null || strFindWhat.Length == 0)
                FindOnExecute(sender, args);
            else
                FindNext();
        }
        // Event handler for Replace menu item.
        void ReplaceOnExecute(object sender, ExecutedRoutedEventArgs args)
        {
            ReplaceDialog dlg = new ReplaceDialog(this);

            dlg.FindWhat = strFindWhat;
            dlg.ReplaceWith = strReplaceWith;
            dlg.MatchCase = strcomp == StringComparison.Ordinal;
            dlg.Direction = dirFind;

            // Install event handlers.
            dlg.FindNext += FindDialogOnFindNext;
            dlg.Replace += ReplaceDialogOnReplace;
            dlg.ReplaceAll += ReplaceDialogOnReplaceAll;

            dlg.Show();
        }

        // Event handler installed for Find/Replace dialog box "Find Next" button.
        void FindDialogOnFindNext(object sender, EventArgs args)
        {
            FindReplaceDialog dlg = sender as FindReplaceDialog;

            // Get properties from dialog box.
            strFindWhat = dlg.FindWhat;
            strcomp = dlg.MatchCase ? StringComparison.Ordinal : 
                                StringComparison.OrdinalIgnoreCase;
            dirFind = dlg.Direction;

            // Call FindNext to do the actual find.
            FindNext();
        }

        // Event handler installed for Replace dialog box "Replace" button.
        void ReplaceDialogOnReplace(object sender, EventArgs args)
        {
            ReplaceDialog dlg = sender as ReplaceDialog;

            // Get properties from dialog box.
            strFindWhat = dlg.FindWhat;
            strReplaceWith = dlg.ReplaceWith;
            strcomp = dlg.MatchCase ? StringComparison.Ordinal :
                                StringComparison.OrdinalIgnoreCase;

            if (strFindWhat.Equals(txtbox.SelectedText, strcomp))
                txtbox.SelectedText = strReplaceWith;

            FindNext();
        }

        // Event handler installed for Replace dialog box "Replace All" button.
        void ReplaceDialogOnReplaceAll(object sender, EventArgs args)
        {
            ReplaceDialog dlg = sender as ReplaceDialog;
            string str = txtbox.Text;
            strFindWhat = dlg.FindWhat;
            strReplaceWith = dlg.ReplaceWith;
            strcomp = dlg.MatchCase ? StringComparison.Ordinal :
                                StringComparison.OrdinalIgnoreCase;
            int index = 0;

            while (index + strFindWhat.Length < str.Length)
            {
                index = str.IndexOf(strFindWhat, index, strcomp);

                if (index != -1)
                {
                    str = str.Remove(index, strFindWhat.Length);
                    str = str.Insert(index, strReplaceWith);
                    index += strReplaceWith.Length;
                }
                else
                    break;
            }
            txtbox.Text = str;
        }

        // General FindNext method. 
        void FindNext()
        {
            int indexStart, indexFind;

            // The starting position of the search and the direction of the search
            //      are determined by the dirFind variable. 
            if (dirFind == Direction.Down)
            {
                indexStart = txtbox.SelectionStart + txtbox.SelectionLength;
                indexFind = txtbox.Text.IndexOf(strFindWhat, indexStart, strcomp);
            }
            else
            {
                indexStart = txtbox.SelectionStart;
                indexFind = txtbox.Text.LastIndexOf(strFindWhat, indexStart, strcomp);
            }

            // If IndexOf (or LastIndexOf) does not return -1, select the found text.
            // Otherwise, display a message box.
            if (indexFind != -1)
            {
                txtbox.Select(indexFind, strFindWhat.Length);
                txtbox.Focus();
            }
            else
                MessageBox.Show("Cannot find \"" + strFindWhat + "\"", Title,
                                MessageBoxButton.OK, MessageBoxImage.Information);
        }
    }
}
