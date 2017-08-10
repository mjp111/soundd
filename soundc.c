#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>

#include "soundclient.h"

void usage(char **argv)
{
  printf("soundd test client. Usage:\n");
  printf("%s: register <alias> <filename>\n", argv[0]);
  printf("%s: play <alias>\n", argv[0]);
  exit(1);

}

int main(int argc, char **argv)
{

  if (argc < 3)
    usage(argv);

  if (!strcmp(argv[1], "register"))
    soundc_register(argv[2], argv[3]);

  if (!strcmp(argv[1], "play"))
    soundc_play(argv[2]);

  return 0;
}
