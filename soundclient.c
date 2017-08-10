#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <string.h>
#include <linux/limits.h>

#define SOCKETNAME "/tmp/socket_soundd"

static char buf[2*PATH_MAX + 4]; /* cmd + ' ' + alias + ' ' + file + null */

static int soundc_send()
{
    int sock;
    struct sockaddr_un name;

    sock = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("opening datagram socket");
        exit(1);
    }

    name.sun_family = AF_UNIX;
    strcpy(name.sun_path, SOCKETNAME);

    if (sendto(sock, buf, strlen(buf), 0,
        (struct sockaddr *)&name, sizeof(struct sockaddr_un)) < 0) {
        perror("sending datagram message");
    }
    close(sock);

    return 0;

}

int soundc_register(const char *alias, const char *filename)
{
  size_t alias_len, filename_len;

  alias_len = strlen(alias);

  buf[0] = 'r';
  buf[1] = ' ';
  strcpy(&buf[2], alias);
  buf[2 + alias_len] = ' ';

  strcpy(&buf[2 + alias_len + 1], filename);

  return soundc_send();
  
}

int soundc_play(const char *alias)
{
  size_t alias_len;

  buf[0] = 'p';
  buf[1] = ' ';
  strcpy(&buf[2], alias);

  return soundc_send();

}

