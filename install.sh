#!/bin/sh
main=express_vpn

# Copy files
sudo cp ./out/${main}.desktop /usr/share/xfce4/panel-plugins/
sudo cp ./out/lib${main}.so /usr/lib/x86_64-linux-gnu/xfce4/panel-plugins/
