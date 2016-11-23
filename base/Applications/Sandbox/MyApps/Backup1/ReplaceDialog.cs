//----------------------------------------------
// ReplaceDialog.cs (c) 2006 by Charles Petzold
//----------------------------------------------
using System;
using System.Windows;
using System.Windows.Controls;

namespace Petzold.NotepadClone
{
    class ReplaceDialog : FindReplaceDialog
    {
        public ReplaceDialog(Window owner): base(owner)
        {
            Title = "Replace";

            // Make the GroupBox invisible.
//            groupDirection.Visibility = Visibility.Hidden;
        }
    }
}
