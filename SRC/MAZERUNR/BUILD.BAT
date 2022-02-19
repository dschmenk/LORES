cl /Ox /I..\..\inc /DCGA_SNOW /omazerunr.exe mazerunr.c ..\..\lib\lores.lib
cl /Ox /I..\..\inc /DCGA_SNOW /DPROFILE /omazeprof.exe mazerunr.c ..\..\lib\lrprof.lib
del *.obj
