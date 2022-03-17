bcc -O -Z -I..\..\inc -e..\..\bin\mapedit.exe mapedit.c ..\..\lib\loresbc.lib
bcc -O -Z -I..\..\inc -e..\..\bin\sprited.exe sprited.c ..\..\lib\loresbc.lib
bcc -O -Z -I..\..\inc -e..\..\bin\sprotate.exe sprotate.c ..\..\lib\loresbc.lib
del *.obj
