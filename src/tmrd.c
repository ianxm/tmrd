/*
 *   tmrd - a simple sound delayer
 *   Copyright (C) 2009-2011 Ian Martins
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
 
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
 
#include <ncurses.h>
#include <jack/jack.h>
 
jack_port_t **input_port;
jack_port_t **output_port;

unsigned int buf_frames;	/* number of frames the buffer can hold */
unsigned int sample_rate;	/* sample rate */
int rec_frame;			/* index in buffer where next frame should be written */
int play_frame;			/* index in buffer where next frame should be read */
unsigned int delay_frames = 0;	/* num frames play is behind rec */
unsigned int max_delay_time = 100; /* in tenths of seconds */
unsigned int delay_time = 25;	/* in tenths of seconds */
int new_delay;
jack_default_audio_sample_t **buffer; /* the buffers */

void run_ui();
void sig_handler(int signum);
void attach_sig_handler();

int process (jack_nframes_t nframes, void *arg)
{
  jack_default_audio_sample_t *out, *in;

  /* record to buffer */
  out = &buffer[0][rec_frame];
  in = (jack_default_audio_sample_t *) jack_port_get_buffer (input_port[0], nframes);
  memcpy (out, in, sizeof (jack_default_audio_sample_t) * nframes);
  out = &buffer[1][rec_frame];
  in = (jack_default_audio_sample_t *) jack_port_get_buffer (input_port[1], nframes);
  memcpy (out, in, sizeof (jack_default_audio_sample_t) * nframes);

  rec_frame += nframes;
  if (rec_frame > buf_frames)
    rec_frame = 0;

  //mvprintw(10,0,"									");
  //mvprintw(10,0,"df: %u- %d= %d", delay_frames, delay_time*(int)sample_rate/10, delay_frames-(delay_time*(int)sample_rate/10));
  //refresh(); /* Print it on to the real screen */


  while (delay_frames < delay_time*sample_rate/10)  /* rewind play frame if its ahead */
  {
    delay_frames += nframes; /* lengthen delay */
    play_frame -= nframes;
    if (play_frame < 0)
      play_frame += buf_frames;
  }

  out = (jack_default_audio_sample_t *) jack_port_get_buffer (output_port[0], nframes);
  in = &buffer[0][play_frame];
  memcpy (out, in, sizeof (jack_default_audio_sample_t) * nframes);
  out = (jack_default_audio_sample_t *) jack_port_get_buffer (output_port[1], nframes);
  in = &buffer[1][play_frame];
  memcpy (out, in, sizeof (jack_default_audio_sample_t) * nframes);

  while (delay_frames-(delay_time*(int)sample_rate/10) > nframes) /* shorten delay */
  {
    play_frame += nframes;
    delay_frames -= nframes;
  }

  play_frame += nframes;
  if (play_frame > buf_frames)
    play_frame = 0;

  return 0;      
}
 
void jack_shutdown (void *arg)
{
  exit (1);
}
 
int main (int argc, char *argv[])
{
  attach_sig_handler();

  jack_client_t *client;
  const char **ports;
 
  /* init jack */
  if ((client = jack_client_new ("smp")) == 0) {
    fprintf (stderr, "jack server not running?\n");
    return 1;
  }
  jack_set_process_callback (client, process, 0);
  jack_on_shutdown (client, jack_shutdown, 0);
  sample_rate = jack_get_sample_rate (client);
  printf ("engine sample rate: %u\n", sample_rate);
 
  /* create a delay buffer for each channel */
  buffer = (jack_default_audio_sample_t**)malloc(2*sizeof(jack_default_audio_sample_t**));

  /* div by 10 since time is in tenths of secs */
  buf_frames = max_delay_time*sample_rate/10;
  buffer[0] = (jack_default_audio_sample_t*)malloc(buf_frames*sizeof(jack_default_audio_sample_t));
  buffer[1] = (jack_default_audio_sample_t*)malloc(buf_frames*sizeof(jack_default_audio_sample_t));
  memset(buffer[0], 0, buf_frames*sizeof(jack_default_audio_sample_t));
  memset(buffer[1], 0, buf_frames*sizeof(jack_default_audio_sample_t));
  printf("buffer frames: %d\n", buf_frames);
  
  /* create two input ports and two output ports */
  input_port = (jack_port_t**)malloc(2*sizeof(jack_port_t**));
  output_port = (jack_port_t**)malloc(2*sizeof(jack_port_t**));
  input_port[0] = jack_port_register (client, "input0", JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
  input_port[1] = jack_port_register (client, "input1", JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
  output_port[0] = jack_port_register (client, "output0", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
  output_port[1] = jack_port_register (client, "output1", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
 
  /* start jack */
  if (jack_activate (client)) {
    fprintf (stderr, "cannot activate client");
    return 1;
  }
  if ((ports = jack_get_ports (client, NULL, NULL, JackPortIsPhysical|JackPortIsOutput)) == NULL) {
    fprintf(stderr, "Cannot find any physical capture ports\n");
    exit(1);
  }
 
  if (jack_connect (client, ports[0], jack_port_name (input_port[0])))
    fprintf (stderr, "cannot connect input ports\n");

  if (jack_connect (client, ports[1], jack_port_name (input_port[1])))
    fprintf (stderr, "cannot connect input ports\n");
 
  free (ports);
         
  if ((ports = jack_get_ports (client, NULL, NULL, JackPortIsPhysical|JackPortIsInput)) == NULL) {
    fprintf(stderr, "Cannot find any physical playback ports\n");
    exit(1);
  }

  if (jack_connect (client, jack_port_name (output_port[0]), ports[0]))
    fprintf (stderr, "cannot connect output ports\n");

  if (jack_connect (client, jack_port_name (output_port[1]), ports[1]))
    fprintf (stderr, "cannot connect output ports\n");
 
  free (ports);
 
  run_ui(); /* start the ui, listen for keyboard input */

  jack_client_close (client);
  exit (0);
}

/*
 * simple ncurses interface
 */
void run_ui()
{
  new_delay = (int)delay_time;
  int ch = 0;
  initscr();				/* Start curses mode   */
  mvprintw(0,0,"Tim's Merrill Reese Delayer 2.0");
  mvprintw(2,0,"instructions:");
  mvprintw(3,0," up arrow to increase delay by .1 sec");
  mvprintw(4,0," down arrow to decrease delay by .1 sec");
  mvprintw(5,0," q to quit\n");
  mvprintw(7,0,"max delay: %.1lf seconds", max_delay_time/10.);
  mvprintw(8,0,"current delay: %.1lf seconds", delay_time/10.);
  curs_set(0);
  refresh();				/* Print it on to the real screen */
  noecho();

  while (ch!='q')
  {
    ch = getch();			/* Wait for user input */
    if (ch==65) new_delay += 1;		/* up */
    else if (ch==66) new_delay -= 1;	/* down */
    if (new_delay < 0) new_delay = 0;
    else if (new_delay > max_delay_time) new_delay = max_delay_time;
    delay_time = new_delay;

    mvprintw(8,0,"current delay: %.1lf seconds", delay_time/10.);
  }
  endwin();				/* End curses mode  */
}

/*
  increase delay on sigusr1
  decrease on sigusr2
  the signal counts as stdin input, so it automatically updates the display
*/
void sig_handler(int signum)
{
  if( signum==SIGUSR1 )
    new_delay += 1;
  else
    new_delay -= 1;
}

void attach_sig_handler()
{
  struct sigaction handler;
  handler.sa_handler = sig_handler;
  handler.sa_flags = SA_NOMASK;
  sigaction(SIGUSR1, &handler, NULL);
  sigaction(SIGUSR2, &handler, NULL);
}
