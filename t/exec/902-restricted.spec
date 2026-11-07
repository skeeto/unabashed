== stdout 410
hrBc
restricted_shell    	on
hrBc
hrBc
restricted_shell    	on
hrBc
restricted_shell    	off
env file ran in /
hrBc
declare -rx PATH
declare -r SHELL
declare -r ENV
declare -r BASH_ENV
declare -r HISTFILE
1
1
ok
st 0
st 1
st 1
st 1
st 1
st 1
st 1
st 1
sourced
sourced
st 0
st 0
st 1
st 1
st 1
st 0
st 1
st 1
cat
cat
st 1
st 1
st 1
st 2
hrBc
st 1
st 1
st 0
st 1
function
st 1
fhBc
hrBc
st 1
script in / hB
hrBc

== stderr 1174
rbash: line 1: cd: restricted
bash: line 1: cd: restricted
bash: line 1: PATH: readonly variable
bash: line 1: /bin/true: restricted: cannot specify `/' in command names
bash: line 1: /dev/null: restricted: cannot redirect output
bash: line 1: out: restricted: cannot redirect output
bash: line 1: out: restricted: cannot redirect output
bash: line 1: out: restricted: cannot redirect output
bash: line 1: out: restricted: cannot redirect output
bash: line 1: out: restricted: cannot redirect output
bash: line 1: .: ./src.sh: restricted
bash: line 1: .: src.sh: restricted
bash: line 1: exec: restricted
bash: line 1: command: -p: restricted
bash: line 1: command: -p: restricted
bash: line 1: hash: /bin/sh: restricted
bash: line 1: hash: nosuchcmd: not found
bash: line 1: /bin/sh: restricted
bash: line 1: nosuchcmd: not found
bash: line 1: enable: restricted
bash: line 1: enable: restricted
bash: line 1: set: +r: invalid option
set: usage: set [-abefhkmnptuvxBCEHPT] [-o option-name] [--] [-] [arg ...]
bash: line 1: set: restricted: invalid option name
bash: line 1: pushd: restricted
bash: line 1: popd: directory stack empty
bash: line 1: history: ./h: restricted

== merged 1584
hrBc
restricted_shell    	on
rbash: line 1: cd: restricted
hrBc
hrBc
restricted_shell    	on
hrBc
restricted_shell    	off
env file ran in /
hrBc
bash: line 1: cd: restricted
declare -rx PATH
declare -r SHELL
declare -r ENV
declare -r BASH_ENV
declare -r HISTFILE
bash: line 1: PATH: readonly variable
bash: line 1: /bin/true: restricted: cannot specify `/' in command names
1
bash: line 1: /dev/null: restricted: cannot redirect output
1
ok
st 0
bash: line 1: out: restricted: cannot redirect output
st 1
bash: line 1: out: restricted: cannot redirect output
st 1
bash: line 1: out: restricted: cannot redirect output
st 1
bash: line 1: out: restricted: cannot redirect output
st 1
bash: line 1: out: restricted: cannot redirect output
st 1
bash: line 1: .: ./src.sh: restricted
st 1
bash: line 1: .: src.sh: restricted
st 1
sourced
sourced
st 0
st 0
bash: line 1: exec: restricted
st 1
bash: line 1: command: -p: restricted
st 1
bash: line 1: command: -p: restricted
st 1
st 0
bash: line 1: hash: /bin/sh: restricted
st 1
bash: line 1: hash: nosuchcmd: not found
st 1
cat
bash: line 1: /bin/sh: restricted
bash: line 1: nosuchcmd: not found
cat
bash: line 1: enable: restricted
st 1
bash: line 1: enable: restricted
st 1
bash: line 1: set: +r: invalid option
set: usage: set [-abefhkmnptuvxBCEHPT] [-o option-name] [--] [-] [arg ...]
st 1
bash: line 1: set: restricted: invalid option name
st 2
hrBc
bash: line 1: pushd: restricted
st 1
bash: line 1: popd: directory stack empty
st 1
st 0
bash: line 1: history: ./h: restricted
st 1
function
st 1
fhBc
hrBc
st 1
script in / hB
hrBc

== status 0
