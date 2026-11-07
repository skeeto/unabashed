# A ~user prefix is that user's home from the password file, left as it
# is for no such user; with HOME unset, ~ is the shell's own user's
# home. Also after : in assignments, and in BASH_ENV.
exec 2>&1
echo ~root ~root/x ~oracle ~nosuch ~daemon/ ~nobody
x=~root:~oracle; echo "$x"
PATH2=a:~root/bin; echo $PATH2
echo "~root" ~"root" \~root
echo "${v:-~root}" ${v:-~root}
[[ ~root == /root ]] && echo match
case /root in ~root) echo case ;; esac
echo 'echo "from BASH_ENV"' >~/e
BASH_ENV='~oracle/e' bash -c 'echo body'
unset HOME; echo ~ ~/x
