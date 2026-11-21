# Command substitutions nested 100 deep through a recursive function,
# each one in the one above: the innermost value comes back out
r() { if (($1 < N)); then echo $(r $(($1+1))); else echo $1; fi; }
N=70 r 0
N=100 r 0
echo "after $?"
