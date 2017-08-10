#ifndef _SOUNDCLIENT_H
#define _SOUNDCLIENT_H


#ifdef __cplusplus
extern "C" {
#endif

int soundc_register(const char *alias, const char *filename);
int soundc_play(const char *alias);

#ifdef __cplusplus
}
#endif

#endif
