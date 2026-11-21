# The working directory moved, removed or relinked under the shell:
# pwd -P and cd -P take getcwd's name where the old one no longer
# leads, as does pwd in POSIX mode, and cd tries a relative name again
# from the directory itself; where getcwd fails too, bash's messages
# ("error retrieving current directory"), naming pwd, cd or chdir, and
# "." as the name. A logical .. must follow a directory. Each case runs
# in a shell of its own, its directory names relative to the case's.
base=$PWD
t() {
    echo "## $1"
    rm -rf "$base/w"; mkdir "$base/w"; cd "$base/w"
    bash -c "$2" bash 2>&1 | sed "s|$base/w|W|g"
    cd "$base"
}
t moved-pwdP 'mkdir -p d/sub; cd d/sub; mv ../../d ../../e; pwd -P; echo "st $?"; pwd; echo "PWD $PWD"; pwd -P; pwd'
t moved-posix 'mkdir -p d/sub; cd d/sub; mv ../../d ../../e; set -o posix; pwd; echo "st $? PWD $PWD"; pwd -P; echo "PWD $PWD"'
t moved-cdP 'mkdir -p d/sub; cd d/sub; mv ../../d ../../e; cd -P .; echo "st $? PWD $PWD OLDPWD $OLDPWD"; pwd'
t moved-cdP-dotdot 'mkdir -p d/sub; cd d/sub; mv ../../d ../../e; cd -P ..; echo "st $? PWD $PWD"; pwd'
t moved-cd 'mkdir -p d/sub; cd d/sub; mv ../../d ../../e; cd .; echo "st $? PWD $PWD"; pwd'
t moved-posix-cd 'mkdir -p d/sub; cd d/sub; mv ../../d ../../e; set -o posix; cd .; echo "st $?"; cd -P .; echo "st $? PWD $PWD"'
t moved-cd-sub 'mkdir -p d/sub/x; cd d/sub; mv ../../d ../../e; cd x; echo "st $? PWD $PWD"; cd ../..; echo "PWD $PWD"'
t moved-cd-missing 'mkdir -p d/sub; cd d/sub; mv ../../d ../../e; cd nothere; echo "st $? PWD $PWD"'
t moved-cdpath 'mkdir -p d/sub/x; cd d/sub; mv ../../d ../../e; CDPATH=. cd x; echo "st $? PWD $PWD"'
t moved-link 'mkdir -p d/sub; ln -s d/sub l; cd l; mv ../../d ../../e; pwd -P; echo "st $?"; pwd; cd -P .; echo "st $? PWD $PWD"'
t relinked 'mkdir a b a/s b/s; ln -s a l; cd l/s; rm ../../l; ln -s b ../../l; pwd -P; pwd; set -o posix; pwd; pwd -P; echo "PWD $PWD"'
t removed-pwdP 'mkdir d; cd d; rmdir ../d; pwd -P; echo "st $?"; pwd; echo "st $? PWD $PWD"'
t removed-posix 'mkdir d; cd d; rmdir ../d; set -o posix; pwd; echo "st $?"; pwd -P; echo "st $? PWD $PWD"'
t removed-cdP 'mkdir d; cd d; rmdir ../d; cd -P .; echo "st $? PWD $PWD"; pwd'
t removed-cd 'mkdir d; cd d; rmdir ../d; cd ./.; echo "st $? PWD $PWD"; pwd'
t removed-setP 'mkdir d; cd d; rmdir ../d; set -P; pwd; echo "st $?"; cd .; echo "st $?"'
t removed-dotdot 'mkdir d; cd d; rmdir ../d; cd ..; echo "st $? PWD $PWD"; pwd'
t lost-dot 'mkdir d; cd d; rmdir ../d; pwd -P 2>/dev/null; cd .; echo "st $? PWD $PWD"; pwd; pwd -P; echo "st $?"; cd -P ./.; echo "st $? PWD $PWD OLDPWD $OLDPWD"; set -o posix; pwd'
t lost-name 'mkdir d; cd d; rmdir ../d; pwd -P 2>/dev/null; cd x; echo "st $?"; cd "$OLDPWD/.."; echo "st $?"; cd -; echo "st $?"'
t dotdot 'mkdir a; touch f; cd nonexist/..; echo "st $?"; cd /nonexist/../tmp; echo "st $?"; cd f/..; echo "st $?"; cd a/../a; echo "st $?"; set -o posix; cd ../nonexist/..; echo "st $?"; cd -P ../nonexist/..; echo "st $? PWD $PWD"'
t dotdot-link 'mkdir -p r/a/x l2; ln -s r/a l; cd l2; cd ../l/../l; echo "st $? PWD $PWD"; cd ../l/sub/..; echo "st $?"'
