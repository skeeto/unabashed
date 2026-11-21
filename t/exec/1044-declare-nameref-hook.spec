== stdout 718
1
bash: line 7: warning: setlocale: LC_ALL: cannot change locale (bar): No such file or directory
2
set +o posix
set +o posix
shopt -s dotglob
shopt -u dotglob
set +o posix
bash: line 11: warning: setlocale: LC_ALL: cannot change locale (bar): No such file or directory
3
set -o posix
set -o posix
set +o posix
bash: line 14: warning: setlocale: LC_ALL: cannot change locale (qq): No such file or directory
4
ignoreeof      	off
ignoreeof      	on
bash: line 17: warning: setlocale: LC_ALL: cannot change locale (foo2): No such file or directory
bash: line 17: warning: setlocale: LC_ALL: cannot change locale (foo2): No such file or directory
5
set +o posix
set -o posix
ignoreeof      	off
set +o posix
set -o posix

== stderr 0

== merged 718
1
bash: line 7: warning: setlocale: LC_ALL: cannot change locale (bar): No such file or directory
2
set +o posix
set +o posix
shopt -s dotglob
shopt -u dotglob
set +o posix
bash: line 11: warning: setlocale: LC_ALL: cannot change locale (bar): No such file or directory
3
set -o posix
set -o posix
set +o posix
bash: line 14: warning: setlocale: LC_ALL: cannot change locale (qq): No such file or directory
4
ignoreeof      	off
ignoreeof      	on
bash: line 17: warning: setlocale: LC_ALL: cannot change locale (foo2): No such file or directory
bash: line 17: warning: setlocale: LC_ALL: cannot change locale (foo2): No such file or directory
5
set +o posix
set -o posix
ignoreeof      	off
set +o posix
set -o posix

== status 0
