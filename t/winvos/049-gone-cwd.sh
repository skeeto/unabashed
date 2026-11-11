# personality: msys
# Hand-checked against Git Bash 5.3.15: a working directory too long for
# a Windows process, over 258 characters, is refused ("File name too
# long", status 126, after the MSYS2 runtime's own explanation there),
# never run elsewhere. Git Bash's cannot be removed, as Cygwin holds it
# open; the shell holds none, so it can, and then a program is refused
# too, where once it started in the shell's first directory. The shell's
# own builtins and redirections are unaffected.
base=$(pwd -W)
x=$(printf "%0$((258 - ${#base} - 1))d" 0)
busybox mkdir "$x" "${x}1"
cd "$x" && argv fits; echo "258 $? ${#PWD}"
cd ../"${x}1" && argv long; echo "259 $?"
( argv sub ); echo "subshell $?"
cd ..
busybox mkdir gone && cd gone && (cd .. && busybox rmdir gone)
argv gone; echo "gone $?"
argv stage | busybox cat; echo "pipeline $?"
argv bg & wait $!; echo "background $?"
echo "comsub [$(argv comsub)] $?"
echo x >f; echo "redirection $?"
echo builtin; pwd
cd .. && argv back; echo "back $?"
