pXar
====

pixel Xpert analysis &amp; readout

Please visit https://twiki.cern.ch/twiki/bin/view/CMS/Pxar for instructions on how to install and use the software.

To install pXar under Windows, please follow these steps:
- download pXar software via git or as .zip file
- install CMake
- install a C++ compiler, e.g. Visual Studio Express Desktop (2013 Version): http://www.microsoft.com/en-us/download/details.aspx?id=40787
- download FTD2XX drivers from http://www.ftdichip.com/Drivers/D2XX.htm and extract the driver files into e.g. pxar/extern/ftd2xx -- please make sure that ftd2xx.h and the full directory structure is present in this folder.
- install libusb library (download from http://sourceforge.net/projects/libusb/files/libusb-1.0, for documentation see http://libusb.info) e.g. into ./extern/libusb
- create a build directory in the pxar folder
- open a shell (optimally the Visual Studio "Developer Command Prompt" from the Start Menu entries for Visual Studio (Tools subfolder) which opens a cmd.exe session with the necessary environment variables already set), enter the build directory and run ```cmake ..```
- now run ```MSBUILD.exe pxar.sln /p:Configuration=Release```
  or  ```MSBUILD.exe INSTALL.vcxproj /p:Configuration=Release``` to install the library into the source directory.
