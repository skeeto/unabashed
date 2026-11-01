g=global
rd() { local i; for ((i=0;i<100000;i++)); do v=$g; done; }
dive() { local l$1=1; if (($1 > 0)); then dive $(($1-1)); else rd; fi; }
dive 100; echo $v
