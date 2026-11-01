== stdout 127
@(a|c)/*: a/b c/g
@(a/b|c): c
@(a/b|c)/*: c/g
*/@(b|g): a/b c/g
!(c)/*/f: a/b/f
+(a|c)/@(b/f): +(a|c)/@(b/f)
@(a|b)/b/f: a/b/f

== stderr 0

== merged 127
@(a|c)/*: a/b c/g
@(a/b|c): c
@(a/b|c)/*: c/g
*/@(b|g): a/b c/g
!(c)/*/f: a/b/f
+(a|c)/@(b/f): +(a|c)/@(b/f)
@(a|b)/b/f: a/b/f

== status 0
