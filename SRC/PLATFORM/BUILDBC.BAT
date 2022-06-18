masm /ml keyboard;
bcc -O -Z -I..\..\inc -DCGA_SNOW -eplatbc.exe platform.c keyboard.obj ..\..\lib\loresbc.lib
bcc -O -Z -I..\..\inc -DCGA_SNOW -DPROFILE -eplatpbc.exe platform.c keyboard.obj ..\..\lib\lrprofbc.lib
del *.obj
