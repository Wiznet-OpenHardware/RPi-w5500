# OBJS_w5x00_dhcp_dns = w5x00_dhcp_dns.o wizchip_conf.o w5500.o socket.o dhcp.o dns.o
# OBJS_w5x00_loopback = w5x00_loopback.o wizchip_conf.o w5500.o socket.o loopback.o

# LDFLAGS = -lgpiod

# all: w5x00_dhcp_dns w5x00_loopback

# w5x00_dhcp_dns: $(OBJS_w5x00_dhcp_dns)
# 	gcc -o $@ $(OBJS_w5x00_dhcp_dns) $(LDFLAGS)

# w5x00_loopback: $(OBJS_w5x00_loopback)
# 	gcc -o $@ $(OBJS_w5x00_loopback) $(LDFLAGS)

# .c.o:
# 	gcc -c $<

# clean:
# 	rm -f $(OBJS_w5x00_dhcp_dns) $(OBJS_W5x00_loopback) w5x00_dhcp_dns w5x00_loopback
# 	rm -f *.o


OBJS_w5x00_loopback = w5x00_loopback.o w5500.o wizchip_conf.o wiz_socket.o loopback.o
OBJS_w5x00_dhcp_dns = w5x00_dhcp_dns.o wizchip_conf.o w5500.o wiz_socket.o dhcp.o dns.o

LDFLAGS = -lgpiod

all: w5x00_loopback w5x00_dhcp_dns

w5x00_loopback: $(OBJS_w5x00_loopback)
	gcc -o $@ $(OBJS_w5x00_loopback) $(LDFLAGS)

w5x00_dhcp_dns: $(OBJS_w5x00_dhcp_dns)
	gcc -o $@ $(OBJS_w5x00_dhcp_dns) $(LDFLAGS)

.c.o:
	gcc -c $<

clean:
	rm -f $(OBJS_w5x00_dhcp_dns) $(OBJS_W5x00_loopback) w5x00_loopback w5x00_dhcp_dns
	rm -f *.o
