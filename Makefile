CC=gcc
CFLAGS=-lm -O3
OUT=YASL
OBJECTS=interpreter/VM.o interpreter/builtins/builtins.o interpreter/constant/constant.o interpreter/hashtable/hashtable.o interpreter/list/list.o interpreter/prime/prime.o interpreter/string8/string8.o interpreter/vtable/vtable.o

YASL: $(OBJECTS)
	$(CC) $(OBJECTS) interpreter/main.c $(CFLAGS) -o $(OUT)

clean:
	rm $(OUT) $(OBJECTS)

remake: clean YASL

interpreter/VM.o:
	$(CC) interpreter/VM.c -c -o interpreter/VM.o

interpreter/builtins/builtins.o:
	$(CC) interpreter/builtins/builtins.c $(CFLAGS) -c -o interpreter/builtins/builtins.o

interpreter/constant/constant.o:
	$(CC) interpreter/constant/constant.c $(CFLAGS) -c -o interpreter/constant/constant.o

interpreter/hashtable/hashtable.o:
	$(CC) interpreter/hashtable/hashtable.c $(CFLAGS) -c -o interpreter/hashtable/hashtable.o

interpreter/list/list.o:
	$(CC) interpreter/list/list.c $(CFLAGS) -c -o interpreter/list/list.o

interpreter/prime/prime.o:
	$(CC) interpreter/prime/prime.c $(CFLAGS) -c -o interpreter/prime/prime.o

interpreter/string8/string8.o:
	$(CC) interpreter/string8/string8.c $(CFLAGS) -c -o interpreter/string8/string8.o

interpreter/vtable/vtable.o:
	$(CC) interpreter/vtable/vtable.c $(CFLAGS) -c -o interpreter/vtable/vtable.o