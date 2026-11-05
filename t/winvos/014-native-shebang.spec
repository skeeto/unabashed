== stdout 369
sh 1
env-sh 2
cmdline = <C:\w64\bin\argv "one  two" ./d 4>
argv[0] = <C:\w64\bin\argv>
argv[1] = <one  two>
argv[2] = <./d>
argv[3] = <4>
cmdline = <C:\w64\bin\argv "one  two" ./d ./e1 ./e2 ./e3 5>
argv[0] = <C:\w64\bin\argv>
argv[1] = <one  two>
argv[2] = <./d>
argv[3] = <./e1>
argv[4] = <./e2>
argv[5] = <./e3>
argv[6] = <5>
status 126
plain 7
status 126
status 127

== stderr 185
bash: ./e4: C:/w64/bin/argv: bad interpreter: Too many levels of symbolic links
bash: ./f: /no/such: bad interpreter: No such file or directory
env: 'nosuch': No such file or directory

== merged 554
sh 1
env-sh 2
cmdline = <C:\w64\bin\argv "one  two" ./d 4>
argv[0] = <C:\w64\bin\argv>
argv[1] = <one  two>
argv[2] = <./d>
argv[3] = <4>
cmdline = <C:\w64\bin\argv "one  two" ./d ./e1 ./e2 ./e3 5>
argv[0] = <C:\w64\bin\argv>
argv[1] = <one  two>
argv[2] = <./d>
argv[3] = <./e1>
argv[4] = <./e2>
argv[5] = <./e3>
argv[6] = <5>
bash: ./e4: C:/w64/bin/argv: bad interpreter: Too many levels of symbolic links
status 126
plain 7
bash: ./f: /no/such: bad interpreter: No such file or directory
status 126
env: 'nosuch': No such file or directory
status 127

== status 0
