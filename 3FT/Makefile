all: ft

ft: ft.o nodeFT.o dynarray.o path.o
	gcc217 ft.o nodeFT.o dynarray.o path.o -o ft

ft.o: ft.c ft.h nodeFT.h dynarray.h path.h a4def.h
	gcc217 -c ft.c

nodeFT.o: nodeFT.c nodeFT.h dynarray.h path.h a4def.h
	gcc217 -c nodeFT.c

path.o: path.c path.h dynarray.h a4def.h
	gcc217 -c path.c

dynarray.o: dynarray.c dynarray.h
	gcc217 -c dynarray.c

clean:
	rm -f ft ft.o nodeFT.o path.o dynarray.o