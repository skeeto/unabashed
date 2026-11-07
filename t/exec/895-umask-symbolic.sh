# umask with symbolic modes: who, operators, copies, X, several
# clauses, -S printing the result, and the error messages
for m in u=r+w u=r-w g+u,o+rwx-u u=r+w,g=wx,o+xr u+w=r+x o=u g=u \
         u=rwx,u-w u=xwr +xwr a+xwr +xr a+xr g+X o+X +X g+x,o+x \
         u+g,g+o,o-rw a= =r ug=rwx,o= u+s o+t go-w; do
    umask 022; umask "$m"; echo "$m: $? $(umask) $(umask -S)"
done
umask 077; umask g+X; umask -S
umask 022; umask -S u=rx; echo "st $?"
umask 022; umask -p -S o=rwx
for m in u u=rq q=r u=r, 'u=r;' u=r,,g=w '' - x; do
    umask 022; umask "$m"; echo "$m: $? $(umask)"
done
