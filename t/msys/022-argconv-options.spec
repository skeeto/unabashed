== stdout 540
cmdline = <@WORK@\bin\argv.exe -I@ROOT@/usr/include -LC:/lib --dir=@ROOT@/foo --dir=@ROOT@\foo;@ROOT@\bar foo=@ROOT@/bar -DX=Y:/ -oX:/ -oX:/y @@ROOT@/file -,X:/ a=b=C:/ @ROOT@/x=/y --a,B:/>
argv[0] = <@WORK@\bin\argv.exe>
argv[1] = <-I@ROOT@/usr/include>
argv[2] = <-LC:/lib>
argv[3] = <--dir=@ROOT@/foo>
argv[4] = <--dir=@ROOT@\foo;@ROOT@\bar>
argv[5] = <foo=@ROOT@/bar>
argv[6] = <-DX=Y:/>
argv[7] = <-oX:/>
argv[8] = <-oX:/y>
argv[9] = <@@ROOT@/file>
argv[10] = <-,X:/>
argv[11] = <a=b=C:/>
argv[12] = <@ROOT@/x=/y>
argv[13] = <--a,B:/>

== stderr 0

== merged 540
cmdline = <@WORK@\bin\argv.exe -I@ROOT@/usr/include -LC:/lib --dir=@ROOT@/foo --dir=@ROOT@\foo;@ROOT@\bar foo=@ROOT@/bar -DX=Y:/ -oX:/ -oX:/y @@ROOT@/file -,X:/ a=b=C:/ @ROOT@/x=/y --a,B:/>
argv[0] = <@WORK@\bin\argv.exe>
argv[1] = <-I@ROOT@/usr/include>
argv[2] = <-LC:/lib>
argv[3] = <--dir=@ROOT@/foo>
argv[4] = <--dir=@ROOT@\foo;@ROOT@\bar>
argv[5] = <foo=@ROOT@/bar>
argv[6] = <-DX=Y:/>
argv[7] = <-oX:/>
argv[8] = <-oX:/y>
argv[9] = <@@ROOT@/file>
argv[10] = <-,X:/>
argv[11] = <a=b=C:/>
argv[12] = <@ROOT@/x=/y>
argv[13] = <--a,B:/>

== status 0
