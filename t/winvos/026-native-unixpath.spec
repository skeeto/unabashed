== stdout 183
sh 1
bash 2
cmdline = <\bin\argv one>
argv[0] = <\bin\argv>
argv[1] = <one>
cmdline = <\usr\sbin\argv two>
argv[0] = <\usr\sbin\argv>
argv[1] = <two>
status 127
status 127
status 127

== stderr 164
bash: line 11: /bin/nosuch: No such file or directory
bash: line 12: /opt/argv: No such file or directory
bash: line 13: /usr/bin/x/argv: No such file or directory

== merged 347
sh 1
bash 2
cmdline = <\bin\argv one>
argv[0] = <\bin\argv>
argv[1] = <one>
cmdline = <\usr\sbin\argv two>
argv[0] = <\usr\sbin\argv>
argv[1] = <two>
bash: line 11: /bin/nosuch: No such file or directory
status 127
bash: line 12: /opt/argv: No such file or directory
status 127
bash: line 13: /usr/bin/x/argv: No such file or directory
status 127

== status 0
