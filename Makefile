CFLAGS = -Wall

all: inotify-watch

inotify-watch:
	gcc ${CFLAGS} -o inotify-watch inotify-watch.c

clean:
	rm -rf inotify-watch *~
