using System;

using TAPI3Lib;
using System.Windows.Forms;
using System.Threading;
using System.Runtime.InteropServices;


namespace complete
{
	class callnotification:TAPI3Lib.ITTAPIEventNotification
	{
		public delegate void listshow(string str);
		public listshow addtolist;
		

		public void Event(TAPI3Lib.TAPI_EVENT te,object eobj)
		{
			switch(te)
			{
				case TAPI3Lib.TAPI_EVENT.TE_CALLNOTIFICATION:
					addtolist("call notification event has occured");
					ITCallNotificationEvent notify;
					notify=(ITCallNotificationEvent)eobj;
					ITCallInfo newcall;
					newcall=notify.Call;
					ITBasicCallControl call_control;//=f1.call_control;
					call_control=(ITBasicCallControl)newcall;
					string callerid=newcall.get_CallInfoString(CALLINFO_STRING.CIS_CALLERIDNUMBER);					if(MessageBox.Show("calling from "+callerid+"\n\n Press Ok to Attend Call    /   Cancel to Reject the Call\n","Answer",MessageBoxButtons.OKCancel)==DialogResult.OK)
					{	
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
				
						ITTerminalSupport term_support=(ITTerminalSupport)Form1.call_address;
						termi=term_support.GetDefaultStaticTerminal(imedia,dir);

						p_stream.SelectTerminal(termi);

						enum_stream.Next(1,out p_stream,ref a11);
						termi1=term_support.GetDefaultStaticTerminal(imedia,TERMINAL_DIRECTION.TD_CAPTURE);

						p_stream.SelectTerminal(termi1);

						call_control.Answer();
					}

					else
					{
						call_control.Disconnect(DISCONNECT_CODE.DC_REJECTED);
					}
					///////
					break;
				case TAPI3Lib.TAPI_EVENT.TE_DIGITEVENT:
					TAPI3Lib.ITDigitDetectionEvent dd=(TAPI3Lib.ITDigitDetectionEvent)eobj;
					addtolist("Dialed digit"+dd.ToString());
					break;
				case TAPI3Lib.TAPI_EVENT.TE_GENERATEEVENT:
					TAPI3Lib.ITDigitGenerationEvent dg=(TAPI3Lib.ITDigitGenerationEvent)eobj;
					addtolist("Dialed digit"+dg.ToString());
					break;
				case TAPI3Lib.TAPI_EVENT.TE_PHONEEVENT:
					addtolist("A phone event!");
					break;
				case TAPI3Lib.TAPI_EVENT.TE_GATHERDIGITS:
					addtolist("Gather digit event!");
					break;
				case TAPI3Lib.TAPI_EVENT.TE_CALLSTATE:
					TAPI3Lib.ITCallStateEvent a=(TAPI3Lib.ITCallStateEvent)eobj;
					TAPI3Lib.ITCallInfo b=a.Call;
				switch(b.CallState)
				{
					case TAPI3Lib.CALL_STATE.CS_INPROGRESS:
						addtolist("dialing");
						break;
					case TAPI3Lib.CALL_STATE.CS_CONNECTED:
						addtolist("Connected");
						break;
					case TAPI3Lib.CALL_STATE.CS_DISCONNECTED:
						addtolist("Disconnected");
						break;
					case TAPI3Lib.CALL_STATE.CS_OFFERING:
						addtolist("A party wants to communicate with you!");
						break;
					case TAPI3Lib.CALL_STATE.CS_IDLE:
						addtolist("Call is created!");
						break;
				}
					break;
			}
		}
	}


}
