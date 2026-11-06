== stdout 1279
/a/b.c/d.e f g
echo /a/b.c/d.e f g
g echo echo /a/b.c/d
echo echo
g
/a/b.c/d
xa xa xa
echo xQ xQ xQ echo Ya Ya Ya
echo xQ xQ xQ xQ echo Ya Ya Ya xQ xQ xQ echo Ya Ya
echo echo echo xQ xQ xQ xQ echo Ya Ya Ya xQ xQ xQ echo Ya Ya
3 15
ok
OK
echo
a b
a
after
/tmp/hx
/tmp/hx
3 a b end
3 a b end
echo 3 a b end
histchars=@
    1  echo /a/b.c/d.e f g
    2  echo echo /a/b.c/d.e f g
    3  echo g echo echo /a/b.c/d
    4  echo echo echo
    5  echo g
    6  echo /a/b.c/d
    7  echo xa xa xa
    8  echo echo xQ xQ xQ echo Ya Ya Ya
    9  echo echo xQ xQ xQ xQ echo Ya Ya Ya xQ xQ xQ echo Ya Ya
   10  echo 'echo' 'echo' 'echo' 'xQ' 'xQ' 'xQ' 'xQ' 'echo' 'Ya' 'Ya' 'Ya' 'xQ' 'xQ' 'xQ' 'echo' 'Ya' 'Ya'
   11  echo '!!' \!! "!" !=x ! [!a] ${!x} $! "$( echo "!" )"
   12  echo "echo '!!' \!! "!" !=x ! [!a] ${!x} $! "$( echo "!" )"" [!a]
   13  (exit 3)
   14  echo $? $LINENO
   15  echo ok # !!
   16  echo OK # !!
   17  echo echo 
   18  echo a b; echo a
   19  echo echo a b; echo a
   20  echo after
   21  ls 2>/dev/null /tmp/hx
   22  echo 2> /dev/null /tmp/hx
   23  echo $((1+2)) $(echo a b) end
   24  echo $((1+2)) $(echo a b) end
   25  for i in 1; do echo echo $((1+2)) $(echo a b) end; done
   26  histchars='@'
   27  echo histchars='@'
   28  history
hi
echo hi
!!
a b

== stderr 984
echo echo /a/b.c/d.e f g
echo g echo echo /a/b.c/d
echo echo echo
echo g
echo /a/b.c/d
/tmp/hx: line 8: :s/f/F/: substitution failed
echo echo xQ xQ xQ echo Ya Ya Ya
echo echo xQ xQ xQ xQ echo Ya Ya Ya xQ xQ xQ echo Ya Ya
echo 'echo' 'echo' 'echo' 'xQ' 'xQ' 'xQ' 'xQ' 'echo' 'Ya' 'Ya' 'Ya' 'xQ' 'xQ' 'xQ' 'echo' 'Ya' 'Ya'
/tmp/hx: line 12: x: invalid indirect expansion
echo "echo '!!' \!! "!" !=x ! [!a] ${!x} $! "$( echo "!" )"" [!a]
/tmp/hx: line 13: x: invalid indirect expansion
/tmp/hx: line 15: !nosuch: event not found
/tmp/hx: line 16: :5: bad word specifier
/tmp/hx: line 16: z: unrecognized history modifier
/tmp/hx: line 16: :s/zzz/y/: substitution failed
echo OK # !!
echo echo 
echo a b; echo a
echo echo a b; echo a
echo 2> /dev/null /tmp/hx
echo $((1+2)) $(echo a b) end
/tmp/hx: line 25: :&: substitution failed
echo echo $((1+2)) $(echo a b) end
echo histchars='@'
echo hi
echo echo hi
echo echo hi
/tmp/hv: line 4: !nosuch: event not found
cat <<E
!!
E
echo a \
 b

== merged 2263
/a/b.c/d.e f g
echo echo /a/b.c/d.e f g
echo /a/b.c/d.e f g
echo g echo echo /a/b.c/d
g echo echo /a/b.c/d
echo echo echo
echo echo
echo g
g
echo /a/b.c/d
/a/b.c/d
/tmp/hx: line 8: :s/f/F/: substitution failed
xa xa xa
echo echo xQ xQ xQ echo Ya Ya Ya
echo xQ xQ xQ echo Ya Ya Ya
echo echo xQ xQ xQ xQ echo Ya Ya Ya xQ xQ xQ echo Ya Ya
echo xQ xQ xQ xQ echo Ya Ya Ya xQ xQ xQ echo Ya Ya
echo 'echo' 'echo' 'echo' 'xQ' 'xQ' 'xQ' 'xQ' 'echo' 'Ya' 'Ya' 'Ya' 'xQ' 'xQ' 'xQ' 'echo' 'Ya' 'Ya'
echo echo echo xQ xQ xQ xQ echo Ya Ya Ya xQ xQ xQ echo Ya Ya
/tmp/hx: line 12: x: invalid indirect expansion
echo "echo '!!' \!! "!" !=x ! [!a] ${!x} $! "$( echo "!" )"" [!a]
/tmp/hx: line 13: x: invalid indirect expansion
/tmp/hx: line 15: !nosuch: event not found
3 15
/tmp/hx: line 16: :5: bad word specifier
/tmp/hx: line 16: z: unrecognized history modifier
/tmp/hx: line 16: :s/zzz/y/: substitution failed
ok
echo OK # !!
OK
echo echo 
echo
echo a b; echo a
a b
a
echo echo a b; echo a
after
/tmp/hx
echo 2> /dev/null /tmp/hx
/tmp/hx
3 a b end
echo $((1+2)) $(echo a b) end
3 a b end
/tmp/hx: line 25: :&: substitution failed
echo echo $((1+2)) $(echo a b) end
echo 3 a b end
echo histchars='@'
histchars=@
    1  echo /a/b.c/d.e f g
    2  echo echo /a/b.c/d.e f g
    3  echo g echo echo /a/b.c/d
    4  echo echo echo
    5  echo g
    6  echo /a/b.c/d
    7  echo xa xa xa
    8  echo echo xQ xQ xQ echo Ya Ya Ya
    9  echo echo xQ xQ xQ xQ echo Ya Ya Ya xQ xQ xQ echo Ya Ya
   10  echo 'echo' 'echo' 'echo' 'xQ' 'xQ' 'xQ' 'xQ' 'echo' 'Ya' 'Ya' 'Ya' 'xQ' 'xQ' 'xQ' 'echo' 'Ya' 'Ya'
   11  echo '!!' \!! "!" !=x ! [!a] ${!x} $! "$( echo "!" )"
   12  echo "echo '!!' \!! "!" !=x ! [!a] ${!x} $! "$( echo "!" )"" [!a]
   13  (exit 3)
   14  echo $? $LINENO
   15  echo ok # !!
   16  echo OK # !!
   17  echo echo 
   18  echo a b; echo a
   19  echo echo a b; echo a
   20  echo after
   21  ls 2>/dev/null /tmp/hx
   22  echo 2> /dev/null /tmp/hx
   23  echo $((1+2)) $(echo a b) end
   24  echo $((1+2)) $(echo a b) end
   25  for i in 1; do echo echo $((1+2)) $(echo a b) end; done
   26  histchars='@'
   27  echo histchars='@'
   28  history
echo hi
hi
echo echo hi
echo echo hi
echo hi
/tmp/hv: line 4: !nosuch: event not found
cat <<E
!!
E
!!
echo a \
 b
a b

== status 0
