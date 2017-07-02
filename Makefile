CFLAGS = -Wall
LFLAGS = -lfuzzy

all: inotify-watch

inotify-watch:
	gcc ${CFLAGS} -o inotify-watch inotify-watch.c ${LFLAGS}

clean:
	rm -rf inotify-watch *~
