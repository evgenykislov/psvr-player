# psvr-player
A 3D movie player for the PS VR headset connected to your computer.

Player features:  
- geometric and color correction;
- support of 180-view of 3D movies;
- minimalistic interface.

Current status: in development.

## Build

1. Install following applications for building: git, cmake, g++, xxd  
**sudo apt install git cmake g++ xxd**  
2. Install libraries: libvlccore-dev, libvlc-dev, libhidapi-dev, libglfw3-dev, libglm-dev:  
**sudo apt install libvlccore-dev libvlc-dev libvlc-dev libhidapi-dev libglfw3-dev libglm-dev**  
3. Download the code from github or gitflic: run the command in your home directory:  
**git clone https://github.com/evgenykislov/psvr-player.git -b main**  
3. Build psvr-player and install it:  
**cd psvr-player**  
**cmake -B build**  
**cmake --build build**  
**sudo cmake --install build**  
