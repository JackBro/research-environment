using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

using ImapX;
using ImapX.Collections;
using System.Windows;

namespace ImapPackage
{
    class ImapService
    {
        private static ImapClient client { get; set; }

        public static void Initialize()
        {
            client = new ImapClient("imap.gmail.com", true);

            if(!client.Connect())
            {
                throw new Exception("Error connecting to the client.");
            }
        }

        public static bool Login(string u, string p)
        {
            return client.Login(u, p);
        }

        public static void Logout()
        {
            // Remove the login value from the client.
            if(client.IsAuthenticated) { client.Logout(); }

            // Clear the logged in value.
            MainWindow.LoggedIn = false;
        }

        public static List<EmailFolder> GetFolders()
        {
            var folders = new List<EmailFolder>();

            foreach (var folder in client.Folders)
            {
                folders.Add(new EmailFolder { Title = folder.Name });
            }

            // Before returning start the idling
            client.Folders.Inbox.StartIdling(); // And continue to listen for more.

            client.Folders.Inbox.OnNewMessagesArrived += Inbox_OnNewMessagesArrived;

            return folders;
        }

        private static void Inbox_OnNewMessagesArrived(object sender, IdleEventArgs e)
        {
            // Show a dialog
            MessageBox.Show($"A new message was downloaded in {e.Folder.Name} folder.");
        }

        public static MessageCollection GetMessagesForFolder(string name)
        {
            client.Folders[name].Messages.Download(); // Start the download process;
            return client.Folders[name].Messages;
        }
    }
}
