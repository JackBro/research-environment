//-----------------------------------------------------------
// PlainTextDocumentPaginator.cs (c) 2006 by Charles Petzold
//-----------------------------------------------------------
using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Printing;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Documents;
using System.Windows.Media;

namespace Petzold.NotepadClone
{
    public class PlainTextDocumentPaginator : DocumentPaginator
    {
        // Private fields, including those associated with public properties.
        char[] charsBreak = new char[] { ' ', '-' };
        string txt = "";
        string txtHeader = null;
        Typeface face = new Typeface("");
        double em = 11;
        Size sizePage = new Size(8.5 * 96, 11 * 96);
        Size sizeMax = new Size(0, 0);
        Thickness margins = new Thickness(96);
        PrintTicket prntkt = new PrintTicket();
        TextWrapping txtwrap = TextWrapping.Wrap;

        // Stores each page as a DocumentPage object.
        List<DocumentPage> listPages;

        // Public properties.
        public string Text
        {
            set { txt = value; }
            get { return txt; }
        }
        public TextWrapping TextWrapping
        {
            set { txtwrap = value; }
            get { return txtwrap; }
        }
        public Thickness Margins
        {
            set { margins = value; }
            get { return margins; }
        }
        public Typeface Typeface
        {
            set { face = value; }
            get { return face; }
        }
        public double FaceSize
        {
            set { em = value; }
            get { return em; }
        }
        public PrintTicket PrintTicket
        {
            set { prntkt = value; }
            get { return prntkt; }
        }
        public string Header
        {
            set { txtHeader = value; }
            get { return txtHeader; }
        }

        // Required overrides.
        public override bool IsPageCountValid
        {
            get 
            {
                if (listPages == null)
                    Format();

                return true; 
            }
        }
        public override int PageCount
        {
            get 
            {
                if (listPages == null)
                    return 0;

                return listPages.Count; 
            }
        }
        public override Size PageSize
        {
            set { sizePage = value; }
            get { return sizePage; }
        }
        public override DocumentPage GetPage(int numPage)
        {
            return listPages[numPage];
        }
        public override IDocumentPaginatorSource Source
        {
            get { return null; }
        }

        // An internal class to indicate if an arrow is to be printed at the 
        //  end of the line of text.
        class PrintLine
        {
            public string String;
            public bool Flag;

            public PrintLine(string str, bool flag)
            {
                String = str;
                Flag = flag;
            }
        }

        // Formats entire document into pages.
        void Format()
        {
            // Store each line of the document as with LineWithFlag object.
            List<PrintLine> listLines = new List<PrintLine>();

            // Use this for some basic calculations.
            FormattedText formtxtSample = GetFormattedText("W");

            // Width of printed line.
            double width = PageSize.Width - Margins.Left - Margins.Right;

            // Serious problem: Abandon ship.
            if (width < formtxtSample.Width)
                return;

            string strLine;
            Pen pn = new Pen(Brushes.Black, 2);
            StringReader reader = new StringReader(txt);

            // Call ProcessLine to store each line in listLines.
            while (null != (strLine = reader.ReadLine()))
                ProcessLine(strLine, width, listLines);

            reader.Close();

            // Now start getting ready to print pages.
            double heightLine = formtxtSample.LineHeight + formtxtSample.Height;
            double height = PageSize.Height - Margins.Top - Margins.Bottom;
            int linesPerPage = (int)(height / heightLine);

            // Serious problem: Abandon ship.
            if (linesPerPage < 1)
                return;

            int numPages = (listLines.Count + linesPerPage - 1) / linesPerPage;
            double xStart = Margins.Left;
            double yStart = Margins.Top;

            // Create the List to store each DocumentPage object.
            listPages = new List<DocumentPage>();
            
            for (int iPage = 0, iLine = 0; iPage < numPages; iPage++)
            {
                // Create the DrawingVisual and open the DrawingContext.
                DrawingVisual vis = new DrawingVisual();
                DrawingContext dc = vis.RenderOpen();

                // Display header at top of page.
                if (Header != null && Header.Length > 0)
                {
                    FormattedText formtxt = GetFormattedText(Header);
                    formtxt.SetFontWeight(FontWeights.Bold);
                    Point ptText = new Point(xStart, yStart - 2 * formtxt.Height);
                    dc.DrawText(formtxt, ptText);
                }

                // Display footer at bottom of page.
                if (numPages > 1)
                {
                    FormattedText formtxt = 
                        GetFormattedText("Page " + (iPage+1) + " of " + numPages);
                    formtxt.SetFontWeight(FontWeights.Bold);
                    Point ptText = new Point(
                        (PageSize.Width + Margins.Left - 
                                            Margins.Right - formtxt.Width) / 2,
                        PageSize.Height - Margins.Bottom + formtxt.Height);
                    dc.DrawText(formtxt, ptText);
                }

                // Look through the lines on the page.
                for (int i = 0; i < linesPerPage; i++, iLine++)
                {
                    if (iLine == listLines.Count)
                        break;

                    // Set up information to display the text of the line.
                    string str = listLines[iLine].String;
                    FormattedText formtxt = GetFormattedText(str);
                    Point ptText = new Point(xStart, yStart + i * heightLine); 
                    dc.DrawText(formtxt, ptText);

                    // Possibly display the little arrow flag.
                    if (listLines[iLine].Flag)
                    {
                        double x = xStart + width + 6;
                        double y = yStart + i * heightLine + formtxt.Baseline;
                        double len = face.CapsHeight * em;
                        dc.DrawLine(pn, new Point(x, y), 
                                        new Point(x + len, y - len));
                        dc.DrawLine(pn, new Point(x, y), 
                                        new Point(x, y - len / 2));
                        dc.DrawLine(pn, new Point(x, y), 
                                        new Point(x + len / 2, y));
                    }
                }
                dc.Close();

                // Create DocumentPage object based on visual.
                DocumentPage page = new DocumentPage(vis);
                listPages.Add(page);
            }
            reader.Close();
        }

        // Process each line of text into multiple printed lines.
        void ProcessLine(string str, double width, List<PrintLine> list)
        {
            str = str.TrimEnd(' ');

            // TextWrapping == TextWrapping.NoWrap.
            // ------------------------------------
            if (TextWrapping == TextWrapping.NoWrap)
            {
                do
                {
                    int length = str.Length;

                    while (GetFormattedText(str.Substring(0, length)).Width > 
                                                                        width)
                        length--;

                    list.Add(new PrintLine(str.Substring(0, length), 
                                                    length < str.Length));
                    str = str.Substring(length);
                }
                while (str.Length > 0);
            }
            // TextWrapping == TextWrapping.Wrap or TextWrapping.WrapWithOverflow.
            // -------------------------------------------------------------------
            else 
            {
                do
                {
                    int length = str.Length;
                    bool flag = false;

                    while (GetFormattedText(str.Substring(0, length)).Width > width)
                    {
                        int index = str.LastIndexOfAny(charsBreak, length - 2);

                        if (index != -1)
                            length = index + 1; // Include trailing space or dash.
                        else
                        {
                            // At this point, we know that the next possible 
                            //  space or dash break is beyond the allowable 
                            //  width. Check if there's *any* space or dash break.
                            index = str.IndexOfAny(charsBreak);

                            if (index != -1)
                                length = index + 1;

                            // If TextWrapping.WrapWithOverflow, just display the
                            //  line. If TextWrapping.Wrap, break it with a flag.
                            if (TextWrapping == TextWrapping.Wrap)
                            {
                                while (GetFormattedText(str.Substring(0, length)).
                                                                    Width > width)
                                    length--;

                                flag = true;
                            }
                            break;  // out of while loop.
                        }
                    }

                    list.Add(new PrintLine(str.Substring(0, length), flag));
                    str = str.Substring(length);
                }
                while (str.Length > 0);
            }
        }
        // Private method to create FormattedText object.
        FormattedText GetFormattedText(string str)
        {
            return new FormattedText(str, CultureInfo.CurrentCulture,
                            FlowDirection.LeftToRight, face, em, Brushes.Black);
        }
    }
}
