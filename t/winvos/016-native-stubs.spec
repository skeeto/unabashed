== stdout 507
cmdline = <ls -d "a b">
argv[0] = <ls>
argv[1] = <-d>
argv[2] = <a b>
cmdline = <C:/work/s/busybox.exe ls 1>
argv[0] = <C:/work/s/busybox.exe>
argv[1] = <ls>
argv[2] = <1>
cmdline = <C:/work/s/busybox.exe ls 2>
argv[0] = <C:/work/s/busybox.exe>
argv[1] = <ls>
argv[2] = <2>
cmdline = <C:/work/s/busybox.exe ls 3>
argv[0] = <C:/work/s/busybox.exe>
argv[1] = <ls>
argv[2] = <3>
cmdline = <ls 4>
argv[0] = <ls>
argv[1] = <4>
cmdline = <s\pkg-config 5>
argv[0] = <s\pkg-config>
argv[1] = <5>
hi
hi
status 126

== stderr 59
bash: line 25: C:/w64/bin/argv.exe: Argument list too long

== merged 566
cmdline = <ls -d "a b">
argv[0] = <ls>
argv[1] = <-d>
argv[2] = <a b>
cmdline = <C:/work/s/busybox.exe ls 1>
argv[0] = <C:/work/s/busybox.exe>
argv[1] = <ls>
argv[2] = <1>
cmdline = <C:/work/s/busybox.exe ls 2>
argv[0] = <C:/work/s/busybox.exe>
argv[1] = <ls>
argv[2] = <2>
cmdline = <C:/work/s/busybox.exe ls 3>
argv[0] = <C:/work/s/busybox.exe>
argv[1] = <ls>
argv[2] = <3>
cmdline = <ls 4>
argv[0] = <ls>
argv[1] = <4>
cmdline = <s\pkg-config 5>
argv[0] = <s\pkg-config>
argv[1] = <5>
hi
hi
bash: line 25: C:/w64/bin/argv.exe: Argument list too long
status 126

== status 0
