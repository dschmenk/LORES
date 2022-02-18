masm /ml memops;
masm /ml cgaops;
masm /ml timer;
bcc -O -Z -I..\inc -c lores.c
bcc -O -Z -I..\inc -c tiler.c
tlib /C ..\lib\loresbc.lib -+lores.obj -+tiler.obj -+timer.obj -+memops.obj -+cgaops.obj
bcc -O -Z -I..\inc -c -DCGA_SNOW lores.c
bcc -O -Z -I..\inc -c -DCGA_SNOW tiler.c
tlib /C ..\lib\lrsnowbc.lib -+lores.obj -+tiler.obj -+timer.obj -+memops.obj -+cgaops.obj
bcc -O -Z -I..\inc -c -DPROFILE lores.c
bcc -O -Z -I..\inc -c -DPROFILE tiler.c
tlib /C ..\lib\lrprofbc.lib -+lores.obj -+tiler.obj -+timer.obj -+memops.obj -+cgaops.obj
bcc -O -Z -I..\inc -c -DCGA_SNOW -DPROFILE lores.c
bcc -O -Z -I..\inc -c -DCGA_SNOW -DPROFILE tiler.c
tlib /C ..\lib\lrprsnbc.lib -+lores.obj -+tiler.obj -+timer.obj -+memops.obj -+cgaops.obj
bcc -O -Z -I..\inc -o..\bin\tiledemo.exe tiledemo.c ..\lib\loresbc.lib
bcc -O -Z -I..\inc -o..\bin\tilesnow.exe -DCGA_SNOW tiledemo.c ..\lib\lrsnowbc.lib
bcc -O -Z -I..\inc -o..\bin\tileprof.exe -DPROFILE tiledemo.c ..\lib\lrprofbc.lib
bcc -O -Z -I..\inc -o..\bin\tilepsno.exe -DCGA_SNOW -DPROFILE tiledemo.c ..\lib\lrprsnbc.lib
bcc -O -Z -I..\inc -o..\bin\sprtdemo.exe sprtdemo.c ..\lib\loresbc.lib
bcc -O -Z -I..\inc -o..\bin\sprtprof.exe -DPROFILE sprtdemo.c ..\lib\lrprofbc.lib
del *.obj
del ..\lib\*.bak
