a=(1 2 3 4); unset 'a[1]'; declare -p a; unset a[-1]; declare -p a; unset 'a[9]'; unset 'a[-9]'; echo st=$?
declare -A h=([x]=1 [y]=2); unset 'h[x]'; declare -p h; unset 'h[@]'; declare -p h
a=(1 2); unset 'a[@]'; declare -p a; a=(1 2); unset 'a[*]'; declare -p a
s=str; unset 's[0]'; declare -p s; s=str; unset 's[1]'; declare -p s
read -a r <<< "x  y z"; declare -p r; read -ra r2 <<< 'a\ b c'; declare -p r2
IFS=: read -a r3 <<< "1::2:"; declare -p r3
a=(x y); a+=z; declare -p a; a=w; declare -p a
declare -a q=1; declare -p q; q=([3]=a); declare -p q; declare q=(z); declare -p q
declare -A ass; ass=(a b c); declare -p ass; ass=([k]=v x); declare -p ass
declare -A h2=(a); declare -p h2; declare -A h3=([]=x); declare -p h3
a=([x]=1); declare -p a
declare -a i=([@]=3); declare -p i
