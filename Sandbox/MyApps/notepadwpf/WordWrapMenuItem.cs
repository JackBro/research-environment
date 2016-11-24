//-------------------------------------------------
// WordWrapMenuItem.cs (c) 2006 by Charles Petzold
//-------------------------------------------------
using System;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Controls.Primitives;

namespace Petzold.NotepadClone
{
    public class WordWrapMenuItem : MenuItem
    {
        // Register WordWrap dependency property.
        public static DependencyProperty WordWrapProperty =
                DependencyProperty.Register("WordWrap", typeof(TextWrapping),
                                            typeof(WordWrapMenuItem));
        // Define WordWrap property.
        public TextWrapping WordWrap
        {
            set { SetValue(WordWrapProperty, value); }
            get { return (TextWrapping)GetValue(WordWrapProperty); }
        }
        // Constructor creates Word Wrap menu item.
        public WordWrapMenuItem()
        {
            Header = "_Word Wrap";

            MenuItem item = new MenuItem();
            item.Header = "_No Wrap";
            item.Tag = TextWrapping.NoWrap;
            item.Click += MenuItemOnClick;
            Items.Add(item);

            item = new MenuItem();
            item.Header = "_Wrap";
            item.Tag = TextWrapping.Wrap;
            item.Click += MenuItemOnClick;
            Items.Add(item);

            item = new MenuItem();
            item.Header = "Wrap with _Overflow";
            item.Tag = TextWrapping.WrapWithOverflow;
            item.Click += MenuItemOnClick;
            Items.Add(item);
        }
        // Set checked item from current WordWrap property.
        protected override void OnSubmenuOpened(RoutedEventArgs args)
        {
            base.OnSubmenuOpened(args);

            foreach (MenuItem item in Items)
                item.IsChecked = ((TextWrapping)item.Tag == WordWrap);
        }
        // Set WordWrap property from clicked item.
        void MenuItemOnClick(object sender, RoutedEventArgs args)
        {
            WordWrap = (TextWrapping)(args.Source as MenuItem).Tag;
        }
    }
}
