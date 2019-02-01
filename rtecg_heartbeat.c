#include <stdint.h>
#include <rtecg_heartbeat.h>
#include <lib/libo/osc_byteorder.h>

char _rtecg_heartbeat_mac[] = {0, 0, 0, 0, '/', 'm', 'a', 'c', 0, 0, 0, 0, ',', 'i', 'i', 'i', 'i', 'i', 'i', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
char _rtecg_heartbeat_ip[] = {0, 0, 0, 0, '/', 'i', 'p', 0, ',', 'i', 'i', 'i', 'i', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
char _rtecg_heartbeat_port[] = {0, 0, 0, 0, '/', 'p', 'o', 'r', 't', 0, 0, 0, ',', 'i', 0, 0, 0, 0, 0, 0};
char _rtecg_heartbeat_pfx[] = {0, 0, 0, 0, '/', 'p', 'x', 0, ',', 's', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
int _rtecg_heartbeat_bndllen = 16 + sizeof(_rtecg_heartbeat_mac) + sizeof(_rtecg_heartbeat_ip) + sizeof(_rtecg_heartbeat_port) + sizeof(_rtecg_heartbeat_pfx);
char _rtecg_heartbeat_bndl[16 + sizeof(_rtecg_heartbeat_mac) + sizeof(_rtecg_heartbeat_ip) + sizeof(_rtecg_heartbeat_port) + sizeof(_rtecg_heartbeat_pfx)];

void rtecg_heartbeat_init(char mac[6], char ip[4], uint32_t receive_port, char *pfx, int pfxlen)
{
	memset(_rtecg_heartbeat_bndl, 0, _rtecg_heartbeat_bndllen);
	memcpy(_rtecg_heartbeat_bndl, "#bundle\0", 8);
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
	
	*((int32_t *)_rtecg_heartbeat_ip) = hton32(sizeof(_rtecg_heartbeat_ip) - 4);
	p = _rtecg_heartbeat_ip + 16;
	*((int32_t *)p) = hton32((int32_t)ip[0]);
	p += 4;
	*((int32_t *)p) = hton32((int32_t)ip[1]);
	p += 4;
	*((int32_t *)p) = hton32((int32_t)ip[2]);
	p += 4;
	*((int32_t *)p) = hton32((int32_t)ip[3]);
	p += 4;
	
	*((int32_t *)_rtecg_heartbeat_port) = hton32(sizeof(_rtecg_heartbeat_port) - 4);
	p = _rtecg_heartbeat_port + 16;
	*((int32_t *)p) = hton32(receive_port);

	*((int32_t *)_rtecg_heartbeat_pfx) = hton32(sizeof(_rtecg_heartbeat_pfx) - 4);
	p = _rtecg_heartbeat_pfx + 12;
	strncpy(p, pfx, 16);

	p = _rtecg_heartbeat_bndl + 16;
	memcpy(p, _rtecg_heartbeat_mac, sizeof(_rtecg_heartbeat_mac));
	p += sizeof(_rtecg_heartbeat_mac);
	memcpy(p, _rtecg_heartbeat_ip, sizeof(_rtecg_heartbeat_ip));
	p += sizeof(_rtecg_heartbeat_ip);
	memcpy(p, _rtecg_heartbeat_port, sizeof(_rtecg_heartbeat_port));
	p += sizeof(_rtecg_heartbeat_port);
	memcpy(p, _rtecg_heartbeat_pfx, sizeof(_rtecg_heartbeat_pfx));
	p += sizeof(_rtecg_heartbeat_pfx);
}

int rtecg_heartbeat_len(void)
{
	return _rtecg_heartbeat_bndllen;
}

char *rtecg_heartbeat_bndl(void)
{
	return _rtecg_heartbeat_bndl;
}
