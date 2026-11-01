# tilde expansion in operator words
p() { printf '<%s>' "$@"; echo; }
unset u
p ${u:-~} ${u:-~/a} ${u-~}
p "${u:-~}" "${u:-~/a}"
p ${u:-a~} ${u:-'~'} ${u:-\~}
p ${u:-~+} ${u:-~-}
x=/home/oracle/dir/f
p "${x#~}" "${x#~/}" ${x#~/dir}
unset v; : ${v:=~/b}; p "$v"
unset w; : ${w:=~/a:~/b}; p "$w"
unset y; : "${y:=~}"; p "$y"
HOME=/h2; p ${u:-~}
