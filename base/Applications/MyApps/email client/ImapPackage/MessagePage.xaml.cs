using ImapX;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;

namespace ImapPackage
{
    /// <summary>
    /// Interaction logic for MessagePage.xaml
    /// </summary>
    public partial class MessagePage : Page
    {
        public MessagePage()
        {
            InitializeComponent();
        }

        public MessagePage(Message message)
        {
            InitializeComponent();

            // Fill the page
            subject.Text = message.Subject;
            body.Text = message.Body.Text;
            time.Text = message.Date.Value.ToString();

            if(message.Attachments.Count() > 0)
            {
                attachments.Text = $"{message.Attachments.Count()} attachments with this message.";
            } else{
                attachments.Text = "No attachments.";
            }
        }
    }
}
