using System;
using System.Drawing;
using System.Collections;
using System.ComponentModel;
using System.Windows.Forms;
using System.Data;

using TAPI3Lib;

namespace complete
{
	/// <summary>
	/// Summary description for Form1.
	/// </summary>
	public class Form1 : System.Windows.Forms.Form
	{
		private System.Windows.Forms.ListBox listBox1;
		private System.Windows.Forms.TextBox textBox1;
		private System.Windows.Forms.Button button1;
		private System.Windows.Forms.Button button2;
		private System.Windows.Forms.Button button3;
		private System.Windows.Forms.Label label2;
		private System.Windows.Forms.Label label3;
		/// <summary>
		/// Required designer variable.
		/// </summary>
		private System.ComponentModel.Container components = null;


		private TAPIClass tapi;
		private callnotification call_notify;
		private long registration;
		public static ITAddress call_address;
				
		public ITBasicCallControl call_control;

		public ITCallHub  Call_Hub;


		public Form1()
		{
			//
			// Required for Windows Form Designer support
			//
			InitializeComponent();

			bool found=false;

			tapi=new TAPIClass();
			tapi.Initialize();
            
			call_notify=new callnotification();
			call_notify.addtolist=new callnotification.listshow(this.status);
			tapi.ITTAPIEventNotification_Event_Event+=new TAPI3Lib.ITTAPIEventNotification_EventEventHandler(call_notify.Event);
			tapi.EventFilter=(int)(TAPI_EVENT.TE_CALLNOTIFICATION|
				TAPI_EVENT.TE_DIGITEVENT|
				TAPI_EVENT.TE_PHONEEVENT|
				TAPI_EVENT.TE_CALLSTATE|
				TAPI_EVENT.TE_GENERATEEVENT|
				TAPI_EVENT.TE_GATHERDIGITS|
				TAPI_EVENT.TE_REQUEST);


			ITCollection collec;
			ITAddress address;
			ITMediaSupport support;
			ITAddressCapabilities capability;
			collec=(ITCollection)tapi.Addresses;
			foreach(ITAddress addr in collec )
			{

				found=false;
				address=addr;
				support=(ITMediaSupport)address;
				capability=(ITAddressCapabilities)address;
				if(support.QueryMediaType(TapiConstants.TAPIMEDIATYPE_AUDIO))
				{
					found=true;
				}
				capability=null;
				support=null;
				address=null;
				if(found==true)
				{
					if(addr.AddressName.ToUpper()=="H323 LINE")
					{

						call_address=addr;
					}
					//break;
				}
			}

			////registration part
			registration=tapi.RegisterCallNotifications(call_address,true,true,TapiConstants.TAPIMEDIATYPE_AUDIO,1);


		}

		/// <summary>
		/// Clean up any resources being used.
		/// </summary>
		protected override void Dispose( bool disposing )
		{
			if( disposing )
			{
				if (components != null) 
				{
					components.Dispose();
				}
			}
			base.Dispose( disposing );
		}

		#region Windows Form Designer generated code
		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>
		private void InitializeComponent()
		{
			this.listBox1 = new System.Windows.Forms.ListBox();
			this.textBox1 = new System.Windows.Forms.TextBox();
			this.button1 = new System.Windows.Forms.Button();
			this.button2 = new System.Windows.Forms.Button();
			this.button3 = new System.Windows.Forms.Button();
			this.label2 = new System.Windows.Forms.Label();
			this.label3 = new System.Windows.Forms.Label();
			this.SuspendLayout();
			// 
			// listBox1
			// 
			this.listBox1.ItemHeight = 14;
			this.listBox1.Location = new System.Drawing.Point(16, 128);
			this.listBox1.Name = "listBox1";
			this.listBox1.Size = new System.Drawing.Size(200, 102);
			this.listBox1.TabIndex = 0;
			// 
			// textBox1
			// 
			this.textBox1.Location = new System.Drawing.Point(72, 16);
			this.textBox1.Name = "textBox1";
			this.textBox1.Size = new System.Drawing.Size(216, 20);
			this.textBox1.TabIndex = 1;
			this.textBox1.Text = "127.0.0.1";
			// 
			// button1
			// 
			this.button1.Location = new System.Drawing.Point(160, 64);
			this.button1.Name = "button1";
			this.button1.Size = new System.Drawing.Size(120, 32);
			this.button1.TabIndex = 2;
			this.button1.Text = "Call";
			this.button1.Click += new System.EventHandler(this.button1_Click);
			// 
			// button2
			// 
			this.button2.Location = new System.Drawing.Point(32, 64);
			this.button2.Name = "button2";
			this.button2.Size = new System.Drawing.Size(120, 32);
			this.button2.TabIndex = 3;
			this.button2.Text = "Disconnect";
			this.button2.Click += new System.EventHandler(this.button2_Click);
			// 
			// button3
			// 
			this.button3.Location = new System.Drawing.Point(224, 200);
			this.button3.Name = "button3";
			this.button3.TabIndex = 4;
			this.button3.Text = "Clear Status";
			this.button3.Click += new System.EventHandler(this.button3_Click);
			// 
			// label2
			// 
			this.label2.BackColor = System.Drawing.Color.Transparent;
			this.label2.ForeColor = System.Drawing.Color.Black;
			this.label2.Location = new System.Drawing.Point(8, 16);
			this.label2.Name = "label2";
			this.label2.Size = new System.Drawing.Size(64, 16);
			this.label2.TabIndex = 6;
			this.label2.Text = "IP Address";
			// 
			// label3
			// 
			this.label3.BackColor = System.Drawing.Color.Transparent;
			this.label3.ForeColor = System.Drawing.Color.Black;
			this.label3.Location = new System.Drawing.Point(16, 104);
			this.label3.Name = "label3";
			this.label3.Size = new System.Drawing.Size(104, 24);
			this.label3.TabIndex = 7;
			this.label3.Text = "Call Status";
			// 
			// Form1
			// 
			this.AutoScaleBaseSize = new System.Drawing.Size(5, 13);
			this.ClientSize = new System.Drawing.Size(330, 239);
			this.Controls.Add(this.label3);
			this.Controls.Add(this.label2);
			this.Controls.Add(this.button3);
			this.Controls.Add(this.button2);
			this.Controls.Add(this.button1);
			this.Controls.Add(this.textBox1);
			this.Controls.Add(this.listBox1);
			this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
			this.MaximizeBox = false;
			this.Name = "Form1";
			this.StartPosition = System.Windows.Forms.FormStartPosition.CenterScreen;
			this.Text = "Audio Conferencing - www.fadidotnet.org";
			this.ResumeLayout(false);

		}
		#endregion

		private void status(string str)
		{
			listBox1.Items.Add(str);
		}

		/// <summary>
		/// The main entry point for the application.
		/// </summary>
		[STAThread]
		static void Main() 
		{
			Application.Run(new Form1());
		}

		private void button1_Click(object sender, System.EventArgs e)
		{
			string addr=textBox1.Text;
			MessageBox.Show("Dialing to "+addr+" ...");
			call_control=call_address.CreateCall(addr,TapiConstants.LINEADDRESSTYPE_IPADDRESS,TapiConstants.TAPIMEDIATYPE_AUDIO | TapiConstants.TAPIMEDIATYPE_VIDEO);
			button1.Enabled=false;
			IEnumStream enum_stream;
			ITStreamControl pstream_control;
			pstream_control=(ITStreamControl)call_control;
			pstream_control.EnumerateStreams(out enum_stream);
			ITStream p_stream;
			uint a11=0;
			enum_stream.Next(1,out p_stream,ref a11);
			int imedia;
			imedia=p_stream.MediaType;
			TERMINAL_DIRECTION dir;
			dir=p_stream.Direction;
			ITTerminal termi,termi1;
				
			ITTerminalSupport term_support=(ITTerminalSupport)call_address;
			termi=term_support.GetDefaultStaticTerminal(imedia,dir);
			p_stream.SelectTerminal(termi);

			

			enum_stream.Next(1,out p_stream,ref a11);
			termi1=term_support.GetDefaultStaticTerminal(imedia,TERMINAL_DIRECTION.TD_CAPTURE);
			p_stream.SelectTerminal(termi1);
			call_control.Connect(false);

		}

		private void button2_Click(object sender, System.EventArgs e)
		{
		       call_control.Disconnect(DISCONNECT_CODE.DC_NORMAL);
			   button1.Enabled=true;
		}

		private void button3_Click(object sender, System.EventArgs e)
		{
		       listBox1.Items.Clear();
		}
	}
}
