OBJS=w5x00_loopback.o wizchip_conf.o w5500.o socket.o loopback.o
SRCS=$(OBJS:.o=c)
LDFLAGS=-lwiringPi
LDFLAGS2=-lwiringPiDev
w5x00_loopback : $(OBJS)
	gcc -o $@ $(OBJS) $(LDFLAGS)
.c.o : #(SRCS)
	gcc -c $<
clean :
	rm $(OBJS) w5x00_loopback
