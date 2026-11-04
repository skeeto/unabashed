== stdout 710
cmdline = <@WORK@\bin\argv.exe "" "a b" "a\"b" a\b a\ a\\ "a\\\"b" "\"" \ "tab	x" " lead" "trail " "x y\\" "\"quoted\"" "a\\\\\"b" %PATH% ^&|<> it's é>
argv[0] = <@WORK@\bin\argv.exe>
argv[1] = <>
argv[2] = <a b>
argv[3] = <a"b>
argv[4] = <a\b>
argv[5] = <a\>
argv[6] = <a\\>
argv[7] = <a\"b>
argv[8] = <">
argv[9] = <\>
argv[10] = <tab	x>
argv[11] = < lead>
argv[12] = <trail >
argv[13] = <x y\>
argv[14] = <"quoted">
argv[15] = <a\\"b>
argv[16] = <%PATH%>
argv[17] = <^&|<>>
argv[18] = <it's>
argv[19] = <é>
cmdline = <@WORK@\bin\argv32.exe "" "a b" "a\"b" a\ "a\\\"b" "x y\\">
argv[0] = <@WORK@\bin\argv32.exe>
argv[1] = <>
argv[2] = <a b>
argv[3] = <a"b>
argv[4] = <a\>
argv[5] = <a\"b>
argv[6] = <x y\>

== stderr 0

== merged 710
cmdline = <@WORK@\bin\argv.exe "" "a b" "a\"b" a\b a\ a\\ "a\\\"b" "\"" \ "tab	x" " lead" "trail " "x y\\" "\"quoted\"" "a\\\\\"b" %PATH% ^&|<> it's é>
argv[0] = <@WORK@\bin\argv.exe>
argv[1] = <>
argv[2] = <a b>
argv[3] = <a"b>
argv[4] = <a\b>
argv[5] = <a\>
argv[6] = <a\\>
argv[7] = <a\"b>
argv[8] = <">
argv[9] = <\>
argv[10] = <tab	x>
argv[11] = < lead>
argv[12] = <trail >
argv[13] = <x y\>
argv[14] = <"quoted">
argv[15] = <a\\"b>
argv[16] = <%PATH%>
argv[17] = <^&|<>>
argv[18] = <it's>
argv[19] = <é>
cmdline = <@WORK@\bin\argv32.exe "" "a b" "a\"b" a\ "a\\\"b" "x y\\">
argv[0] = <@WORK@\bin\argv32.exe>
argv[1] = <>
argv[2] = <a b>
argv[3] = <a"b>
argv[4] = <a\>
argv[5] = <a\"b>
argv[6] = <x y\>

== status 0
