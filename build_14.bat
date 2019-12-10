set WORKSPACE=%~dp0
set INSTALL_DIRS=output
echo %INSTALL_DIRS%

call "%VS140COMNTOOLS%vsvars32.bat"
mkdir %INSTALL_DIRS%

devenv CUBRIDProvider-vs2017.sln /project CUBRIDProvider.vcxproj /Rebuild "Release|Win32" 
devenv CUBRIDProvider-vs2017.sln /project CUBRIDProvider.vcxproj /Rebuild "Release|x64" 

copy Win32\Release\CUBRIDProvider.dll  %INSTALL_DIRS%\CUBRIDProvider32.dll
copy x64\Release\CUBRIDProvider.dll  %INSTALL_DIRS%\CUBRIDProvider64.dll

copy installer\installer.nsi %INSTALL_DIRS%\installer.nsi
copy installer\license.txt %INSTALL_DIRS%\license.txt
copy installer\README.txt %INSTALL_DIRS%\README.txt

c:
cd C:\NSIS

makensis %WORKSPACE%\%INSTALL_DIRS%\installer.nsi