/* inotify-watch.c -- Watches for events on specified files and directories and
 *                 -- logs them. This only works on Linux because it uses
 *                 -- inotify.
 *  by Daniel Roberson @dmfroberson
 *
 * The Linux Programming Interface book helped a lot with this!!!
 *
 * TODO:
 * - atexit() and signal handlers to alert if this gets killed
 */

#include <sys/inotify.h>
#include <sys/types.h>
#include <limits.h>
#include <stdio.h>
#include <stdarg.h>
#include <syslog.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <time.h>

#include "inotify-watch.h"


#define BUF_LEN (10 * (sizeof(struct inotify_event) + NAME_MAX + 1))


/* inotify_t structure
 * wd - watch descriptor
 * filename - self-explanatory
 */
typedef struct inotifyEntry {
  int wd;
  char filename[BUF_LEN];
  struct inotifyEntry *next;
} inotify_t;


/* Globals */
int daemonize = 0;
int use_syslog = 1;
inotify_t *head = NULL;
char *logfile = LOGFILE;
char *pidfile = PIDFILE;
char *configfile = CONFIGFILE;


/* log_entry() -- adds log entry
 *             -- displays to stdout/stderr if not daemonized
 *             -- returns 0 on success, 1 on failure
 */
int log_entry(const char *fmt, ...) {
  int n;
  FILE *fp;
  va_list va;
  char buf[1024];
  char *timestr;
  time_t t;


  /* print timestamp */
  time(&t);
  timestr = strtok(ctime(&t), "\n");

  va_start(va, fmt);
  vsnprintf(buf, sizeof(buf), fmt, va);
  va_end(va);

  if (use_syslog)
    syslog(LOG_INFO, "[%s] %s", timestr, buf);

  if ((fp = fopen(logfile, "a+")) == NULL) {
    fprintf (stderr, "Unable to open logfile %s: %s\n",
	     logfile,
	     strerror(errno));
    return 1;
  }

  fprintf(fp, "[%s] %s\n", timestr, buf);

  if (daemonize == 0)
    printf("[%s] %s\n", timestr, buf);

  fclose(fp);
  return 0;
}

/* displayInotifyEvent() -- output inotify event in human-readable form
 */
void displayInotifyEvent(struct inotify_event *i, inotify_t *head) {
  char buf[1024];
  char int2str[32];
  char *mask;
  inotify_t *current = head;


  /* print filename */
  while(current->wd != i->wd)
    current = current->next;

  /* TODO: check if more than one event per mask ever happens */
  if (i->mask & IN_ACCESS)        mask = "IN_ACCESS";
  if (i->mask & IN_ATTRIB)        mask = "IN_ATTRIB";
  if (i->mask & IN_CLOSE_NOWRITE) mask = "IN_CLOSE_NOWRITE";
  if (i->mask & IN_CLOSE_WRITE)   mask = "IN_CLOSE_WRITE";
  if (i->mask & IN_CREATE)        mask = "IN_CREATE";
  if (i->mask & IN_DELETE)        mask = "IN_DELETE";
  if (i->mask & IN_DELETE_SELF)   mask = "IN_DELETE_SELF";
  if (i->mask & IN_IGNORED)       mask = "IN_IGNORED";
  if (i->mask & IN_ISDIR)         mask = "IN_ISDIR";
  if (i->mask & IN_MODIFY)        mask = "IN_MODIFY";
  if (i->mask & IN_MOVE_SELF)     mask = "IN_MOVE_SELF";
  if (i->mask & IN_MOVED_FROM)    mask = "IN_MOVED_FROM";
  if (i->mask & IN_MOVED_TO)      mask = "IN_MOVED_TO";
  if (i->mask & IN_OPEN)          mask = "IN_OPEN";
  if (i->mask & IN_Q_OVERFLOW)    mask = "IN_Q_OVERFLOW";
  if (i->mask & IN_UNMOUNT)       mask = "IN_UNMOUNT";

  if (i->cookie > 0)
    snprintf(int2str, sizeof(int2str), "%4d", i->cookie);

  snprintf(buf, sizeof(buf), "%s%s%s; wd =%2d; %s%s%s%s",
	   current->filename,
	   (i->len > 0) ? "/" : "",
	   (i->len > 0) ? i->name : "",
	   i->wd,
	   (i->cookie > 0) ? "cookie =" : "",
	   (i->cookie > 0) ? int2str : "",
	   (i->cookie > 0) ? "; " : "",
	   mask);

  log_entry("%s", buf);
}


/* addInotifyEntry -- adds wd/filename pair to linked list for future reference
 */
void addInotifyEntry(int wd, char *filename) {
  inotify_t *link = (inotify_t*)malloc(sizeof(inotify_t));

  link->wd = wd;
  strncpy(link->filename, filename, sizeof(link->filename));

  link->next = head;
  head = link;
}

/* writePidFile() -- writes pid file!
 */
void writePidFile(char *path, pid_t pid) {
  FILE *fp;

  fp = fopen(path, "w");
  if (fp == NULL) {
    log_entry("FATAL: Unable to open PID file %s: %s\n", path, strerror(errno));
    exit(EXIT_FAILURE);
  }

  fprintf(fp, "%d", pid);
  fclose(fp);
}


/* usage() -- print usage/help menu
 */
void usage(const char *progname) {
  fprintf(stderr, "usage: %s [-f <file>] [-l <file>] [-p <file>] [-sdh?]\n",
	  progname);
  fprintf(stderr, "  -f <file> -- Config file. Default: %s\n", configfile);
  fprintf(stderr, "  -l <file> -- Log file. Default: %s\n", logfile);
  fprintf(stderr, "  -p <file> -- PID file. Default: %s\n", pidfile);
  fprintf(stderr, "  -s        -- Toggles syslog usage. Default: %s\n",
	  use_syslog ? "yes" : "no");
  fprintf(stderr, "  -d        -- Daemonize. Default: %s\n",
	  daemonize ? "yes" : "no");
  fprintf(stderr, "  -h/-?     -- This help menu.\n");

  exit(EXIT_FAILURE);
}


/* addInotifyFiles() -- read filenames from config file and add to inotify
 */
void addInotifyFiles(int fd, const char *path) {
  int wd;
  FILE *fp;
  char hmm[BUF_LEN];


  fp = fopen(path, "r");
  if (fp == NULL) {
    log_entry("FATAL: Unable to open config file %s: %s\n",
	      path,
	      strerror(errno));
    exit(EXIT_FAILURE);
  }

  while (fgets(hmm, sizeof(hmm), fp) != NULL) {
    hmm[strcspn(hmm, "\n")] = '\0';
    if (strlen(hmm) == 0)
      continue;

    wd = inotify_add_watch(fd, hmm, IN_ALL_EVENTS);
    if (wd == -1) {
      fprintf(stderr, "inotify_add_watch %s: %s\n", hmm, strerror(errno));
    }

    addInotifyEntry(wd, hmm);
  }

  fclose(fp);
}


/* main() -- entry point of this program
 */
int main(int argc, char *argv[]) {
  int fd;
  int wd;
  int i;
  int opt;
  size_t num;
  char buf[BUF_LEN];
  char *p;
  pid_t pid;
  struct inotify_event *event;


  while ((opt = getopt(argc, argv, "h?sl:f:p:d")) != -1) {
    switch (opt) {
    case 'f': /* config file */
      configfile = optarg;
      break;

    case 'l': /* log file */
      logfile = optarg;
      break;

    case 'p': /* pid file */
      pidfile = optarg;
      break;

    case 's': /* toggle syslog */
      use_syslog = use_syslog ? 0 : 1;
      break;

    case 'd': /* daemonize */
      daemonize = daemonize ? 0 : 1;
      break;
 
    case '?': /* usage */
    case 'h':
      usage(argv[0]);
      
    default:
      usage(argv[0]);
    }
  }

  if (daemonize == 1) {
    pid = fork();
    if (pid < 0) {
      log_entry("FATAL: fork(): %s\n", strerror(errno));
      exit(EXIT_FAILURE);
    }

    else if (pid > 0) {
      writePidFile(pidfile, pid);
      exit(EXIT_SUCCESS);
    }

    printf("inotify-watch %s by %s started. PID %d\n",
	   VERSION,
	   AUTHOR,
	   getpid());
  }

  log_entry("inotify-watch %s by %s started. PID %d",
	    VERSION,
	    AUTHOR,
	    getpid());

  fd = inotify_init();
  if (fd == -1) {
    fprintf(stderr, "inotify_init() error: %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }

  addInotifyFiles(fd, configfile);

  for(;;) {
    num = read(fd, buf, BUF_LEN);
    if (num == 0 || num == -1) {
      log_entry("read() on inotify fd: %s\n", strerror(errno));
      exit(EXIT_FAILURE);
    }

    for (p = buf; p < buf + num;) {
      event = (struct inotify_event *) p;
      displayInotifyEvent(event, head);

      p += sizeof(struct inotify_event) + event->len;
    }
  }

  return EXIT_SUCCESS;
}
