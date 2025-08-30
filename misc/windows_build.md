# Build for Windows:  
## Installing dependencies:  
  
### VLC
Download the vlc development library, compiled for your platform.  
If you have vlc player installed, then it is better to use the same version as the player. If the player is not installed, then take the latest one.  
For example:  
The vlc library version 3.0.21 for win64 can be downloaded from the link:  
https://download.videolan.org/pub/videolan/vlc/3.0.21/win64/vlc-3.0.21-win64.7z  
  
Create the libvlcsdk folder in the root of the C: drive. Copy the contents of the sdk folder (vlc-3.0.21\sdk) of the archive to this folder.  
Note: if everything is done correctly, then there must be a file  
C:\libvlcsdk\include\vlc\libvlc.h  
In folder C:\libvlcsdk\lib delete the vlc.lib and vlccore.lib files (they are 20 bytes in size and link to other files).  
  
### Hid Api  
Download the HidApi library from here:  
https://github.com/libusb/hidapi/releases  
We take the latest release (for example, hidapi-0.15.0).  
Create a folder C:\libhidapi and write the contents of the archive into it.  
In folder C:\libhidapi\include create a hidapi folder and copy the *.h files from the folder C:\libhidapi\include.
  
### Xxd  
The utility can be downloaded from here:  
https://sourceforge.net/projects/xxd-for-windows/  
We save the unpacked utility to a folder C:\utils (you can use any other one) and add this folder to the PATH variable.
  
### Glfw  
You can download the library from here:  
https://www.glfw.org/download.html  
We download precompiled binary files, for example, for Visual Studio 2022.
Create a folder C:\libglfw and copy the include and lib-vc2022 folders from the archive with it. Rename the lib-vc2022 folder to lib.  
  
### Glm  
Download the glm mathematical library from here:  
https://github.com/g-truc/glm/releases  
The contents of the archive are unpacked into a folder C:\libglm.
  
## Compiler
For the build, it is best to install the Visual Studio development environment (especially since it is available in the free version). In my case, version 2022 was used. You can use a different one, but make sure that the library versions match.  
  
You will also need the cmake utility.  
You can get it here https://cmake.org/download/  
  
## Building
Go to the project folder and run the commands:  
**cmake -B build**  
**cmake --build build --config Release**  
  
To use the player, put it in one folder (for example, C:\utils\psvrplayer ) the executable file psvrplayer.exe, copy there hidapi.dll (from C:\libhidapi\x64), the contents of the vlc library folder. Also add the path to the application in the PATH variable.
  
Use it!  