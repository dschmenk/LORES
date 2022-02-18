cl /Ox /I..\..\inc /omazerunr.exe mazerunr.c ..\..\lib\lores.lib
cl /Ox /I..\..\inc /DPROFILE /omazeprof.exe mazerunr.c ..\..\lib\lrprof.lib
del *.obj
