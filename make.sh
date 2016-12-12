#!/bin/sh
main=express_vpn

# Ensure "out" directory exists, create it if it doesn't
if [ ! -d "./out" ]; then
  mkdir "./out"
fi 

# Compile and link
gcc -Wall -shared -o ./out/lib${main}.so -fPIC ./src/${main}.c `pkg-config --cflags --libs libxfce4panel-1.0` `pkg-config --cflags --libs gtk+-3.0`

# Copy .desktop file into "out" directory
cp ./src/express_vpn.desktop ./out