== stdout 1278
bash: --badopt: invalid option
Usage:	bash [GNU long option] [option] ...
	bash [GNU long option] [option] script-file ...
st 2
bash: -q: invalid option
	--debug
Shell options:
	-ilrsD or -c command or -O shopt_option		(invocation only)
st 2
bash: +q: invalid option
bash: --: invalid option
bash: --: invalid option
bash: +r: invalid option
st 2
bash: rcfile: option requires an argument
st 2
single dash
--norc
version 0
Usage:	bash [GNU long option] [option] ...
	bash [GNU long option] [option] script-file ...
GNU long options:
	--debug
	--debugger
	--dump-po-strings
	--dump-strings
	--help
	--init-file
	--login
	--noediting
	--noprofile
	--norc
	--posix
	--pretty-print
	--rcfile
	--restricted
	--verbose
	--version
Shell options:
	-ilrsD or -c command or -O shopt_option		(invocation only)
help 0
has version
debug
verbose
st 2
"plain"
"two
lines"
"q\"b"
"inner"
"insub"
st 0
"from -c"
#: d.sh:1
msgid "plain"
msgstr ""
#: d.sh:2
msgid ""
"two\n"
"lines"
msgstr ""
#: d.sh:3
msgid "q\\\"b"
msgstr ""
#: d.sh:4
msgid "inner"
msgstr ""
#: d.sh:4
msgid "insub"
msgstr ""

a=1

f () 
{ 
    echo f
}
g () 
{ 
    :
}
if x; then
    y;
fi &
case a in 
    a)
        b
    ;;
esac
for i in 1 2;
do
    echo $i;
done | cat

st 0
st 1
echo a

st 0
runs
zz zz
+i hBc
-i +i hBc

== stderr 189
bash: rcfile: option requires an argument
echo verbose
bash: line 0: nosuch: invalid shell option name
bad.sh: line 1: syntax error near unexpected token `newline'
bad.sh: line 1: `echo ('

== merged 1467
bash: --badopt: invalid option
Usage:	bash [GNU long option] [option] ...
	bash [GNU long option] [option] script-file ...
st 2
bash: -q: invalid option
	--debug
Shell options:
	-ilrsD or -c command or -O shopt_option		(invocation only)
st 2
bash: +q: invalid option
bash: --: invalid option
bash: --: invalid option
bash: +r: invalid option
bash: rcfile: option requires an argument
st 2
bash: rcfile: option requires an argument
st 2
single dash
--norc
version 0
Usage:	bash [GNU long option] [option] ...
	bash [GNU long option] [option] script-file ...
GNU long options:
	--debug
	--debugger
	--dump-po-strings
	--dump-strings
	--help
	--init-file
	--login
	--noediting
	--noprofile
	--norc
	--posix
	--pretty-print
	--rcfile
	--restricted
	--verbose
	--version
Shell options:
	-ilrsD or -c command or -O shopt_option		(invocation only)
help 0
has version
debug
echo verbose
verbose
bash: line 0: nosuch: invalid shell option name
st 2
"plain"
"two
lines"
"q\"b"
"inner"
"insub"
st 0
"from -c"
#: d.sh:1
msgid "plain"
msgstr ""
#: d.sh:2
msgid ""
"two\n"
"lines"
msgstr ""
#: d.sh:3
msgid "q\\\"b"
msgstr ""
#: d.sh:4
msgid "inner"
msgstr ""
#: d.sh:4
msgid "insub"
msgstr ""

a=1

f () 
{ 
    echo f
}
g () 
{ 
    :
}
if x; then
    y;
fi &
case a in 
    a)
        b
    ;;
esac
for i in 1 2;
do
    echo $i;
done | cat

st 0
bad.sh: line 1: syntax error near unexpected token `newline'
bad.sh: line 1: `echo ('
st 1
echo a

st 0
runs
zz zz
+i hBc
-i +i hBc

== status 0
