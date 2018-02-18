
/* Use the newer ALSA API */
#define ALSA_PCM_NEW_HW_PARAMS_API

#include "fcd.h"
#include "fcdhidcmd.h"

#include <alsa/asoundlib.h>

#include <math.h>

// based on the #defines for TLGE_P10_0DB etc. from fcdhidcmd.h :
double lnagainvalues[]={-5.0,-2.5,-999,-999,0,2.5,5,7.5,10,12.5,15,17.5,20,25,30};

inline double sqr(double x) { return x*x; }

void writeAscii(FILE *fdo, short int *buf, int n)
{
  int c=0;
  for (c=0; c<n; c++) fprintf(fdo, "%d\n", buf[c]);
}

/********************************************************************/
/** statistics **/

struct statT {
  int nb;
  double sx;
  double sx2;
};

void stats_sum(struct statT *st, short int *buf, int n)
{
  int c=0;
  for (c=0; c<n; c++) {
    st->sx += buf[c];
    st->sx2 += sqr(buf[c]);
  }
  st->nb += n;
}

inline double stats_mean(struct statT *st)
{
  return st->sx / st->nb;
}

inline double stats_var(struct statT *st, double m)
{
  return (st->sx2-sqr(m)*st->nb) / (st->nb-1.);
}

void stats_edit(FILE *fdo, struct statT *st)
{
  fprintf(fdo, "nb %d\n", st->nb);
  double m=stats_mean(st);
  fprintf(fdo, "mean %g\n", m);
  double var=stats_var(st, m);
  fprintf(fdo, "var %g\n", var);
  fprintf(fdo, "sdev %g\n", sqrt(var));
}

int stats_high(struct statT *st, double sql)
{
  return stats_var(st, stats_mean(st)) > sql;
}

/****************************************************************/
/** SDR stuff **/

snd_pcm_t *handle;
snd_pcm_uframes_t frames;
snd_pcm_hw_params_t *params;

void dumpSdr()
{
  // TODO beware leaves the funcube in a weird state
  char version[256];
  fcdGetFwVerStr(version);
  printf("# funcube version %s\n", version);

  uint8_t f[8];
  fcdAppGetParam(FCD_CMD_APP_GET_FREQ_HZ, f, 8);
  printf("# frequency %g MHz\n", (*(int*)f)/1e6);
  
  uint8_t lnagain;
  fcdAppGetParam(FCD_CMD_APP_GET_LNA_GAIN, &lnagain, 1);
  printf("# lna gain %g\n", lnagainvalues[lnagain]);
}


int openFuncubeSdr(double freq, double gain)
{
  int stat=0;
  /* set it */
  stat = fcdAppSetFreq(freq);
  switch (stat) {
    case FCD_MODE_NONE:
      printf("No FCD Detected.\n"); exit(255);
      break;
    case FCD_MODE_BL:
      printf("FCD in bootloader mode.\n"); exit(255);
      break;
    default:
      printf("frequency %.6f MHz\n", freq/1e6);
  }

  int mindiff=999;
  unsigned char b=-1, c;
  // find best value for lna gain
  int ng = sizeof(lnagainvalues)/sizeof(lnagainvalues[0]);	// oups
  for (c=0; c<ng; c++) {
    double delta;
    delta = fabs(gain-lnagainvalues[c]);
    if (delta<mindiff) { b=c; mindiff=delta; }
  }

  stat = fcdAppSetParam(FCD_CMD_APP_SET_LNA_GAIN,&b,1);
  switch (stat) {
    case FCD_MODE_NONE:
      printf("No FCD Detected.\n"); exit(255);
      break;
    case FCD_MODE_BL:
      printf("FCD in bootloader mode.\n"); exit(255);
      break;
    default:
      printf("gain %.6f dB\n", lnagainvalues[b]);
  }
  return 0;
}

char *openFuncubePcm()
{
  int rc;
  int dir;
  int size;
  unsigned int val;

  /* Open PCM device for playback. */
  char dev[1024];

  strcpy(dev, "hw:CARD=V10");
  // strcpy(dev, "hw:1,0");

  rc = snd_pcm_open(&handle, dev, SND_PCM_STREAM_CAPTURE, 0);
  if (rc < 0) {
    fprintf(stderr, "unable to open pcm device: %s\n", snd_strerror(rc));
    exit(1);
  }

  /* Allocate a hardware parameters object. */
  snd_pcm_hw_params_alloca(&params);

  /* Fill it in with default values. */
  snd_pcm_hw_params_any(handle, params);

  /** Set the desired hardware parameters. */
  /* Interleaved mode */
  snd_pcm_hw_params_set_access(handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);

  /* Signed 16-bit little-endian format */
  snd_pcm_hw_params_set_format(handle, params, SND_PCM_FORMAT_S16_LE);

  /* Two channels (I+Q) */
  snd_pcm_hw_params_set_channels(handle, params, 2);

  /* 44100 bits/second sampling rate (CD quality) */
  val = 96000;
  snd_pcm_hw_params_set_rate_near(handle, params, &val, &dir);

  /* Set period size to 32 frames. */
  frames = 32;
  snd_pcm_hw_params_set_period_size_near(handle, params, &frames, &dir);

  /* Write the parameters to the driver */
  rc = snd_pcm_hw_params(handle, params);
  if (rc < 0) {
    fprintf(stderr, "unable to set hw parameters: %s\n", snd_strerror(rc));
    exit(1);
  }

  snd_pcm_hw_params_get_rate(params, &val, &dir);
  printf("sampling_freq %d\n", val);

  /* Use a buffer large enough to hold one period */
  snd_pcm_hw_params_get_period_size(params, &frames, &dir);
  size = frames * 2 * 2; /* 2 bytes/sample, 2 channels */
  return (char *) malloc(size);
}

void readFuncube(char *buffer, double time, int sz, double sql)
{
  /**
   buffer is preallocated buffer
   time is capture time
   sz is size of frames
   */
  int dir, rc;
  long loops;
  unsigned int val;

  printf("dumping file fcrec.raw\n");
  int fdob = open("fcrec.raw", O_CREAT|O_WRONLY|O_ASYNC, 00644);
  printf("dumping file fcrec.dat\n");
  FILE *fdoa = fopen("fcrec.dat", "w");

  /* We want to loop for time seconds */
  snd_pcm_hw_params_get_period_time(params, &val, &dir);
  /* time seconds in microseconds divided by period time */
  loops = time * 1000000 / val;
  printf("loops %ld\n", loops);
  struct statT st = { 0, 0., 0. };

  while (loops > 0) {
    rc = snd_pcm_readi(handle, buffer, frames);
    if (rc == -EPIPE) {
      /* EPIPE means underrun */
      fprintf(stderr, "underrun occurred\n");
      snd_pcm_prepare(handle);
    }
    else if (rc < 0)
      fprintf(stderr, "error from read: %s\n", snd_strerror(rc));
    else if (rc != (int)frames)
      fprintf(stderr, "short read, read %d frames\n", rc);
    // read has been fully satistory
    else if (stats_high(&st, sql)) {
      if (write(fdob, buffer, sz*rc) != sz*rc) abort();
      writeAscii(fdoa, (short int *)buffer, sz*rc/2);
    }
    stats_sum(&st, (short int *)buffer, sz*rc/2);
    loops--;
  }

  stats_edit(fdoa, &st);
  close(fdob);
  fclose(fdoa);
}

void closeFuncube()
{
  snd_pcm_drain(handle);
  snd_pcm_close(handle);
}

/****************************************************************/

int main(int argc, char **argv) {
  double freq = 1.e6 * atof(argv[1]);
  double gain = atof(argv[2]);
  double time = atof(argv[3]);
  double sql=0;
  if (argc>4) sql = atof(argv[4]);
  openFuncubeSdr(freq, gain);
  //dumpSdr();
  char *buffer =openFuncubePcm(freq);
  int size=4; // size of pcm frames
  readFuncube(buffer, time, size, sql);
  closeFuncube();
  free(buffer);
  return 0;
}

