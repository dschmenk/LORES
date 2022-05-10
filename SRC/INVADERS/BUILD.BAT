masm /ml keyboard;
cl /Ox /I..\..\inc /DCGA_SNOW invaders.c keyboard.obj ..\..\lib\lores.lib
cl /Ox /I..\..\inc /DCGA_SNOW /DPROFILE /o invaderp.exe invaders.c keyboard.obj ..\..\lib\lrprof.lib
del *.obj
