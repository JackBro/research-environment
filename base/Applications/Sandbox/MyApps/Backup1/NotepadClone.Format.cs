//----------------------------------------------------
// NotepadClone.Format.cs (c) 2006 by Charles Petzold
//----------------------------------------------------
using Petzold.ChooseFont;
using System;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Media;

namespace Petzold.NotepadClone
{
    public partial class NotepadClone
    {
        void AddFormatMenu(Menu menu)
        {
            // Create top-level Format item.
            MenuItem itemFormat = new MenuItem();
            itemFormat.Header = "F_ormat";
            menu.Items.Add(itemFormat);

            // Create Word Wrap menu item.
            WordWrapMenuItem itemWrap = new WordWrapMenuItem();
            itemFormat.Items.Add(itemWrap);

            // Bind item to TextWrapping property of TextBox.
            Binding bind = new Binding();
            bind.Path = new PropertyPath(TextBox.TextWrappingProperty);
            bind.Source = txtbox;
            bind.Mode = BindingMode.TwoWay;
            itemWrap.SetBinding(WordWrapMenuItem.WordWrapProperty, bind);

            // Create Font menu item.
            MenuItem itemFont = new MenuItem();
            itemFont.Header = "_Font...";
            itemFont.Click += FontOnClick;
            itemFormat.Items.Add(itemFont);
        }
        // Font item event handler.
        void FontOnClick(object sender, RoutedEventArgs args)
        {
            FontDialog dlg = new FontDialog();
            dlg.Owner = this;

            // Set TextBox properties in FontDialog.
            dlg.Typeface = new Typeface(txtbox.FontFamily, txtbox.FontStyle, 
                                        txtbox.FontWeight, txtbox.FontStretch);
            dlg.FaceSize = txtbox.FontSize;

            if (dlg.ShowDialog().GetValueOrDefault())
            {
                // Set FontDialog properties in TextBox.
                txtbox.FontFamily = dlg.Typeface.FontFamily;
                txtbox.FontSize = dlg.FaceSize;
                txtbox.FontStyle = dlg.Typeface.Style;
                txtbox.FontWeight = dlg.Typeface.Weight;
                txtbox.FontStretch = dlg.Typeface.Stretch;
            }
        }
    }
}
