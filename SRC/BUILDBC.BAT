masm /ml memops;
masm /ml cgaops;
masm /ml timer;
bcc -O -Z -I..\inc -c lores.c
bcc -O -Z -I..\inc -c tiler.c
tlib /C ..\lib\loresbc.lib -+lores.obj -+tiler.obj -+timer.obj -+memops.obj -+cgaops.obj
bcc -O -Z -I..\inc -c -DPROFILE tiler.c
tlib /C ..\lib\lrprofbc.lib -+lores.obj -+tiler.obj -+timer.obj -+memops.obj -+cgaops.obj
bcc -O -Z -I..\inc tiledemo.c ..\lib\loresbc.lib
bcc -O -Z -I..\inc sprtdemo.c ..\lib\loresbc.lib
del *.obj
del ..\lib\*.bak
