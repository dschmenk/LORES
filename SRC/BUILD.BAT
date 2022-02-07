masm /ml memops;
masm /ml cgaops;
masm /ml timer;
cl /Ox /I..\inc /c lores.c
cl /Ox /I..\inc /c tiler.c
lib ..\lib\lores.lib -+lores.obj -+tiler.obj -+timer.obj -+memops.obj -+cgaops.obj ;
cl /Ox /I..\inc /c /DCGA_SNOW lores.c
cl /Ox /I..\inc /c /DCGA_SNOW tiler.c
lib ..\lib\lrsnow.lib -+lores.obj -+tiler.obj -+timer.obj -+memops.obj -+cgaops.obj ;
cl /Ox /I..\inc /c /DPROFILE lores.c
cl /Ox /I..\inc /c /DPROFILE tiler.c
lib ..\lib\lrprof.lib -+lores.obj -+tiler.obj -+timer.obj -+memops.obj -+cgaops.obj ;
cl /Ox /I..\inc /c /DCGA_SNOW /DPROFILE lores.c
cl /Ox /I..\inc /c /DCGA_SNOW /DPROFILE tiler.c
lib ..\lib\lrprosno.lib -+lores.obj -+tiler.obj -+timer.obj -+memops.obj -+cgaops.obj ;
cl /Ox /I..\inc /o..\bin\tiledemo.exe tiledemo.c ..\lib\lores.lib
cl /Ox /I..\inc /o..\bin\tilesnow.exe /DCGA_SNOW tiledemo.c ..\lib\lrsnow.lib
cl /Ox /I..\inc /o..\bin\tileprof.exe /DPROFILE tiledemo.c ..\lib\lrprof.lib
cl /Ox /I..\inc /o..\bin\tilepsno.exe /DCGA_SNOW /DPROFILE tiledemo.c ..\lib\lrprosno.lib
cl /Ox /I..\inc /o..\bin\sprtdemo.exe sprtdemo.c ..\lib\lores.lib
cl /Ox /I..\inc /o..\bin\sprtsnow.exe /DCGA_SNOW sprtdemo.c ..\lib\lrsnow.lib
cl /Ox /I..\inc /o..\bin\sprtprof.exe /DPROFILE sprtdemo.c ..\lib\lrprof.lib
cl /Ox /I..\inc /o..\bin\sprtpsno.exe /DCGA_SNOW /DPROFILE sprtdemo.c ..\lib\lrprosno.lib
del *.obj
del ..\lib\*.bak
