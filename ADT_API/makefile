# This make file builds the AltaAPI Layer 1 source files into a shared object file
all :
	gcc -Wall -fPIC -c *.c
	gcc -shared -Wl,-soname,libADT_L1_Linux_x86_64.so -o libADT_L1_Linux_x86_64.so *.o

clean :
	rm -f *~ core *.o *.so

