masm /ml keyboard;
cl /Ox /I..\..\inc /DCGA_SNOW platform.c keyboard.obj ..\..\lib\lores.lib
cl /Ox /I..\..\inc /DCGA_SNOW /DPROFILE /o platfrmp.exe platform.c keyboard.obj ..\..\lib\lrprof.lib
del *.obj
