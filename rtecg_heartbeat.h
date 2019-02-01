#ifndef __RTECG_HEARTBEAT_H__
#define __RTECG_HEARTBEAT_H__

#ifdef __cplusplus
extern "C" {
#endif

void rtecg_heartbeat_init(char mac[6], char ip[4], uint32_t receive_port, char *pfx, int pfxlen);
int rtecg_heartbeat_len(void);
char *rtecg_heartbeat_bndl(void);


#ifdef __cplusplus
}
#endif

#endif
