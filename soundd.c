#include <stdio.h>
#include <stdlib.h>
#include <error.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <alsa/asoundlib.h>
#include <linux/limits.h>

#define RATE (16000)
#define BITS (16)
#define CHANNELS (1)
#define SAMPLE_SIZE (BITS * CHANNELS / 8)
#define DEVICENAME "default"
#define WAV_HDRSIZE (44)

#define SOCKETNAME "/tmp/socket_soundd"
#define PROGNAME "soundd v0.1"

static snd_pcm_t *handle;
static int sound_socket;

static snd_pcm_uframes_t  _buffersize;
static snd_pcm_uframes_t period_frames;

typedef struct _sound_item {
  char name[PATH_MAX];
  char file[PATH_MAX];
  struct _sound_item *prev;
} sound_item_t;

sound_item_t *sounds;

int write_to_pcm(char *ptr, int n)
{

  snd_pcm_uframes_t samples;
  int nwrite;

  snd_pcm_uframes_t target_frames;
  samples = n / SAMPLE_SIZE;
  
  if ((nwrite = snd_pcm_writei(handle, ptr, samples)) < 0) {
    fprintf (stderr, "write to audio interface failed (%s)\n",
             snd_strerror (nwrite));
    snd_pcm_drop(handle);
    snd_pcm_prepare(handle);    
  }
  return n;
}

static int configure_buffers_periods(snd_pcm_hw_params_t *params)
{

  snd_pcm_uframes_t     buffersize, periodsize;
  unsigned int val, val2, targetbuffertime;
  int err;

  /*
    Try to configure 4*20ms buffer 
  */
  targetbuffertime = 60000; 
 __again:
  targetbuffertime += 20000; /* 20ms */

  if (targetbuffertime > 1000000) {
    printf("Cannot configure buffer time..giving up.\n");
    return -1;
  }
    
  printf("  ..trying to configure buffer size of %u ms\n",targetbuffertime/1000);
  err = snd_pcm_hw_params_set_buffer_time_near(handle, params, &targetbuffertime, 0);
  if (err < 0) 
    goto __again;

  snd_pcm_hw_params_get_buffer_size(params, &buffersize);

  val = 20000;

  err = snd_pcm_hw_params_set_period_time_near(handle, params, &val, 0);

  if (err < 0)
    goto __again;
  
  snd_pcm_hw_params_get_period_size(params, &periodsize, NULL);
  snd_pcm_hw_params_get_buffer_size(params, &buffersize);
  if (periodsize * 2 > buffersize)
    goto __again;
  
  _buffersize = buffersize;
  period_frames = periodsize;

  snd_pcm_hw_params_get_period_time(params, &val, 0);
  snd_pcm_hw_params_get_buffer_time(params, &val2, 0);
  
  printf("  ..configure OK: period = %u ms, buffer = %u ms\n", val/1000, val2/1000);
  
  return 0;
}

int open_pcm()
{
  snd_pcm_hw_params_t *hw_params;
  snd_pcm_sw_params_t *sw_params;
  snd_pcm_stream_t     stream = SND_PCM_STREAM_PLAYBACK;
  snd_pcm_format_t     pcmformat = SND_PCM_FORMAT_S16_LE;
  snd_pcm_uframes_t boundary;
  int rate = RATE;
  int err;

  if ((err = snd_pcm_open (&handle, DEVICENAME, stream, 0)) < 0) {
    fprintf (stderr, "cannot open audio device %s (%s)\n", 
             "default",
             snd_strerror (err));
    return -1;
  }

  if ((err = snd_pcm_hw_params_malloc (&hw_params)) < 0) {
    fprintf (stderr, "cannot allocate hardware parameter structure (%s)\n",
             snd_strerror (err));
    return -1;
  }

  snd_pcm_sw_params_alloca(&sw_params);
                                 
  if ((err = snd_pcm_hw_params_any (handle, hw_params)) < 0) {
    fprintf (stderr, "cannot initialize hardware parameter structure (%s)\n",
             snd_strerror (err));
    return -1;
  }
        
  if ((err = snd_pcm_hw_params_set_access (handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
    fprintf (stderr, "cannot set access type (%s)\n",
             snd_strerror (err));
    return -1;
  }
        
  if ((err = snd_pcm_hw_params_set_format (handle, hw_params, pcmformat)) < 0) {
    fprintf (stderr, "cannot set sample format (%s)\n",
             snd_strerror (err));
    return -1;
  }

  if ((err = snd_pcm_hw_params_set_rate_near (handle, hw_params, &rate, 0)) < 0) {
    fprintf (stderr, "cannot set sample rate (%s)\n",
             snd_strerror (err));
    return -1;
  }

  if ((err = snd_pcm_hw_params_set_channels (handle, hw_params, CHANNELS)) < 0) {
    fprintf (stderr, "cannot set channel count (%s)\n",
             snd_strerror (err));
    return -1;
  }

 if (configure_buffers_periods(hw_params) < 0)
    return -1;

 if ((err = snd_pcm_hw_params (handle, hw_params)) < 0) {
    fprintf (stderr, "cannot set parameters (%s)\n",
             snd_strerror (err));
    return -1;
  }
        
  snd_pcm_hw_params_free (hw_params);

  snd_pcm_sw_params_current(handle, sw_params);

  snd_pcm_sw_params_get_boundary(sw_params, &boundary);

  snd_pcm_sw_params_set_silence_threshold(handle, sw_params, 0);
  snd_pcm_sw_params_set_silence_size(handle, sw_params, boundary);

  snd_pcm_sw_params_set_start_threshold(handle, sw_params, 0);
  snd_pcm_sw_params_set_stop_threshold(handle, sw_params, boundary);
  
  if ((err = snd_pcm_sw_params(handle, sw_params)) < 0) {
    fprintf(stderr, "cannot set sw parameters (%s)\n", snd_strerror(err));
      return -1;
  }

    return 0;
}

static int process_wav(const char *filename)
{

  char buf[1024];
  ssize_t n;

  snd_pcm_reset(handle);
  int fd = open(filename, O_RDONLY);

  if (fd < 0)
    return fd;

  lseek(fd, WAV_HDRSIZE, SEEK_SET);

  while(1) {
    n = read(fd, buf, 1024);
    if (n <= 0)
      break;

    write_to_pcm(buf, n);
  }

  close(fd);

  return 0;      
}

static void play_sound(const char *ptr)
{
  sound_item_t *item;

  while (*ptr && *ptr == ' ')
    ptr++;
  
  if (!ptr)
    return;
      
  item = sounds;

  while (item) {

    if (!strcmp(item->name, ptr)) 
      break;
    	
    item = item->prev;
  }

  if (item)
    process_wav(item->file);
}

static void register_sound(const char *ptr)
{

  sound_item_t *item;

  item = (sound_item_t *)malloc(sizeof(sound_item_t));
  if (!item)
    return;

  int n = sscanf(ptr, "%s %s", item->name, item->file);

  if (n == 2) {
    printf("registering %s as %s\n", item->name, item->file);
    item->prev = sounds;
    sounds = item;
  }
  else
    free(item);

}

static int process_cmd()
{
  char buf[2*PATH_MAX + 3]; /* cmd + ' ' + alias + ' ' + file */
  int ret;

  memset(buf, 0, sizeof(buf));
  ret = read(sound_socket, buf, sizeof(buf)-1);

  if (ret <= 0)
    return 0;

  switch (buf[0]) {
  case 'r':
    register_sound(buf+1);
    break;

  case 'p':
    play_sound(buf+1);
    break;
  }

}

static void create_socket()
{
    struct sockaddr_un name;

    unlink(SOCKETNAME);

    sound_socket = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (sound_socket < 0) {
      perror("opening datagram socket");
      exit(1);
    }
        
    name.sun_family = AF_UNIX;
    strcpy(name.sun_path, SOCKETNAME);
        
    if (bind(sound_socket, (struct sockaddr *) &name, sizeof(struct sockaddr_un))) {
      perror("binding name to datagram socket");
      exit(1);
    }    
}

int main(int argc, char **argv)
{
  fd_set active_ips, ips;
  int ret;

  printf("%s %s\n", PROGNAME, "starting...");

  create_socket();

  if (open_pcm() < 0) 
    exit(1);

  FD_ZERO(&active_ips);
  FD_SET(sound_socket, &active_ips);

  printf("%s %s\n", PROGNAME, "ready.");

  while (1) {
    memcpy(&ips, &active_ips, sizeof(fd_set));
    
    if ((ret = select(sound_socket+1, &ips, NULL, NULL, NULL)) < 0) {
      perror("select() failed");
      continue;
    }

    process_cmd();

  }
  return 0;
}



