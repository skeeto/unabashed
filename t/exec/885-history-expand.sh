# History expansion, in a script read by the shell (not -c): events,
# word designators, modifiers, quoting, bash's exceptions, :p, ^old^new,
# errors (the line dropped, $? kept) and the lines that go to history
cat > /tmp/hx <<'END'
set -o history -H
echo /a/b.c/d.e f g
echo !!
echo !$:h !^:t !*:r
echo !-2:0 !echo:2
echo !?b.c?:1:e
echo !?b.c?%
!!:s/f/F/
echo xa xa xa
echo !!:gs/a/Q/ !!:Gs/x/Y/
echo !!:1-2 !!:2* !!:2-
echo !:1:q !!:x
echo '!!' \!! "!" !=x ! [!a] ${!x} $! "$( echo "!" )"
echo "!!" [!a]
(exit 3)
echo !nosuch
echo $? $LINENO
echo !!:5
echo !!:z
echo !!:s/zzz/y/
echo ok # !!
^ok^OK^
echo !#
echo a b; echo !#:1
echo !!:p
echo after
ls 2>/dev/null /tmp/hx
echo !ls:*
echo $((1+2)) $(echo a b) end
echo !!:*
echo !!:&
for i in 1; do
echo !!
done
histchars='@'
echo @@
history
END
bash /tmp/hx
cat > /tmp/hv <<'END'
set -o history -H -v
echo hi
echo !!
echo !nosuch
cat <<E
!!
E
echo a \
 b
END
bash /tmp/hv
