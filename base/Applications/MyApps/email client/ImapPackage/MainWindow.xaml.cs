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
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        public static Frame MainFrame { get; set; }
        public static bool LoggedIn { get; set; }

        public MainWindow()
        {
            InitializeComponent();
            MainFrame = mainFrame;

            // Chance are, its not logged in.
            MainFrame.Content = new LoginPage();

            // Initialize the Imap
            ImapService.Initialize();
        }
    }
}
