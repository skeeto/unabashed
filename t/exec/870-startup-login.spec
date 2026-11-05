== stdout 462
profile: name 1 unset
cmd: name arg
login_shell         	on
/usr/local/bin:/usr/bin:/bin:/usr/local/games:/usr/games
bash_login
cmd2
bash_profile env
cmd3 set
noprofile2
nonlogin
login_shell         	off
dash: -bash
login_shell         	on
exec -l: -bash
bash_profile 
dash-i: -bash
bash_profile 
lc
bash_profile 
script: s.sh one
status 0
set 0
unset 0
bash_profile 
unset 0
login_shell         	on
login hBc
after 0
exiting
status 4
after syntax 2
dir profile

== stderr 152
/home/oracle/h/.bash_login: line 2: syntax error: unexpected end of file from `if' command on line 1
bash: /home/oracle/h/.bash_profile: is a directory

== merged 614
profile: name 1 unset
cmd: name arg
login_shell         	on
/usr/local/bin:/usr/bin:/bin:/usr/local/games:/usr/games
bash_login
cmd2
bash_profile env
cmd3 set
noprofile2
nonlogin
login_shell         	off
dash: -bash
login_shell         	on
exec -l: -bash
bash_profile 
dash-i: -bash
bash_profile 
lc
bash_profile 
script: s.sh one
status 0
set 0
unset 0
bash_profile 
unset 0
login_shell         	on
login hBc
after 0
exiting
status 4
/home/oracle/h/.bash_login: line 2: syntax error: unexpected end of file from `if' command on line 1
after syntax 2
bash: /home/oracle/h/.bash_profile: is a directory
dir profile

== status 0
