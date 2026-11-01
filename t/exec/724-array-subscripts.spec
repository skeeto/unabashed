== stdout 157
3 4 0 2 3 4
1 3 4 X t u 0 2 3 4 10 11
two five five five
xy
tr str  1
c b
a bc d
declare -ai n=([0]="2" [1]="6")
declare -a y=([1]="ab")
1 
2 
4 str
5 
done

== stderr 235
bash: line 3: ${#a[@]:1}: bad substitution
bash: line 8: s: bad array subscript
bash: line 13: u: bad array subscript
bash: line 14: s: bad array subscript
bash: line 15: [-1]: bad array subscript
bash: line 17: s: bad array subscript

== merged 392
bash: line 3: ${#a[@]:1}: bad substitution
3 4 0 2 3 4
1 3 4 X t u 0 2 3 4 10 11
two five five five
xy
bash: line 8: s: bad array subscript
tr str  1
c b
a bc d
declare -ai n=([0]="2" [1]="6")
declare -a y=([1]="ab")
bash: line 13: u: bad array subscript
1 
bash: line 14: s: bad array subscript
2 
bash: line 15: [-1]: bad array subscript
4 str
bash: line 17: s: bad array subscript
5 
done

== status 0
