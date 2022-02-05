cl /Ox /I..\..\inc /omazerunr.exe mazerunr.c ..\..\lib\lores.lib
cl /Ox /I..\..\inc /DPROFILE /omazeprof.exe mazerunr.c ..\..\lib\lrprof.lib
cl /Ox /I..\..\inc /DCGA_SNOW /omazesnow.exe mazerunr.c ..\..\lib\lrsnow.lib
cl /Ox /I..\..\inc /DPROFILE /DCGA_SNOW /omazeprsn.exe mazerunr.c ..\..\lib\lrprosno.lib
del *.obj
