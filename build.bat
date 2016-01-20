set WORKSPACE=%~dp0
set INSTALL_DIRS=output
echo %INSTALL_DIRS%

call "E:\Program Files (x86)\Microsoft Visual Studio 9.0\Common7\Tools\vsvars32.bat"
mkdir %INSTALL_DIRS%

devenv CUBRIDProvider.sln /project CUBRIDProvider.vcproj /rebuild "release|Win32" 
devenv CUBRIDProvider.sln /project CUBRIDProvider.vcproj /rebuild "debug|Win32" 
devenv CUBRIDProvider.sln /project CUBRIDProvider.vcproj /rebuild "release|x64" 
devenv CUBRIDProvider.sln /project CUBRIDProvider.vcproj /rebuild "debug|x64" 

 
copy Win32\Debug\CUBRIDProvider.dll  %INSTALL_DIRS%\CUBRIDProvider32_d.dll
copy Win32\Release\CUBRIDProvider.dll  %INSTALL_DIRS%\CUBRIDProvider32.dll

copy x64\Debug\CUBRIDProvider.dll  %INSTALL_DIRS%\CUBRIDProvider64_d.dll
copy x64\Release\CUBRIDProvider.dll  %INSTALL_DIRS%\CUBRIDProvider64.dll

copy installer\installer.nsi %INSTALL_DIRS%\installer.nsi
copy installer\license.txt %INSTALL_DIRS%\license.txt
copy installer\README.txt %INSTALL_DIRS%\README.txt

c:
cd C:\NSIS

makensis %WORKSPACE%\%INSTALL_DIRS%\installer.nsi