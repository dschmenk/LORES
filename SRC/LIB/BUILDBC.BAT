masm /ml memops;
masm /ml cgaops;
masm /ml timer;
bcc -O -Z -I..\..\inc -c mapio.c
bcc -O -Z -I..\..\inc -c lores.c
bcc -O -Z -I...\.\inc -c tiler.c
tlib /C ..\..\lib\loresbc.lib -+mapio.obj -+lores.obj -+tiler.obj -+timer.obj -+memops.obj -+cgaops.obj
bcc -O -Z -I..\..\inc -c -DPROFILE tiler.c
tlib /C ..\..\lib\lrprofbc.lib -+mapio.obj -+lores.obj -+tiler.obj -+timer.obj -+memops.obj -+cgaops.obj
del *.obj
del ..\..\lib\*.bak
