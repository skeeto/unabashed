# personality: native
# Hand-checked baseline, not recorded from an oracle (M5a step 9),
# checked against w64devkit's busybox sh. A login shell (-l, a leading
# dash, or a name starting with l) reads the system drive's
# /etc/profile, then etc/profile beside the shell's directory, then
# $HOME/.profile, with the positional parameters already bound; no
# tilde, missing files are skipped quietly, a return in a profile leaves
# its status. An interactive shell reads $ENV, expanded. BASH_ENV is
# not read. --noprofile and --norc are accepted (busybox rejects them).
# An interactive shell's PS1 defaults to busybox-w32's \w \$ (an
# inherited one stays).
mkdir -p C:/etc C:/w64/etc
echo 'echo sys "$0" "$1"' >C:/etc/profile
echo 'echo exe "$#"' >C:/w64/etc/profile
echo 'echo home; return 3' >"$HOME/.profile"
echo 'echo env' >C:/work/env.sh
sh -l -c 'echo "run $?"' zero one
cp C:/w64/bin/sh.exe C:/w64/bin/lash.exe
lash -c 'echo lash'
sh -c 'echo plain'
BASH_ENV=C:/work/env.sh sh -c 'echo no bash_env'
ENV='$PWD/env.sh' sh -i -c 'echo interactive'
ENV=C:/work/env.sh sh --norc -l -i -c 'echo both'
sh --noprofile -l -c 'echo noprofile'
rm C:/w64/etc/profile "$HOME/.profile"
mkdir "$HOME/.profile"
sh -l -c 'echo dir'
(exec -a -sh sh -c 'echo dash')
(unset HOME; sh -l -c 'echo nohome')
(unset PS1; sh -i -c 'echo "PS1=$PS1"')
(PS1='> ' sh -i -c 'echo "PS1=$PS1"')
