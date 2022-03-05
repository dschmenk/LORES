masm /ml memops;
masm /ml cgaops;
masm /ml timer;
cl /Ox /I..\..\inc /c mapio.c
cl /Ox /I..\..\inc /c lores.c
cl /Ox /I..\..\inc /c tiler.c
lib ..\..\lib\lores.lib -+mapio.obj -+lores.obj -+tiler.obj -+timer.obj -+memops.obj -+cgaops.obj ;
cl /Ox /I..\..\inc /c /DPROFILE tiler.c
lib ..\..\lib\lrprof.lib -+mapio.obj -+lores.obj -+tiler.obj -+timer.obj -+memops.obj -+cgaops.obj ;
del *.obj
del ..\..\lib\*.bak
