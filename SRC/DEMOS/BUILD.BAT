cl /Ox /I..\..\inc /otiledemo.exe tiledemo.c ..\..\lib\lores.lib
cl /Ox /I..\..\inc /otilesnow.exe /DCGA_SNOW tiledemo.c ..\..\lib\lores.lib
cl /Ox /I..\..\inc /otileprof.exe /DPROFILE tiledemo.c ..\..\lib\lrprof.lib
cl /Ox /I..\..\inc /otilepsno.exe /DCGA_SNOW /DPROFILE tiledemo.c ..\..\lib\lrprof.lib
cl /Ox /I..\..\inc /osprtdemo.exe sprtdemo.c ..\..\lib\lores.lib
cl /Ox /I..\..\inc /osprtprof.exe /DPROFILE sprtdemo.c ..\..\lib\lrprof.lib
del *.obj
