//--------------------------------------------------
// NotepadClone.View.cs (c) 2006 by Charles Petzold
//--------------------------------------------------
using System;
using System.Windows;
using System.Windows.Controls;

namespace Petzold.NotepadClone
{
    public partial class NotepadClone
    {
        MenuItem itemStatus;

        void AddViewMenu(Menu menu)
        {
            // Create top-level View item.
            MenuItem itemView = new MenuItem();
            itemView.Header = "_View";
            itemView.SubmenuOpened += ViewOnOpen;
            menu.Items.Add(itemView);

            // Create Status Bar item on View menu.
            itemStatus = new MenuItem();
            itemStatus.Header = "_Status Bar";
            itemStatus.IsCheckable = true;
            itemStatus.Checked += StatusOnCheck;
            itemStatus.Unchecked += StatusOnCheck;
            itemView.Items.Add(itemStatus);
        }
        void ViewOnOpen(object sender, RoutedEventArgs args)
        {
            itemStatus.IsChecked = (status.Visibility == Visibility.Visible);
        }
        void StatusOnCheck(object sender, RoutedEventArgs args)
        {
            MenuItem item = sender as MenuItem;
            status.Visibility = 
                item.IsChecked ? Visibility.Visible : Visibility.Collapsed;
        }
    }
}
