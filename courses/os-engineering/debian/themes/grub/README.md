Configure graphical splashimage

    install the grub2-splashimages package: aptitude install grub2-splashimages
    choose one of the nice images to use as the splashimage. You can look at the images with any image viewer (Ex: display)

    configure/add the GRUB_BACKGROUND variable in /etc/default/grub. Ex: GRUB_BACKGROUND="/usr/share/images/grub/Lake_mapourika_NZ.tga"

    make sure that GRUB_TERMINAL=console is commented out. The graphical mode will not be enabled if this is uncommented. By default it's commented, so you shouldn't worry too much. 

    the graphical resolution can be changed via the variable GRUB_GFXMODE

    run update-grub
    reboot to observe the change 
