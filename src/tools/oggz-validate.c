/*
   Copyright (C) 2003 Commonwealth Scientific and Industrial Research
   Organisation (CSIRO) Australia

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

   - Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.

   - Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

   - Neither the name of CSIRO Australia nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
   PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE ORGANISATION OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <errno.h>

#include <oggz/oggz.h>

#include "oggz_tools.h"

#define MAX_ERRORS 10

#define SUBSECONDS 1000.0

typedef ogg_int64_t timestamp_t;

typedef struct _OVData {
  OGGZ * writer;
  OggzTable * missing_eos;
  int theora_count;
  int audio_count;
} OVData;

typedef struct {
  int error;
  char * description;
} error_text;

static error_text errors[] = {
  {-20, "Packet belongs to unknown serialno"},
  {-24, "Granulepos decreasing within track"},
  {-5, "Multiple bos packets"},
  {-6, "Multiple eos packets"},
  {0, NULL}
};

static int max_errors = MAX_ERRORS;
static int multifile = 0;
static char * current_filename = NULL;
static timestamp_t current_timestamp = 0;
static int exit_status = 0;
static int nr_errors = 0;
static int prefix = 0, suffix = 0;

static void
list_errors (void)
{
  int i = 0;

  printf ("  File contains no Ogg packets\n");
  printf ("  Packets out of order\n");
  for (i = 0; errors[i].error; i++) {
    printf ("  %s\n", errors[i].description);
  }
  printf ("  eos marked but no bos\n");
  printf ("  Missing eos packets\n");
  printf ("  Granulepos on page with no completed packets\n");
  printf ("  Theora video bos page after audio bos page\n");
}
static void
usage (char * progname)
{

  printf ("Usage: %s [options] filename ...\n", progname);
  printf ("Validate the Ogg framing of one or more files\n");
  printf ("\n%s detects the following errors in Ogg framing:\n", progname);

  list_errors ();

  printf ("\nError reporting options\n");
  printf ("  -M num, --max-errors num\n");
  printf ("                         Exit after the specified number of errors.\n");
  printf ("                         A value of 0 specifies no maximum. Default: %d\n", MAX_ERRORS);
  printf ("  -p, --prefix           Treat input as the prefix of a stream; suppress\n");
  printf ("                         warnings about missing end-of-stream markers\n");
  printf ("  -s, --suffix           Treat input as the suffix of a stream; suppress\n");
  printf ("                         warnings about missing beginning-of-stream markers\n");
  printf ("                         on the first chain\n");
  printf ("  -P, --partial          Treat input as a the middle portion of a stream;\n");
  printf ("                         equivalent to both --prefix and --suffix\n");

  printf ("\nMiscellaneous options\n");
  printf ("  -h, --help             Display this help and exit\n");
  printf ("  -E, --help-errors      List known types of error and exit\n");
  printf ("  -v, --version          Output version information and exit\n");
  printf ("\n");
  printf ("Exit status is 0 if all input files are valid, 1 otherwise.\n\n");
  printf ("Please report bugs to <ogg-dev@xiph.org>\n");
}

static int
log_error (void)
{
  if (multifile && nr_errors == 0) {
    fprintf (stderr, "%s: Error:\n", current_filename);
  }

  exit_status = 1;

  nr_errors++;
  if (max_errors && nr_errors > max_errors)
    return OGGZ_STOP_ERR;

  return OGGZ_STOP_OK;
}

static ogg_int64_t
gp_to_granule (OGGZ * oggz, long serialno, ogg_int64_t granulepos)
{
  int granuleshift;
  ogg_int64_t iframe, pframe;

  granuleshift = oggz_get_granuleshift (oggz, serialno);

  iframe = granulepos >> granuleshift;
  pframe = granulepos - (iframe << granuleshift);

  return (iframe + pframe);
}

static timestamp_t
gp_to_time (OGGZ * oggz, long serialno, ogg_int64_t granulepos)
{
  ogg_int64_t gr_n, gr_d;
  ogg_int64_t granule;

  if (granulepos == -1) return -1.0;
  if (oggz_get_granulerate (oggz, serialno, &gr_n, &gr_d) != 0) return -1.0;

  granule = gp_to_granule (oggz, serialno, granulepos);

  return (timestamp_t)((double)(granule * gr_d) / (double)gr_n);
}

static int
read_page (OGGZ * oggz, const ogg_page * og, long serialno, void * user_data)
{
  OVData * ovdata = (OVData *)user_data;
  ogg_int64_t gpos = ogg_page_granulepos((ogg_page *)og);
  const char * content_type;
  int ret = 0;

  if (ogg_page_bos ((ogg_page *)og)) {
    content_type = ot_page_identify (og, NULL);

    if (content_type) {
      if (!strcmp (content_type, "Theora")) {
	ovdata->theora_count++;
	if (ovdata->audio_count > 0) {
	  log_error ();
	  fprintf (stderr, "serialno %010ld: Theora video bos page after audio bos page\n", serialno);
	}
      } else if (!strcmp (content_type, "Vorbis") || !strcmp (content_type, "Speex")) {
	ovdata->audio_count++;
      }
    }
  }

  if(gpos != -1 && ogg_page_packets((ogg_page *)og) == 0) {
    ret = log_error ();
    fprintf (stderr, "serialno %010ld: granulepos %lld on page with no completed packets, must be -1\n", serialno, gpos);
  }

  return ret;
}

static void
ovdata_init (OVData * ovdata)
{
  int flags;

  current_timestamp = 0;

  flags = OGGZ_WRITE|OGGZ_AUTO;
  if (prefix) flags |= OGGZ_PREFIX;
  if (suffix) flags |= OGGZ_SUFFIX;

  if ((ovdata->writer = oggz_new (flags)) == NULL) {
    fprintf (stderr, "oggz-validate: unable to create new writer\n");
    exit (1);
  }

  ovdata->missing_eos = oggz_table_new ();
  ovdata->theora_count = 0;
  ovdata->audio_count = 0;
}

static void
ovdata_clear (OVData * ovdata)
{
  long serialno;
  int i, nr_missing_eos = 0;

  oggz_close (ovdata->writer);

  if (!prefix && (max_errors == 0 || nr_errors <= max_errors)) {
    nr_missing_eos = oggz_table_size (ovdata->missing_eos);
    for (i = 0; i < nr_missing_eos; i++) {
      log_error ();
      oggz_table_nth (ovdata->missing_eos, i, &serialno);
      fprintf (stderr, "serialno %010ld: missing *** eos\n", serialno);
    }
  }

  oggz_table_delete (ovdata->missing_eos);
}

static int
read_packet (OGGZ * oggz, ogg_packet * op, long serialno, void * user_data)
{
  OVData * ovdata = (OVData *)user_data;
  timestamp_t timestamp;
  int flush;
  int ret = 0, feed_err = 0, i;

  if (op->b_o_s) {
    oggz_table_insert (ovdata->missing_eos, serialno, (void *)0x1);
  }

  if (!suffix && oggz_table_lookup (ovdata->missing_eos, serialno) == NULL) {
    ret = log_error ();
    fprintf (stderr, "serialno %010ld: missing *** bos\n", serialno);
  }

  if (!suffix && op->e_o_s) {
    if (oggz_table_remove (ovdata->missing_eos, serialno) == -1) {
      ret = log_error ();
      fprintf (stderr, "serialno %010ld: *** eos marked but no bos\n",
	       serialno);
    }
  }

  timestamp = gp_to_time (oggz, serialno, op->granulepos);
  if (timestamp != -1.0) {
    if (timestamp < current_timestamp) {
      ret = log_error();
      ot_fprint_time (stderr, (double)timestamp/SUBSECONDS);
      fprintf (stderr, ": serialno %010ld: Packet out of order (previous ",
	       serialno);
      ot_fprint_time (stderr, (double)current_timestamp/SUBSECONDS);
      fprintf (stderr, ")\n");
    }
    current_timestamp = timestamp;
  }

  if (op->granulepos == -1) {
    flush = 0;
  } else {
    flush = OGGZ_FLUSH_AFTER;
  }

  if ((feed_err = oggz_write_feed (ovdata->writer, op, serialno, flush, NULL)) != 0) {
    ret = log_error ();
    if (timestamp == -1.0) {
      fprintf (stderr, "%ld", oggz_tell (oggz));
    } else {
      ot_fprint_time (stderr, (double)timestamp/SUBSECONDS);
    }
    fprintf (stderr, ": serialno %010ld: ", serialno);
    for (i = 0; errors[i].error; i++) {
      if (errors[i].error == feed_err) {
	fprintf (stderr, "%s\n", errors[i].description);
	break;
      }
    }
    if (errors[i].error == 0) {
      fprintf (stderr,
	       "Packet violates Ogg framing constraints: %d\n",
	       feed_err);
    }
  }

  /* If this is the last packet in a chain, reset ovdata */
  if (op->e_o_s && oggz_table_size (ovdata->missing_eos) == 0) {
    ovdata_clear (ovdata);
    ovdata_init (ovdata);
    suffix = 0;
  }

  return ret;
}

static int
validate (char * filename)
{
  OGGZ * reader;
  OVData ovdata;
  unsigned char buf[1024];
  long n, nout = 0, bytes_written = 0;
  int active = 1;

  current_filename = filename;
  current_timestamp = 0;
  nr_errors = 0;

  /*printf ("oggz-validate: %s\n", filename);*/

  if (!strncmp (filename, "-", 2)) {
    if ((reader = oggz_open_stdio (stdin, OGGZ_READ|OGGZ_AUTO)) == NULL) {
      fprintf (stderr, "oggz-validate: unable to open stdin\n");
      return -1;
    }
  } else if ((reader = oggz_open (filename, OGGZ_READ|OGGZ_AUTO)) == NULL) {
    fprintf (stderr, "oggz-validate: unable to open file %s\n", filename);
    return -1;
  }

  ovdata_init (&ovdata);

  oggz_set_read_callback (reader, -1, read_packet, &ovdata);
  oggz_set_read_page (reader, -1, read_page, &ovdata);

  while (active && (n = oggz_read (reader, 1024)) != 0) {
    if (max_errors && nr_errors > max_errors) {
      fprintf (stderr,
	       "oggz-validate: maximum error count reached, bailing out ...\n");
      active = 0;
    } else while ((nout = oggz_write_output (ovdata.writer, buf, n)) > 0) {
      bytes_written += nout;
    }
  }

  oggz_close (reader);

  if (bytes_written == 0) {
    log_error ();
    fprintf (stderr, "File contains no Ogg packets\n");
  }

  ovdata_clear (&ovdata);

  return active ? 0 : -1;
}

int
main (int argc, char ** argv)
{
  int show_version = 0;
  int show_help = 0;

  /* Cache the --prefix, --suffix options and reset before validating
   * each input file */
  int opt_prefix = 0;
  int opt_suffix = 0;

  char * progname;
  char * filename;
  int i = 1;

  ot_init();

  progname = argv[0];

  if (argc < 2) {
    usage (progname);
    return (1);
  }

  while (1) {
    char * optstring = "M:psPhvE";

#ifdef HAVE_GETOPT_LONG
    static struct option long_options[] = {
      {"max-errors", required_argument, 0, 'M'},
      {"prefix", no_argument, 0, 'p'},
      {"suffix", no_argument, 0, 's'},
      {"partial", no_argument, 0, 'P'},
      {"help", no_argument, 0, 'h'},
      {"help-errors", no_argument, 0, 'E'},
      {"version", no_argument, 0, 'v'},
      {0,0,0,0}
    };

    i = getopt_long(argc, argv, optstring, long_options, NULL);
#else
    i = getopt (argc, argv, optstring);
#endif
    if (i == -1) {
      break;
    }
    if (i == ':') {
      usage (progname);
      goto exit_err;
    }

    switch (i) {
    case 'M': /* max-errors */
      max_errors = atoi (optarg);
      break;
    case 'p': /* prefix */
      opt_prefix = 1;
      break;
    case 's': /* suffix */
      opt_suffix = 1;
      break;
    case 'P': /* partial */
      opt_prefix = 1;
      opt_suffix = 1;
      break;
    case 'h': /* help */
      show_help = 1;
      break;
    case 'v': /* version */
      show_version = 1;
      break;
    case 'E': /* list errors */
      show_help = 2;
      break;
    default:
      break;
    }
  }

  if (show_version) {
    printf ("%s version " VERSION "\n", progname);
  }

  if (show_help == 1) {
    usage (progname);
  } else if (show_help == 2) {
    list_errors ();
  }

  if (show_version || show_help) {
    goto exit_out;
  }

  if (max_errors < 0) {
    printf ("%s: Error: [-M num, --max-errors num] option must be non-negative\n", progname);
    goto exit_err;
  }

  if (optind >= argc) {
    usage (progname);
    goto exit_err;
  }

  if (argc-i > 2) multifile = 1;

  for (i = optind; i < argc; i++) {
    filename = argv[i];
    prefix = opt_prefix;
    suffix = opt_suffix;
    if (validate (filename) == -1)
      exit_status = 1;
  }

 exit_out:
  exit (exit_status);

 exit_err:
  exit (1);
}
