CC=gcc
LD=LD_RUN_PATH=$(shell pwd) gcc
LDR=$$LD_RUN_PATH
CFLAGS=-std=gnu99 -Wall -Wextra -Werror -g
LFLAGS=-Wl,-export-dynamic -lbsd
EXC=3 4 5 6 7 8 1
TRASH=*.o $(EXC) .depend 1RTL 1LTL lib1.so lib2.so 2Test

all: .depend $(EXC) 1RTL 1LTL lib2.so

clean : 
	-rm -f $(TRASH)

dist : clean
	cd ../ && rm -f Czapl*prog3.tgz && tar czf Czaplinski_Lukasz_prog3.tgz Czap*prog3

.PHONY : all clean $(EXC)

depend : .depend
	@echo "done"

.depend: cmd = gcc -MM -MF depend $(var).c && echo $(var)": aux"$(var) >> depend	&& cat depend >> .depend ;
.depend:
	 @echo "Generating dependencies..."
	 @-rm -f .depend
	 @$(foreach var, $(EXC), $(cmd))
	 @rm -f depend
		
-include .depend

aux% : %.o
	@echo "Generating "$*"..."
	$(CC) -o $* $< $(LFLAGS)

libP%.o : libP%.c libP%.h
	$(CC) -fPIC -shared -c $(CFLAGS) -export-dynamic libP$*.c

lib1.so : libP1.o libP2.o libP3.o
	$(LD) -fPIC -shared -o lib1.so libP1.o libP2.o libP3.o

lib2.so : lib2.c
	$(CC) -fPIC -shared -c $(CFLAGS) -export-dynamic lib2.c
	$(LD) -fPIC -shared -o lib2.so lib2.o

1LTL : 1.c lib1.so
	$(CC) $(CFLAGS) -DLTL -export-dynamic -c 1.c
	$(LD) $(LFLAGS) -o 1LTL 1.o lib1.so

1RTL : 1.c
	LD_RUN_PATH=$(shell pwd) $(CC) $(CFLAGS) $(LFLAGS) -ldl -DRTL -o 1RTL 1.c

2Test : lib2.so 2.c
	$(CC) $(CFLAGS) -export-dynamic -c 2.c
	$(LD) $(LFLAGS) -o 2Test 2.o lib2.so
	
LDTrick : lib2.so
