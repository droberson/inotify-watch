all: inotify-watch

inotify-watch:
	gcc -o inotify-watch inotify-watch.c

clean:
	rm -rf inotify-watch *~
