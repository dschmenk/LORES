masm /ml keyboard;
cl /Ox /I..\..\inc /DCGA_SNOW repelz.c keyboard.obj ..\..\lib\lores.lib
cl /Ox /I..\..\inc /DCGA_SNOW /DPROFILE /o repelzp.exe repelz.c keyboard.obj ..\..\lib\lrprof.lib
del *.obj
