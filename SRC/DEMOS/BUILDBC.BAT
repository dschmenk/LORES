bcc -O -Z -I..\..\inc -etiledebc.exe tiledemo.c ..\..\lib\loresbc.lib
bcc -O -Z -I..\..\inc -esprtdebc.exe sprtdemo.c ..\..\lib\loresbc.lib
bcc -O -Z -I..\..\inc -DPROFILE -etileprbc.exe tiledemo.c ...\.\lib\lrprofbc.lib
bcc -O -Z -I..\..\inc -DPROFILE -esprtprbc.exe sprtdemo.c ..\..\lib\lrprofbc.lib
del *.obj
