object = main.o sche.o config.o comm.o

test:$(object)
	cc $(object) -lpthread -lrt -o test
	@echo $(object) 1111111

main.o:type.h sche.h config.h comm.h

sche.o:type.h sche.h config.h comm.h

config.o:type.h sche.h config.h comm.h

comm.o:type.h sche.h config.h comm.h

%:
	@echo $(object) 1111111

.PHONY:clean

clean:
	-rm test *.o
