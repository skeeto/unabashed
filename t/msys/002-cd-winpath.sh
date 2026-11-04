# vos
# cd into the test directory by its Windows spellings
mkdir sub
here=$(pwd)
w=$(pwd -W)
echo "$here"
echo "$w"
cd / && cd "$w" && pwd
cd / && cd "${w//\//\\}" && pwd
cd / && cd "$w/sub" && pwd && echo "$OLDPWD"
cd / && cd "${w//\//\\}\\sub" && pwd
cd "$here" && cd 'sub\..\sub' && pwd
cd "$here" && cd - >/dev/null && pwd
cd "$here" && cd sub\\ && pwd
