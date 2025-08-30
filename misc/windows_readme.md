# psvr-player
A 3D movie player for the PS VR headset connected to your computer.

Player features:  
- vr helmet position compensation;  
- adjusting the interpupillary distance;  
- correction of geometric and color distortions, color format, streams order;  
- support for 180 side-by-side (sbs) 3D movie format;
- minimalistic interface.
  
# Quick Start
  
## Connecting PSVR headset  
The PSVR headset is connected via a standard VR processor. HDMI and USB connectors are connected to the computer instead of connecting to the Playstation.  
  
## Preparing the psvrplayer  
You need to do follow things before using psvrplayer:  
1. Select VR helmet devices  
Run the command:
**psvrplayer --selectdevices**  
and select control and sensor devices from the suggested list.
2. Calibrate the vr helmet  
Place the vr helmet on a stable horizontal surface.  
Run the command:  
**psvrplayer --calibration**  
and follow the instructions  
3. Select vr screen  
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
  