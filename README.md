# psvr-player
A 3D movie player for the PS VR headset connected to your computer.

Player features:  
- vr helmet position compensation;  
- adjusting the interpupillary distance;  
- correction of geometric and color distortions, color format, streams order;  
- support for 180 side-by-side (sbs) 3D movie format;
- minimalistic interface.
  
Supported OS: Linux.  

## Building for Linux  
  
Building on Linux is made from source code and requires installation of additional libraries to work with vr helmet hardware, graphics acceleration, etc.  
Commands for installation are given for Debian OS, for other OS use the appropriate package manager.  
Build sequence:  
1. Install the compiler and build programs: git, cmake, g++, xxd  
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
  
## Preparing the psvrplayer  
  
You need to do two things before using psvrplayer:  
1. Calibrate the vr helmet  
Place the vr helmet on a stable horizontal surface.  
Run the command:  
**psvrplayer --calibration**  
and follow the instructions  
2. Select vr screen  
Run the command with the vr helmet connected and turned on:  
**psvrplayer --listscreens**  
The program will display a list of screens, select the appropriate one for your vr helmet and remember the screen position, for example: **1920x0**  
  
## Playing 3D movies  
  
Run the command:
**psvrplayer --screen=1920x0 --play==filename**  
where:  
1920x0 - screen position, substitute your own value obtained when selecting the screen (see preparation section)  
filename - name of the movie file  

### Playback control  
Playback is controlled from the keyboard:  
Esc - finish playing and quit  
Space - pause/playback  
Left Ctrl - center the view  
Left Arrow - rewind: normal, fast (double-click), very fast (triple-click)  
Right Arrow - forward: normal, fast (double-click), very fast (triple-click).  

### More information
For information on command line options, run the command:
**psvrplayer --help**  
For more information, visit [apoheliy.com/psvrplayer](https://apoheliy.com/psvrplayer/)  
  
## Authors  
  
**Evgeny Kislov** - [evgenykislov.com](https://evgenykislov.com), [github/evgenykislov](https://github.com/evgenykislov)  
  
## License  
  
This project is licensed under the GPL License - see the [LICENSE](LICENSE) file for details  
  