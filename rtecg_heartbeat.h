#ifndef __RTECG_HEARTBEAT_H__
#define __RTECG_HEARTBEAT_H__

#ifdef __cplusplus
extern "C" {
#endif

void rtecg_heartbeat_set_mac(char mac[6]);
void rtecg_heartbeat_set_ip_local(char ip_local[4]);
void rtecg_heartbeat_set_port_local(uint32_t port_local);
void rtecg_heartbeat_set_ip_remote(char ip_remote[4]);
void rtecg_heartbeat_set_port_remote(uint32_t port_remote);
void rtecg_heartbeat_set_self(char *self, int selflen);
void rtecg_heartbeat_init(char mac[6], char ip_local[4], uint32_t port_local, char ip_remote[4], uint32_t port_remote, char *self, int selflen);
int rtecg_heartbeat_len(void);
char *rtecg_heartbeat_bndl(void);


#ifdef __cplusplus
}
#endif

#endif
