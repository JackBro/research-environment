#!/bin/bash

#Found at http://askubuntu.com/a/193065/35729
#don't forget to 'chmod +x <thisFile>'

sudo add-apt-repository ppa:motumedia/mplayer-daily
sudo add-apt-repository ppa:videolan/stable-daily
sudo apt-get update
sudo apt-get upgrade

sudo apt-get install libaacs0 libbluray1

mkdir -p ~/.config/aacs
pushd ~/.config/aacs
wget http://vlc-aacs.whoknowsmy.name/files/KEYDB.cfg
popd

sudo apt-get install vlc vlc-plugin-pulse
