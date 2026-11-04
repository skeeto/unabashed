== stdout 571
cmdline = <@WORK@\bin\argv.exe a:/b C:\x;@WORK@\023-argconv-lists\tmp /c/x;/d/y @ROOT@/foo,/bar X://.. "@ROOT@/a b" a:b a:/b: /c //server/share /server //c/x C: c:/x \x\y \\server\share>
argv[0] = <@WORK@\bin\argv.exe>
argv[1] = <a:/b>
argv[2] = <C:\x;@WORK@\023-argconv-lists\tmp>
argv[3] = </c/x;/d/y>
argv[4] = <@ROOT@/foo,/bar>
argv[5] = <X://..>
argv[6] = <@ROOT@/a b>
argv[7] = <a:b>
argv[8] = <a:/b:>
argv[9] = </c>
argv[10] = <//server/share>
argv[11] = </server>
argv[12] = <//c/x>
argv[13] = <C:>
argv[14] = <c:/x>
argv[15] = <\x\y>
argv[16] = <\\server\share>

== stderr 0

== merged 571
cmdline = <@WORK@\bin\argv.exe a:/b C:\x;@WORK@\023-argconv-lists\tmp /c/x;/d/y @ROOT@/foo,/bar X://.. "@ROOT@/a b" a:b a:/b: /c //server/share /server //c/x C: c:/x \x\y \\server\share>
argv[0] = <@WORK@\bin\argv.exe>
argv[1] = <a:/b>
argv[2] = <C:\x;@WORK@\023-argconv-lists\tmp>
argv[3] = </c/x;/d/y>
argv[4] = <@ROOT@/foo,/bar>
argv[5] = <X://..>
argv[6] = <@ROOT@/a b>
argv[7] = <a:b>
argv[8] = <a:/b:>
argv[9] = </c>
argv[10] = <//server/share>
argv[11] = </server>
argv[12] = <//c/x>
argv[13] = <C:>
argv[14] = <c:/x>
argv[15] = <\x\y>
argv[16] = <\\server\share>

== status 0
