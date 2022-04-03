bcc -O -Z -I..\..\inc -DCGA_SNOW -erepelzbc.exe repelz.c ..\..\lib\loresbc.lib
bcc -O -Z -I..\..\inc -DCGA_SNOW -DPROFILE -erepelpbc.exe repelz.c ..\..\lib\lrprofbc.lib
del *.obj
