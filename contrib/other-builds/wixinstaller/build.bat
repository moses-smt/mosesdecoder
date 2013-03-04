@echo off
candle MosesDecoder.wxs
light -ext WixUIExtension MosesDecoder.wixobj

mkdir stubs
mkdir dist_models

set homedir=%cd%
echo %homedir%
for /f %%f in ('dir /ad /b models') do (
	pushd %homedir%\models\%%f
	echo Building %%f.wxs ...
	Rem candle %%fInstall.wxs
	Rem light %%fInstall.wixobj -ext WixUIExtension
	Rem setupbld -out %%fInstall.exe -mi %%fInstall.msi -setup "C:\Program Files (x86)\WiX Toolset v3.7\bin\setup.exe" -title "Model Install"
	candle %%f.wxs -ext WixBalExtension -ext WiXUtilExtension
	light %%f.wixobj -ext WixBalExtension -ext WiXUtilExtension
	move %%f.exe %homedir%\stubs
	Rem copy %%fInstall.exe %homedir%\dist_models
	copy %%fInstall.zip %homedir%\dist_models
	popd
)

candle MosesInstaller.wxs -ext WixBalExtension -ext WiXUtilExtension
light MosesInstaller.wixobj -ext WixBalExtension -ext WiXUtilExtension

@echo on

