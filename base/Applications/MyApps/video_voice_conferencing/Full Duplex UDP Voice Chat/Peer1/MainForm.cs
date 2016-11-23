using System;
using System.Windows.Forms;
using System.Net;
using System.Net.Sockets;
using System.Threading;
using Voice;

namespace VOIP
{

	public class Form1 : System.Windows.Forms.Form
	{

		#region variables
		private Socket r;
		private Thread t;
		private bool connected = false;
		private System.Windows.Forms.TextBox textBox1;
		private System.Windows.Forms.Label label1;
		private System.Windows.Forms.Label label2;
		private System.Windows.Forms.TextBox textBox2;
		private System.Windows.Forms.TextBox textBox3;
		private System.Windows.Forms.Label label3;
		private Button button3;
		private Button button4;
		private System.ComponentModel.Container components = null;
		#endregion

		#region Form1()
		public Form1()
		{

			InitializeComponent();
			r = new Socket(AddressFamily.InterNetwork, SocketType.Dgram, ProtocolType.Udp);
			t = new Thread(new ThreadStart(Voice_In));
		}
		#endregion

		#region For Desginer
		/// <summary>
		/// Clean up any resources being used.
		/// </summary>
		protected override void Dispose(bool disposing)
		{
			if (disposing)
			{
				if (components != null)
				{
					components.Dispose();
				}
			}
			base.Dispose(disposing);
		}

		#region Windows Form Designer generated code
		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>
		private void InitializeComponent()
		{
			this.textBox1 = new System.Windows.Forms.TextBox();
			this.label1 = new System.Windows.Forms.Label();
			this.label2 = new System.Windows.Forms.Label();
			this.textBox2 = new System.Windows.Forms.TextBox();
			this.textBox3 = new System.Windows.Forms.TextBox();
			this.label3 = new System.Windows.Forms.Label();
			this.button3 = new System.Windows.Forms.Button();
			this.button4 = new System.Windows.Forms.Button();
			this.SuspendLayout();
			// 
			// textBox1
			// 
			this.textBox1.Location = new System.Drawing.Point(88, 8);
			this.textBox1.Name = "textBox1";
			this.textBox1.Size = new System.Drawing.Size(168, 20);
			this.textBox1.TabIndex = 4;
			this.textBox1.Text = "127.0.0.1";
			// 
			// label1
			// 
			this.label1.Location = new System.Drawing.Point(16, 8);
			this.label1.Name = "label1";
			this.label1.Size = new System.Drawing.Size(64, 16);
			this.label1.TabIndex = 5;
			this.label1.Text = "Recevier IP";
			// 
			// label2
			// 
			this.label2.Location = new System.Drawing.Point(16, 40);
			this.label2.Name = "label2";
			this.label2.Size = new System.Drawing.Size(128, 16);
			this.label2.TabIndex = 6;
			this.label2.Text = "Receiving Port Number";
			// 
			// textBox2
			// 
			this.textBox2.Location = new System.Drawing.Point(152, 40);
			this.textBox2.Name = "textBox2";
			this.textBox2.Size = new System.Drawing.Size(104, 20);
			this.textBox2.TabIndex = 7;
			this.textBox2.Text = "6000";
			// 
			// textBox3
			// 
			this.textBox3.Location = new System.Drawing.Point(152, 72);
			this.textBox3.Name = "textBox3";
			this.textBox3.Size = new System.Drawing.Size(104, 20);
			this.textBox3.TabIndex = 9;
			this.textBox3.Text = "5000";
			// 
			// label3
			// 
			this.label3.Location = new System.Drawing.Point(16, 72);
			this.label3.Name = "label3";
			this.label3.Size = new System.Drawing.Size(128, 16);
			this.label3.TabIndex = 8;
			this.label3.Text = "Sending Port Number";
			// 
			// button3
			// 
			this.button3.Location = new System.Drawing.Point(56, 104);
			this.button3.Name = "button3";
			this.button3.Size = new System.Drawing.Size(75, 27);
			this.button3.TabIndex = 10;
			this.button3.Text = "Start";
			this.button3.Click += new System.EventHandler(this.button3_Click);
			// 
			// button4
			// 
			this.button4.Location = new System.Drawing.Point(160, 104);
			this.button4.Name = "button4";
			this.button4.Size = new System.Drawing.Size(75, 27);
			this.button4.TabIndex = 11;
			this.button4.Text = "Stop";
			this.button4.Click += new System.EventHandler(this.button4_Click);
			// 
			// Form1
			// 
			this.AutoScaleBaseSize = new System.Drawing.Size(5, 13);
			this.AutoScroll = true;
			this.ClientSize = new System.Drawing.Size(314, 136);
			this.Controls.Add(this.button4);
			this.Controls.Add(this.button3);
			this.Controls.Add(this.textBox3);
			this.Controls.Add(this.textBox2);
			this.Controls.Add(this.textBox1);
			this.Controls.Add(this.label3);
			this.Controls.Add(this.label2);
			this.Controls.Add(this.label1);
			this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedSingle;
			this.MaximizeBox = false;
			this.Name = "Form1";
			this.StartPosition = System.Windows.Forms.FormStartPosition.CenterScreen;
			this.Text = "Peer to Peer Full Duplex Voice Chat";
			this.Closing += new System.ComponentModel.CancelEventHandler(this.Form1_Closing_1);
			this.ResumeLayout(false);

		}
		#endregion

		/// <summary>
		/// The main entry point for the application.
		/// </summary>
		/// 

		[STAThread]
		static void Main()
		{
			Application.Run(new Form1());
		}
		#endregion



		#region Voice_In()
		private void Voice_In()
		{
			byte[] br;
			r.Bind(new IPEndPoint(IPAddress.Any, int.Parse(this.textBox2.Text)));
			while (true)
			{
				br = new byte[16384];
				r.Receive(br);
				m_Fifo.Write(br, 0, br.Length);
			}
		}
		#endregion
		#region Voice_Out()
		
		private void Voice_Out(IntPtr data, int size)
		{
			//for Recorder
			if (m_RecBuffer == null || m_RecBuffer.Length < size)
				m_RecBuffer = new byte[size];
			System.Runtime.InteropServices.Marshal.Copy(data, m_RecBuffer, 0, size);
			//Microphone ==> data ==> m_RecBuffer ==> m_Fifo
			r.SendTo(m_RecBuffer, new IPEndPoint(IPAddress.Parse(this.textBox1.Text), int.Parse(this.textBox3.Text)));
		}  
		
		#endregion


        
		//********************************************************************************//
		private WaveOutPlayer m_Player;
		private WaveInRecorder m_Recorder;
		private FifoStream m_Fifo = new FifoStream();

		private byte[] m_PlayBuffer;
		private byte[] m_RecBuffer;


		private void button3_Click(object sender, EventArgs e)
		{
			if (connected == false)
			{
				t.Start();
				connected = true;
			}

			Start();
		}

		private void button4_Click(object sender, EventArgs e)
		{
			Stop();
		}

		private void Start()
		{
			Stop();
			try
			{
				WaveFormat fmt = new WaveFormat(44100, 16, 2);
				m_Player = new WaveOutPlayer(-1, fmt, 16384, 6, new BufferFillEventHandler(Filler));
				m_Recorder = new WaveInRecorder(-1, fmt, 16384, 6, new BufferDoneEventHandler(Voice_Out));
			}
			catch
			{
				Stop();
				throw;
			}
		}

		private void Stop()
		{
			if (m_Player != null)
				try
				{
					m_Player.Dispose();
				}
				finally
				{
					m_Player = null;
				}
			if (m_Recorder != null)
				try
				{
					m_Recorder.Dispose();
				}
				finally
				{
					m_Recorder = null;
				}
			m_Fifo.Flush(); // clear all pending data
		}

		private void Filler(IntPtr data, int size)
		{
			if (m_PlayBuffer == null || m_PlayBuffer.Length < size)
				m_PlayBuffer = new byte[size];
			if (m_Fifo.Length >= size)
				m_Fifo.Read(m_PlayBuffer, 0, size);
			else
				for (int i = 0; i < m_PlayBuffer.Length; i++)
					m_PlayBuffer[i] = 0;
			System.Runtime.InteropServices.Marshal.Copy(m_PlayBuffer, 0, data, size);
			// m_Fifo ==> m_PlayBuffer==> data ==> Speakers
		}

		private void Form1_Closing_1(object sender, System.ComponentModel.CancelEventArgs e)
		{
			t.Abort();
			r.Close();
			Stop();
		}

      
	}
}
