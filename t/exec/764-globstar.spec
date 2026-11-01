== stdout 502
**: a a/b a/b/c a/b/c/z a/b/y a/x d d/e d/e/w top
**/: a/ a/b/ a/b/c/ d/ d/e/
**/*: a a/b a/b/c a/b/c/z a/b/y a/x d d/e d/e/w top
**/y: a/b/y
a/**: a/ a/b a/b/c a/b/c/z a/b/y a/x
a/**/: a/ a/b/ a/b/c/
a/**/z: a/b/c/z
**/b/**: a/b a/b/c a/b/c/z a/b/y
**/**: a a/b a/b/c a/b/c/z a/b/y a/x d d/e d/e/w top
**/**/z: a/b/c/z
a/**/**: a a/b a/b/c a/b/c/z a/b/y a/x
x**: x**
**x: **x
a**/y: a**/y
*/**: a a/b a/b/c a/b/c/z a/b/y a/x d d/e d/e/w
**/c: a/b/c
**/c/: a/b/c/
off **: a d top
off **/*: a/b a/x d/e

== stderr 0

== merged 502
**: a a/b a/b/c a/b/c/z a/b/y a/x d d/e d/e/w top
**/: a/ a/b/ a/b/c/ d/ d/e/
**/*: a a/b a/b/c a/b/c/z a/b/y a/x d d/e d/e/w top
**/y: a/b/y
a/**: a/ a/b a/b/c a/b/c/z a/b/y a/x
a/**/: a/ a/b/ a/b/c/
a/**/z: a/b/c/z
**/b/**: a/b a/b/c a/b/c/z a/b/y
**/**: a a/b a/b/c a/b/c/z a/b/y a/x d d/e d/e/w top
**/**/z: a/b/c/z
a/**/**: a a/b a/b/c a/b/c/z a/b/y a/x
x**: x**
**x: **x
a**/y: a**/y
*/**: a a/b a/b/c a/b/c/z a/b/y a/x d d/e d/e/w
**/c: a/b/c
**/c/: a/b/c/
off **: a d top
off **/*: a/b a/x d/e

== status 0
