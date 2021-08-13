@echo off

IF NOT EXIST ..\build mkdir ..\build

SET DIRECTX_LIB_PATH="C:\Program Files (x86)\Microsoft DirectX SDK (June 2010)\Lib\x64"
SET DIRECTX_INCLUDE_PATH="C:\Program Files (x86)\Microsoft DirectX SDK (June 2010)\Include"

pushd ..\build
cl -nologo /Zi -wd4005 ..\code\main.cpp User32.lib d3d11.lib d3dx11.lib dxerr.lib d3dcompiler.lib /I%DIRECTX_INCLUDE_PATH% /link /LIBPATH:%DIRECTX_LIB_PATH%
popd
