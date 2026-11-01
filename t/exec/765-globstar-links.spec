== stdout 251
**: dangling link real real/f real/sub real/sub/g
**/: link/ real/ real/sub/
**/g: real/sub/g
**/f: real/f
link/**: link/ link/f link/sub link/sub/g
link/*: link/f link/sub
*/: link/ real/
*/f: link/f real/f
d*: dangling
**/link: link
**/link/: link/

== stderr 0

== merged 251
**: dangling link real real/f real/sub real/sub/g
**/: link/ real/ real/sub/
**/g: real/sub/g
**/f: real/f
link/**: link/ link/f link/sub link/sub/g
link/*: link/f link/sub
*/: link/ real/
*/f: link/f real/f
d*: dangling
**/link: link
**/link/: link/

== status 0
