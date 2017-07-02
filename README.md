# inotify-watch
This tool uses the inotify facility on a Linux system to log filesystem
events. It is currently very basic, but allows some features that the
programs provided by inotify-tools do not have:

- Ability to be run as a daemon.
- Ability to log via syslog.
- Shows ssdeep hashes of filenames during key events.

## Compiling
This is pretty straightforward. Requires GCC, make, and libfuzzy:

	$ sudo apt install libfuzzy-dev
	$ make

## Running
- Edit inotify-watch.conf to list the directories and files you'd like to 
  monitor.
- ./inotify-watch -h # Shows usage info
- ./inotify-watch -d # Runs this in the background.
- inotify-watch-pid-check.sh is included to re-start in the event of a
  failure. Edit this file to suit your environment and for an example
  cron entry.


Check syslog for messages regarding the files you have watched.

