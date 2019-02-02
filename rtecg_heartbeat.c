#include <stdint.h>
#include <rtecg_heartbeat.h>
#include <lib/libo/osc_byteorder.h>

char _rtecg_heartbeat_mac[] = {0, 0, 0, 0, '/', 'm', 'a', 'c', 0, 0, 0, 0, ',', 'i', 'i', 'i', 'i', 'i', 'i', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
char _rtecg_heartbeat_ip_local[] = {0, 0, 0, 0, '/', 'i', 'p', '/', 'l', 'o', 'c', 'a', 'l', 0, 0, 0, ',', 'i', 'i', 'i', 'i', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
char _rtecg_heartbeat_port_local[] = {0, 0, 0, 0, '/', 'p', 'o', 'r', 't', '/', 'l', 'o', 'c', 'a', 'l', 0, ',', 'i', 0, 0, 0, 0, 0, 0};
char _rtecg_heartbeat_ip_remote[] = {0, 0, 0, 0, '/', 'i', 'p', '/', 'r', 'e', 'm', 'o', 't', 'e', 0, 0, ',', 'i', 'i', 'i', 'i', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
char _rtecg_heartbeat_port_remote[] = {0, 0, 0, 0, '/', 'p', 'o', 'r', 't', '/', 'r', 'e', 'm', 'o', 't', 'e', 0, 0, 0, 0, ',', 'i', 0, 0, 0, 0, 0, 0};
char _rtecg_heartbeat_self[] = {0, 0, 0, 0, '/', 's', 'e', 'l', 'f', 0, 0, 0, ',', 's', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
int _rtecg_heartbeat_bndllen = 16 + sizeof(_rtecg_heartbeat_mac) + sizeof(_rtecg_heartbeat_ip_local) + sizeof(_rtecg_heartbeat_port_local) + sizeof(_rtecg_heartbeat_ip_remote) + sizeof(_rtecg_heartbeat_port_remote) + sizeof(_rtecg_heartbeat_self);
char _rtecg_heartbeat_bndl[16 + sizeof(_rtecg_heartbeat_mac) + sizeof(_rtecg_heartbeat_ip_local) + sizeof(_rtecg_heartbeat_port_local) + sizeof(_rtecg_heartbeat_ip_remote) + sizeof(_rtecg_heartbeat_port_remote) + sizeof(_rtecg_heartbeat_self)];

void rtecg_heartbeat_set_mac(char mac[6])
{
	*((int32_t *)_rtecg_heartbeat_mac) = hton32(sizeof(_rtecg_heartbeat_mac) - 4);
	char *p = _rtecg_heartbeat_mac + 20;
	*((int32_t *)p) = hton32((int32_t)mac[0]);
	p += 4;
	*((int32_t *)p) = hton32((int32_t)mac[1]);
	p += 4;
	*((int32_t *)p) = hton32((int32_t)mac[2]);
	p += 4;
	*((int32_t *)p) = hton32((int32_t)mac[3]);
	p += 4;
	*((int32_t *)p) = hton32((int32_t)mac[4]);
	p += 4;
	*((int32_t *)p) = hton32((int32_t)mac[5]);

	p = _rtecg_heartbeat_bndl + 16;
	memcpy(p, _rtecg_heartbeat_mac, sizeof(_rtecg_heartbeat_mac));
}

void rtecg_heartbeat_set_ip_local(char ip_local[4])
{
	*((int32_t *)_rtecg_heartbeat_ip_local) = hton32(sizeof(_rtecg_heartbeat_ip_local) - 4);
	char *p = _rtecg_heartbeat_ip_local + 24;
	*((int32_t *)p) = hton32((int32_t)ip_local[0]);
	p += 4;
	*((int32_t *)p) = hton32((int32_t)ip_local[1]);
	p += 4;
	*((int32_t *)p) = hton32((int32_t)ip_local[2]);
	p += 4;
	*((int32_t *)p) = hton32((int32_t)ip_local[3]);

	p = _rtecg_heartbeat_bndl + 16 + sizeof(_rtecg_heartbeat_mac);
	memcpy(p, _rtecg_heartbeat_ip_local, sizeof(_rtecg_heartbeat_ip_local));
}

void rtecg_heartbeat_set_port_local(uint32_t port_local)
{
	*((int32_t *)_rtecg_heartbeat_port_local) = hton32(sizeof(_rtecg_heartbeat_port_local) - 4);
	char *p = _rtecg_heartbeat_port_local + 20;
	*((int32_t *)p) = hton32(port_local);

	p = _rtecg_heartbeat_bndl + 16 + sizeof(_rtecg_heartbeat_mac) + sizeof(_rtecg_heartbeat_ip_local);
	memcpy(p, _rtecg_heartbeat_port_local, sizeof(_rtecg_heartbeat_port_local));
}

void rtecg_heartbeat_set_ip_remote(char ip_remote[4])
{
	*((int32_t *)_rtecg_heartbeat_ip_remote) = hton32(sizeof(_rtecg_heartbeat_ip_remote) - 4);
	char *p = _rtecg_heartbeat_ip_remote + 24;
	*((int32_t *)p) = hton32((int32_t)ip_remote[0]);
	p += 4;
	*((int32_t *)p) = hton32((int32_t)ip_remote[1]);
	p += 4;
	*((int32_t *)p) = hton32((int32_t)ip_remote[2]);
	p += 4;
	*((int32_t *)p) = hton32((int32_t)ip_remote[3]);

	p = _rtecg_heartbeat_bndl + 16 + sizeof(_rtecg_heartbeat_mac) + sizeof(_rtecg_heartbeat_ip_local) + sizeof(_rtecg_heartbeat_port_local);
	memcpy(p, _rtecg_heartbeat_ip_remote, sizeof(_rtecg_heartbeat_ip_remote));
}

void rtecg_heartbeat_set_port_remote(uint32_t port_remote)
{
	*((int32_t *)_rtecg_heartbeat_port_remote) = hton32(sizeof(_rtecg_heartbeat_port_remote) - 4);
	char *p = _rtecg_heartbeat_port_remote + 24;
	*((int32_t *)p) = hton32(port_remote);

	p = _rtecg_heartbeat_bndl + 16 + sizeof(_rtecg_heartbeat_mac) + sizeof(_rtecg_heartbeat_ip_local) + sizeof(_rtecg_heartbeat_port_local) + sizeof(_rtecg_heartbeat_ip_remote);
	memcpy(p, _rtecg_heartbeat_port_remote, sizeof(_rtecg_heartbeat_port_remote));
}

void rtecg_heartbeat_set_self(char *self, int selflen)
{
	*((int32_t *)_rtecg_heartbeat_self) = hton32(sizeof(_rtecg_heartbeat_self) - 4);
	char *p = _rtecg_heartbeat_self + 16;
	strncpy(p, self, 16);

	p = _rtecg_heartbeat_bndl + 16 + sizeof(_rtecg_heartbeat_mac) + sizeof(_rtecg_heartbeat_ip_local) + sizeof(_rtecg_heartbeat_port_local) + sizeof(_rtecg_heartbeat_ip_remote) + sizeof(_rtecg_heartbeat_port_remote);
	memcpy(p, _rtecg_heartbeat_self, sizeof(_rtecg_heartbeat_self));
}

void rtecg_heartbeat_init(char mac[6], char ip_local[4], uint32_t port_local, char ip_remote[4], uint32_t port_remote, char *self, int selflen)
{
	memset(_rtecg_heartbeat_bndl, 0, _rtecg_heartbeat_bndllen);
	memcpy(_rtecg_heartbeat_bndl, "#bundle\0", 8);
	rtecg_heartbeat_set_mac(mac);
	rtecg_heartbeat_set_ip_local(ip_local);
	rtecg_heartbeat_set_port_local(port_local);
	rtecg_heartbeat_set_ip_remote(ip_remote);
	rtecg_heartbeat_set_port_remote(port_remote);
	rtecg_heartbeat_set_self(self, selflen);
}

int rtecg_heartbeat_len(void)
{
	return _rtecg_heartbeat_bndllen;
}

char *rtecg_heartbeat_bndl(void)
{
	return _rtecg_heartbeat_bndl;
}
