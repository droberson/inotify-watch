# inotify-watch
This tool uses the inotify facility on a Linux system to log filesystem
events. It is currently very basic, but allows some features that the
programs provided by inotify-tools do not have:

- Ability to be run as a daemon.
- Ability to log via syslog.

## Compiling
This is pretty straightforward. Requires GCC and make:

	$ make

## Running
- Edit inotify-watch.conf to list the directories and files you'd like to 
  monitor.
- ./inotify-watch -h # Shows usage info
- ./inotify-watch -d # Runs this in the background.

Check syslog for messages regarding the files you have watched.

