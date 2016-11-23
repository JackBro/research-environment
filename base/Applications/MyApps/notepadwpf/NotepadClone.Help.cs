//--------------------------------------------------
// NotepadClone.Help.cs (c) 2006 by Charles Petzold
//--------------------------------------------------
using System;

using System.Reflection;

using System.Windows;
using System.Windows.Controls;

namespace Petzold.NotepadClone
{
    public partial class NotepadClone
    {
        void AddHelpMenu(Menu menu)
        {
            MenuItem itemHelp = new MenuItem();
            itemHelp.Header = "_Help";
            itemHelp.SubmenuOpened += ViewOnOpen;
            menu.Items.Add(itemHelp);

            MenuItem itemAbout = new MenuItem();
            itemAbout.Header = "_About " + strAppTitle + "...";
            itemAbout.Click += AboutOnClick;
            itemHelp.Items.Add(itemAbout);
        }
        void AboutOnClick(object sender, RoutedEventArgs args)
        {
            AboutDialog dlg = new AboutDialog(this);
            dlg.ShowDialog();
        }
    }
}