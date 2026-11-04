== stdout 706
cmdline = <@WORK@\bin\argv.exe http://a/b a/b ./a/b ../a */x /x* /x? /[x] 1:2 x:/y C:/x C:\x ~/x :/x a'b/c a`b/c /x::y a/~b a:./b a:../b /a@@b "" plain -x --flag=word "a b" C:/x;D:/y>
argv[0] = <@WORK@\bin\argv.exe>
argv[1] = <http://a/b>
argv[2] = <a/b>
argv[3] = <./a/b>
argv[4] = <../a>
argv[5] = <*/x>
argv[6] = </x*>
argv[7] = </x?>
argv[8] = </[x]>
argv[9] = <1:2>
argv[10] = <x:/y>
argv[11] = <C:/x>
argv[12] = <C:\x>
argv[13] = <~/x>
argv[14] = <:/x>
argv[15] = <a'b/c>
argv[16] = <a`b/c>
argv[17] = </x::y>
argv[18] = <a/~b>
argv[19] = <a:./b>
argv[20] = <a:../b>
argv[21] = </a@@b>
argv[22] = <>
argv[23] = <plain>
argv[24] = <-x>
argv[25] = <--flag=word>
argv[26] = <a b>
argv[27] = <C:/x;D:/y>

== stderr 0

== merged 706
cmdline = <@WORK@\bin\argv.exe http://a/b a/b ./a/b ../a */x /x* /x? /[x] 1:2 x:/y C:/x C:\x ~/x :/x a'b/c a`b/c /x::y a/~b a:./b a:../b /a@@b "" plain -x --flag=word "a b" C:/x;D:/y>
argv[0] = <@WORK@\bin\argv.exe>
argv[1] = <http://a/b>
argv[2] = <a/b>
argv[3] = <./a/b>
argv[4] = <../a>
argv[5] = <*/x>
argv[6] = </x*>
argv[7] = </x?>
argv[8] = </[x]>
argv[9] = <1:2>
argv[10] = <x:/y>
argv[11] = <C:/x>
argv[12] = <C:\x>
argv[13] = <~/x>
argv[14] = <:/x>
argv[15] = <a'b/c>
argv[16] = <a`b/c>
argv[17] = </x::y>
argv[18] = <a/~b>
argv[19] = <a:./b>
argv[20] = <a:../b>
argv[21] = </a@@b>
argv[22] = <>
argv[23] = <plain>
argv[24] = <-x>
argv[25] = <--flag=word>
argv[26] = <a b>
argv[27] = <C:/x;D:/y>

== status 0
