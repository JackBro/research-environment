Dell Optiplex 755


1. Download Debain 8 Standard.

2. Create USB boot stick

3. Install Operating System

4. Install graphics drivers an X.

     $sudo apt-get install firmware-linux-nonfree libgl1-mesa-dri xserver-xorg-video-ati
     
5. Install Budgie-Desktop dependencies.
	
     $sudo apt-get install accountsservice libdbus-glib2.0-cil libglib2.0-dev gnome-bluetooth gnome gir1.2-glib-2.0 gobject-introspection libgtk2.0 ibus libpulse-devlibupower-glib-dev uuid valac gtk-doc-tools curl

6. Install Mono complete and fsharp

     $sudo apt-get install mono-complete fsharp

7. Intall DeepDive and postgres

     $bash <(curl -fsSL git.io/getdeepdive)

     export PATH=~/local/bin:"$PATH"

8. Set Network Manager to Managed

	managed=true in /etc/NetworkManager/NetworkManager.conf
	$sudo /etc/init.d/network-manager restart

	reboot
9. Install Menedley Desktop

	https://www.mendeley.com/download-mendeley-desktop/debian/instructions/



 
