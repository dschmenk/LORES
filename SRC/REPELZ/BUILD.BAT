cl /Ox /DUSE_GETCH /I..\..\inc /DCGA_SNOW repelz.c ..\..\lib\lores.lib
cl /Ox /DUSE_GETCH /I..\..\inc /DCGA_SNOW /DPROFILE /o repelzp.exe repelz.c ..\..\lib\lrprof.lib
del *.obj
