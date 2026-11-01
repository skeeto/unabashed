== stdout 313
2 <(>
1 <a)>
2 <[>
2 <[a>
2 <a{1>
2 <a{2,1}>
2 <*a>
2 <\>
2 <(a)\2>
2 <[[:foo:]]>
2 <[z-a]>
2 <[[.xx.]]>
2 <a{99999}>
2 <[[=ab=]]>
2 <a{,1,2}>
2 <(a|*b)>
2 <^*>
2 <a{}>
2 <a{x}>
2 <[a-[:digit:]]>
2 <[[:alpha:]-z]>
2 <(a)|\1>
2 <[[.>
! 0
|| 0
&& 2
else 2
declare -a BASH_REMATCH=([0]="x")
t 2
!t 0
t|| 0
eq 1
done

== stderr 2242
bash: line 5: [[: invalid regular expression `(': Unmatched ( or \(
bash: line 5: [[: invalid regular expression `[': Invalid regular expression
bash: line 5: [[: invalid regular expression `[a': Unmatched [, [^, [:, [., or [=
bash: line 5: [[: invalid regular expression `a{1': Unmatched \{
bash: line 5: [[: invalid regular expression `a{2,1}': Invalid content of \{\}
bash: line 5: [[: invalid regular expression `*a': Invalid preceding regular expression
bash: line 5: [[: invalid regular expression `\': Trailing backslash
bash: line 5: [[: invalid regular expression `(a)\2': Invalid back reference
bash: line 5: [[: invalid regular expression `[[:foo:]]': Invalid character class name
bash: line 5: [[: invalid regular expression `[z-a]': Invalid range end
bash: line 5: [[: invalid regular expression `[[.xx.]]': Invalid collation character
bash: line 5: [[: invalid regular expression `a{99999}': Regular expression too big
bash: line 5: [[: invalid regular expression `[[=ab=]]': Invalid collation character
bash: line 5: [[: invalid regular expression `a{,1,2}': Invalid content of \{\}
bash: line 5: [[: invalid regular expression `(a|*b)': Invalid preceding regular expression
bash: line 5: [[: invalid regular expression `^*': Invalid preceding regular expression
bash: line 5: [[: invalid regular expression `a{}': Invalid content of \{\}
bash: line 5: [[: invalid regular expression `a{x}': Invalid content of \{\}
bash: line 5: [[: invalid regular expression `[a-[:digit:]]': Invalid range end
bash: line 5: [[: invalid regular expression `[[:alpha:]-z]': Invalid range end
bash: line 5: [[: invalid regular expression `(a)|\1': Invalid back reference
bash: line 5: [[: invalid regular expression `[[.': Unmatched [, [^, [:, [., or [=
bash: line 9: [[: invalid regular expression `(': Unmatched ( or \(
bash: line 10: [[: invalid regular expression `(': Unmatched ( or \(
bash: line 11: [[: invalid regular expression `(': Unmatched ( or \(
bash: line 12: [[: invalid regular expression `(': Unmatched ( or \(
+ [[ a.b =~ a\.b ]]
+ [[ abc =~ ab. ]]
+ re='x|(y)'
+ [[ zy =~ x|(y) ]]
+ [[ a =~ []] ]]
+ set +x
bash: line 20: [[: X: integer expected
bash: line 21: [[: X: integer expected
bash: line 22: [[: X: integer expected

== merged 2555
bash: line 5: [[: invalid regular expression `(': Unmatched ( or \(
2 <(>
1 <a)>
bash: line 5: [[: invalid regular expression `[': Invalid regular expression
2 <[>
bash: line 5: [[: invalid regular expression `[a': Unmatched [, [^, [:, [., or [=
2 <[a>
bash: line 5: [[: invalid regular expression `a{1': Unmatched \{
2 <a{1>
bash: line 5: [[: invalid regular expression `a{2,1}': Invalid content of \{\}
2 <a{2,1}>
bash: line 5: [[: invalid regular expression `*a': Invalid preceding regular expression
2 <*a>
bash: line 5: [[: invalid regular expression `\': Trailing backslash
2 <\>
bash: line 5: [[: invalid regular expression `(a)\2': Invalid back reference
2 <(a)\2>
bash: line 5: [[: invalid regular expression `[[:foo:]]': Invalid character class name
2 <[[:foo:]]>
bash: line 5: [[: invalid regular expression `[z-a]': Invalid range end
2 <[z-a]>
bash: line 5: [[: invalid regular expression `[[.xx.]]': Invalid collation character
2 <[[.xx.]]>
bash: line 5: [[: invalid regular expression `a{99999}': Regular expression too big
2 <a{99999}>
bash: line 5: [[: invalid regular expression `[[=ab=]]': Invalid collation character
2 <[[=ab=]]>
bash: line 5: [[: invalid regular expression `a{,1,2}': Invalid content of \{\}
2 <a{,1,2}>
bash: line 5: [[: invalid regular expression `(a|*b)': Invalid preceding regular expression
2 <(a|*b)>
bash: line 5: [[: invalid regular expression `^*': Invalid preceding regular expression
2 <^*>
bash: line 5: [[: invalid regular expression `a{}': Invalid content of \{\}
2 <a{}>
bash: line 5: [[: invalid regular expression `a{x}': Invalid content of \{\}
2 <a{x}>
bash: line 5: [[: invalid regular expression `[a-[:digit:]]': Invalid range end
2 <[a-[:digit:]]>
bash: line 5: [[: invalid regular expression `[[:alpha:]-z]': Invalid range end
2 <[[:alpha:]-z]>
bash: line 5: [[: invalid regular expression `(a)|\1': Invalid back reference
2 <(a)|\1>
bash: line 5: [[: invalid regular expression `[[.': Unmatched [, [^, [:, [., or [=
2 <[[.>
bash: line 9: [[: invalid regular expression `(': Unmatched ( or \(
! 0
bash: line 10: [[: invalid regular expression `(': Unmatched ( or \(
|| 0
bash: line 11: [[: invalid regular expression `(': Unmatched ( or \(
&& 2
bash: line 12: [[: invalid regular expression `(': Unmatched ( or \(
else 2
declare -a BASH_REMATCH=([0]="x")
+ [[ a.b =~ a\.b ]]
+ [[ abc =~ ab. ]]
+ re='x|(y)'
+ [[ zy =~ x|(y) ]]
+ [[ a =~ []] ]]
+ set +x
bash: line 20: [[: X: integer expected
t 2
bash: line 21: [[: X: integer expected
!t 0
bash: line 22: [[: X: integer expected
t|| 0
eq 1
done

== status 0
