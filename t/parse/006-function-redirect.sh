f() { echo out; } > out.txt 2>&1
g() { read line; } < in.txt
h() ( echo x ) >> log 2> err
declare -f
