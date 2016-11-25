Microsoft Research IPv6 Parser for Network Monitor

NETWORK MONITOR:

Microsoft Network Monitor (also known as Bloodhound) is a tool for
capturing and graphically displaying packet traces. It supports
runtime-loadable parsers.  This release contains our IPv6 parser
module for Network Monitor. It is an add-on and you must have already
installed Network Monitor.

It has been tested with Network Monitor build 354 (in the Help About
dialog, it shows version 4.00.354) and build 503 (a preliminary build
of NetMon v2), and it should work with other versions as well.

Network Monitor ships with Windows NT Server (a "lite" version that only
supports capture of packets sent to/from the current machine)
and with Back Office (as part of SMS, or Systems Management Server).

The next major release of Network Monitor ("v2") will contain
a version of this IPv6 parser as a standard component.

[If you have the binary-only distribution, you can skip down to INSTALLATION]


BUILDING:

The build environment for the source distribution is that provided by the
Microsoft Windows NT Version 4.0 Device Driver Kit (DDK), which in turn
requires a machine with Windows NT 4.0, the Platform Software Development Kit
(SDK) for Win32 and Visual C++ 5.0 (or an equivalent 32-bit C compiler)
installed.

The Platform SDK provides the essential headers and libraries for
building a parser. Our parser build environment relies on the DDK only
for the build.exe command to control the build.  The Platform SDK
includes documentation describing parsers; in the help file, look in
the section on Systems Management Server.

If you do not have some of these tools and wish to acquire them, you will need
to join Microsoft's Developer Network (MSDN) at the Professional Subscription
level or greater.  Please see http://msdn.microsoft.com/developer/default.htm
for more information.  Educational institutions may be eligible for special
academic prices, please see your local academic reseller for pricing and
availability.


CREATING AN INSTALL KIT:

Once you have successfully built the tree, you can package up an IPv6
parser install kit for easy installation of the associated parts.  To
do this, go to the "kit" subdirectory of msripv6\netmon.  Run the
"mkkit.cmd" script with the path to an existing directory in which to
place the completed kit as an argument (for example, "mkkit C:\BHKit").


INSTALLATION:

The same procedure is used to install either our binary distribution or a
version you built yourself from our source distribution.

In the directory containing the install kit, run the setup.cmd script.
It takes as an argument the directory containing your Network Monitor
installation (for example, "setup c:\NM").

Note that the setup script overwrites several existing files
in your Network Monitor installation:
tcpip.dll - Contains UDP and TCP parsers, modified to support IPv6.
netmon.ini - Added ETYPE_IPv6 description.
parser.ini - Added IP6, ICMP6 protocols.
mac.ini - Added ETYPE_IPv6 -> IP6 handoff.
tcpip.ini - Added IP protocol 41 -> IP6 handoff.

It also installs two new files:
tcpip6.dll - Contains the new IPv6 and ICMPv6 parsers.
tcpip6.ini - Contains IP6 -> TCP, UDP, ICMP6 handoff.


MAILING LIST:

We have set up a mailing list for interested parties to use in
exchanging information regarding their experiences working with our
implementation.  We will also be making interim announcements
regarding the state of our implementation to this list.  To join, send
an email message containing the text
        subscribe msripv6-users yourfirstname yourlastname
to listserv@list.research.microsoft.com.


BUG REPORTS/FEEDBACK:

We welcome bug reports and other feedback, especially interoperability reports
with other IPv6 implementations.  Please send email on such subjects to
msripv6-bugs@list.research.microsoft.com.


CONTACT INFO:

Should you wish to contact us for any reason besides the above mentioned
bug reports/feedback, please send email to msripv6@microsoft.com or
write Microsoft Research, One Microsoft Way, Redmond, WA 98052-6399.
