all: diskinfo disklist diskget diskput

diskinfo: diskinfo.c disk.h
	gcc diskinfo.c -Wall -Wno-pointer-sign -o diskinfo

disklist: disklist.c disk.h
	gcc disklist.c -Wall -Wno-pointer-sign -o disklist

diskget: diskget.c disk.h
	gcc diskget.c -Wall -Wno-pointer-sign -o diskget

diskput: diskput.c disk.h
	gcc diskput.c -Wall -Wno-pointer-sign -o diskput
