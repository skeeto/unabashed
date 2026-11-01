== stdout 858
[] []
foo qux : /usr/sbin/foo /usr/local/bin/qux
hits	command
   0	/usr/sbin/foo
   0	/sbin/blat
   0	/usr/local/bin/qux
hits	command
   0	/usr/sbin/foo
   0	/sbin/blat
   0	/usr/local/bin/qux
alias blat='cd /blat'
alias foo='/usr/sbin/foo'
alias sh='/bin/bash --login -o posix'
blat foo sh
alias blat='cd /blat'
alias foo='/usr/sbin/foo'
alias sh='/bin/bash --login -o posix'
/home/oracle 1
braceexpand:hashall:interactive-comments
checkwinsize:cmdhist:complete_fullquote:extquote:force_fignore:globasciiranges:globskipdots:hostcomplete:interactive_comments:patsub_replacement:progcomp:promptvars:sourcepath
braceexpand:errexit:hashall:interactive-comments:noglob
checkwinsize:cmdhist:complete_fullquote:extglob:extquote:force_fignore:globasciiranges:globskipdots:hostcomplete:interactive_comments:nullglob:patsub_replacement:progcomp:promptvars:sourcepath

== stderr 44
bash: line 20: SHELLOPTS: readonly variable

== merged 902
[] []
foo qux : /usr/sbin/foo /usr/local/bin/qux
hits	command
   0	/usr/sbin/foo
   0	/sbin/blat
   0	/usr/local/bin/qux
hits	command
   0	/usr/sbin/foo
   0	/sbin/blat
   0	/usr/local/bin/qux
alias blat='cd /blat'
alias foo='/usr/sbin/foo'
alias sh='/bin/bash --login -o posix'
blat foo sh
alias blat='cd /blat'
alias foo='/usr/sbin/foo'
alias sh='/bin/bash --login -o posix'
/home/oracle 1
braceexpand:hashall:interactive-comments
checkwinsize:cmdhist:complete_fullquote:extquote:force_fignore:globasciiranges:globskipdots:hostcomplete:interactive_comments:patsub_replacement:progcomp:promptvars:sourcepath
braceexpand:errexit:hashall:interactive-comments:noglob
checkwinsize:cmdhist:complete_fullquote:extglob:extquote:force_fignore:globasciiranges:globskipdots:hostcomplete:interactive_comments:nullglob:patsub_replacement:progcomp:promptvars:sourcepath
bash: line 20: SHELLOPTS: readonly variable

== status 1
