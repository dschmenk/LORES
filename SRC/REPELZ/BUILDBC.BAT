bcc -O -Z -I..\..\inc -c keyboard.c
bcc -O -Z -I..\..\inc -DCGA_SNOW -erepelzbc.exe repelz.c keyboard.obj ..\..\lib\loresbc.lib
bcc -O -Z -I..\..\inc -DCGA_SNOW -DPROFILE -erepelpbc.exe repelz.c keyboard.obj ..\..\lib\lrprofbc.lib
del *.obj
