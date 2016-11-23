using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using System.IO;

namespace WindowsApplication1
{
    public partial class Form1 : Form, WindowsApplication1.IForm1
    {
        public Form1()
        {
            InitializeComponent();
        }

        string fname = "";
        private System.Drawing.Printing.PrintDocument docToPrint = new System.Drawing.Printing.PrintDocument();
        private ToolStripMenuItem wordWrapToolStripMenuItem;
        

        public ToolStripMenuItem WordWrapToolStripMenuItem
        {
            get { return wordWrapToolStripMenuItem; }
            set { wordWrapToolStripMenuItem = value; }
        }
        private void saveToolStripMenuItem_Click(object sender, EventArgs e)
        {
            if (fname == "")
            {
                saveFileDialog1.Filter = "Text Files|*.txt";
                DialogResult result = saveFileDialog1.ShowDialog();
                if (result == DialogResult.Cancel)
                {
                    return;
                }
                fname = saveFileDialog1.FileName;
                StreamWriter s = new StreamWriter(fname);
                s.WriteLine(richTextBox1.Text);
                s.Flush();
                s.Close();
            }
        }

        private void openCtrlOToolStripMenuItem_Click(object sender, EventArgs e)
        {
            try
            {
                DialogResult res = openFileDialog1.ShowDialog();
                if (res == DialogResult.OK)
                {
                    string str = openFileDialog1.FileName, str2;
                    FileStream fS = new FileStream(str, FileMode.Open, FileAccess.Read);
                    StreamReader sr = new StreamReader(fS);
                    str2 = sr.ReadToEnd();
                    richTextBox1.Text = str2;
                    sr.Close();
                    fS.Close();
                }
                else
                {
                    MessageBox.Show(" Not Opened File");
                }
            }
            catch (Exception ee)
            {
                Console.WriteLine(ee.Message);
            }
        }

        private void newToolStripMenuItem_Click(object sender, EventArgs e)
        {
            if (richTextBox1.Text != "")
            {
                DialogResult click = MessageBox.Show("The text in the Untitled has changed.\n\n Do you want to save the changes?", " My Notepad", MessageBoxButtons.YesNoCancel, MessageBoxIcon.Exclamation, MessageBoxDefaultButton.Button1);
                if (click == DialogResult.Yes)
                {
                    if (fname == "")
                    {
                        saveFileDialog1.Filter = "Text Files|*.textBox1";
                        DialogResult result = saveFileDialog1.ShowDialog();
                        if (result == DialogResult.Cancel)
                        {
                            return;
                        }
                        fname = saveFileDialog1.FileName;
                        // MessageBox.Show(fname);
                    }
                    StreamWriter write = new StreamWriter(fname);
                    write.WriteLine(richTextBox1.Text);
                    write.Flush();
                    //  textBox1.Text = "";
                    write.Close();

                    richTextBox1.Text = "";
                    fname = "";
                    // bool flag = false;
                }
                if (click == DialogResult.No)
                {
                    richTextBox1.Text = "";
                    fname = "";
                }
            }
            else
            {
                richTextBox1.Text = "";
                fname = "";
            }
        }

        private void saveAsToolStripMenuItem_Click(object sender, EventArgs e)
        {
            saveFileDialog1.Filter = "Text Files|*.txt";
            saveFileDialog1.ShowDialog();
            fname = saveFileDialog1.FileName;
            if (fname == "")
            {
                saveFileDialog1.Filter = "Text Files|*.txt";
                DialogResult result = saveFileDialog1.ShowDialog();
                if (result == DialogResult.Cancel)
                {
                    return;
                }
                fname = saveFileDialog1.FileName;
            }
            StreamWriter s = new StreamWriter(fname);
            s.WriteLine(richTextBox1.Text);
            s.Flush();
            s.Close();
        }

        private void pageSetupToolStripMenuItem_Click(object sender, EventArgs e)
        {
            pageSetupDialog1.PageSettings = new System.Drawing.Printing.PageSettings();
            pageSetupDialog1.PrinterSettings = new System.Drawing.Printing.PrinterSettings();
            pageSetupDialog1.ShowNetwork = false;
            DialogResult result = pageSetupDialog1.ShowDialog();
            if (result == DialogResult.OK)
            {
                object[] results = new object[]{
                    pageSetupDialog1.PageSettings.Margins,
                    pageSetupDialog1.PageSettings.PaperSize,
                    pageSetupDialog1.PageSettings.Landscape,
                    pageSetupDialog1.PrinterSettings.PrinterName,
                    pageSetupDialog1.PrinterSettings.PrintRange};
                //richTextBox1.Text.LastIndexOf(results);
            }
        }

        private void printToolStripMenuItem_Click(object sender, EventArgs e)
        {

            printDialog1.AllowSomePages = true;
            printDialog1.ShowHelp = true;
            printDialog1.Document = docToPrint;
            DialogResult result = printDialog1.ShowDialog();
            if (result == DialogResult.OK)
            {
                docToPrint.Print();
            }

        }
        private void document_PrintPage(object sender,
          System.Drawing.Printing.PrintPageEventArgs e)
        {
            string text = "In document_PrintPage method.";
            System.Drawing.Font printFont = new System.Drawing.Font
                ("Arial", 35, System.Drawing.FontStyle.Regular);
            e.Graphics.DrawString(text, printFont,
                System.Drawing.Brushes.Black, 10, 10);
        }

        private void exitToolStripMenuItem_Click(object sender, EventArgs e)
        {
            if (richTextBox1.Text != "")
        {
            DialogResult click = MessageBox.Show("The text in the Untitled has changed.\n\n Do you want to save the changes?", " My Notepad", MessageBoxButtons.YesNoCancel, MessageBoxIcon.Exclamation, MessageBoxDefaultButton.Button1);
            if (click == DialogResult.Yes)
            {
                if (fname == "")
                {
                    saveFileDialog1.Filter = "Text Files|*.txt";
                    DialogResult result = saveFileDialog1.ShowDialog();
                    if (result == DialogResult.Cancel)
                    {
                        return;
                    }
                    fname = saveFileDialog1.FileName;
                    // MessageBox.Show(fname);
                }
                StreamWriter write = new StreamWriter(fname);
                write.WriteLine(richTextBox1.Text);
                write.Flush();
                write.Close();
                Application.Exit();
            }
            if (click == DialogResult.No)
            {
                Application.Exit();
            }
            }
        }

        private void undoToolStripMenuItem_Click(object sender, EventArgs e)
        {
            if (richTextBox1.CanUndo)
                richTextBox1.Undo();
        }

        private void cutToolStripMenuItem_Click(object sender, EventArgs e)
        {
            if (richTextBox1.SelectedText != "")
            {
                cutToolStripMenuItem.Enabled = true;
                richTextBox1.Cut();
            }
        }

        private void copyToolStripMenuItem_Click(object sender, EventArgs e)
        {
            if (richTextBox1.SelectedText != "")
            {
                richTextBox1.Copy();
            }
        }

        private void pToolStripMenuItem_Click(object sender, EventArgs e)
        {
            richTextBox1.Paste();
        }

        private void deleteToolStripMenuItem_Click(object sender, EventArgs e)
        {
            if (richTextBox1.Text != "")
            {
                richTextBox1.SelectedText = "";
            }
        }

        private void findToolStripMenuItem_Click(object sender, EventArgs e)
        {
            //FindTab tab = new FindTab(this);
            //tab.Show();
        }

        private void selectAllToolStripMenuItem_Click(object sender, EventArgs e)
        {
            if (richTextBox1.Text != "")
            {
                richTextBox1.SelectAll();
            }
            else
            {
                MessageBox.Show("No Text is there");
            }
        }

        private void timeDateToolStripMenuItem_Click(object sender, EventArgs e)
        {
            string timeDate;
            timeDate = DateTime.Now.ToShortTimeString() + " " +
            DateTime.Now.ToShortDateString();
            int newSelectionStart = richTextBox1.SelectionStart + timeDate.Length;
            richTextBox1.Text = richTextBox1.Text.Insert(richTextBox1.SelectionStart, timeDate);
            richTextBox1.SelectionStart = newSelectionStart;
        }

        private void fontToolStripMenuItem_Click(object sender, EventArgs e)
        {
            fontDialog1.ShowColor = true;
            fontDialog1.ShowDialog();
            richTextBox1.Font = fontDialog1.Font;
            richTextBox1.ForeColor = fontDialog1.Color;
        }

        private void wordToolStripMenuItem_Click(object sender, EventArgs e)
        {

        }
    }
}
