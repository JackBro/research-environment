using System;
using System.Windows.Forms;

namespace mymediaplayer
{
    public partial class Form1 : Form
    {
        public Form1()
        {
            InitializeComponent();
        }

        private void btnBrowse_Click(object sender, EventArgs e)
        {
            openFileDialog1.Filter = "mp3,wav,mp4,mov,wmv,mpg|*.mp3;*.wav;*.mp4;*.mov;*.wmv;*.mpg|all files|*.*";
            if(openFileDialog1.ShowDialog()==DialogResult.OK)
                axWindowsMediaPlayer1.URL = openFileDialog1.FileName;
        }
    }
}
