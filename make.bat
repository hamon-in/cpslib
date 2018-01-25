CALL %LOCALAPPDATA%"\Programs\Common\Microsoft\Visual C++ for Python\9.0\vcvarsall.bat" %PROCESSOR_ARCHITECTURE%
cl /c common.c pslib_windows.c
LIB.EXE /OUT:pslib.obj pslib_windows.obj common.obj
LIB.EXE /OUT:pslib.lib pslib_windows.obj common.obj
cl driver.c /link pslib