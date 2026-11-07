# cdable_vars: a cd argument that is no directory may name a variable
# holding one, which cd prints; cdspell needs an interactive shell
exec 2>&1
mkdir -p /tmp/cv/sub; cd /tmp/cv
proj=/tmp/cv/sub rel=sub empty= notdir=/nonexistent
cd proj; echo "off $? $PWD"
shopt -s cdable_vars
cd proj; echo "on $? $PWD"
cd /tmp/cv; cd rel; echo "rel $? $PWD"
cd /tmp/cv; cd empty; echo "empty $? $PWD"
cd notdir; echo "notdir $? $PWD"
cd unset_var; echo "unset $? $PWD"
mkdir /tmp/cv/proj; cd /tmp/cv; cd proj; echo "dir wins $? $PWD"
cd /tmp/cv; cd 'a b'; echo "badname $? $PWD"
shopt -s cdspell; cd /tmp/cv; cd sbu; echo "spell $? $PWD"
