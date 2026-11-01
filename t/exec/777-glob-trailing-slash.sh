# A trailing slash keeps only directories (links to them included),
# and the slashes are kept as written.
cd /tmp && mkdir ts && cd ts
mkdir d1 d2 .d3
touch f1
ln -s d1 l1
ln -s f1 l2
show() { printf '%s:' "$1"; shift; printf ' %s' "$@"; echo; }
show '*/'      */
show '*//'     */ /
show '*//'     *//
show '.*/'     .*/
show 'd*/.'    d*/.
show '*/..'    */..
show '/tmp/ts/d*/' /tmp/ts/d*/
show '//tmp//ts//d?' //tmp//ts//d?
show './*1'    ./*1
show './/l*/'  .//l*/
