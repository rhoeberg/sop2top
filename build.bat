SET VC_PATH=C:\Program Files (x86)\Microsoft Visual Studio\2019\Community

IF NOT DEFINED LIB (IF EXIST "%VC_PATH%" (call "%VC_PATH%\VC\Auxiliary\Build\vcvars64.bat" %1))


msbuild sop2top.vcxproj /p:configuration=debug
