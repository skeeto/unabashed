fib() { if (($1 < 2)); then r=$1; return; fi; local a; fib $(($1-1)); a=$r; fib $(($1-2)); r=$((a+r)); }
fib 24; echo $r
deep() { if (($1 > 0)); then deep $(($1-1)); fi; }
deep 5000
big() { local a=1 b=2 c=3; a=$((a+b)); b=$((b+c)); c=$((c+a))
  : one; : two; : three; : four; : five; : six; : seven; : eight
  : nine; : ten; : eleven; : twelve; : thirteen; : fourteen; x=$a$b$c; }
for ((i=0;i<100000;i++)); do big; done; echo $x
