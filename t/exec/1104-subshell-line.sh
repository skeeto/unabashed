# bash's child for a ( list ) sets its line, as only some commands set
# it, to the list's ) line: inside, a compound command's redirection
# error and the hook of a temporary binding as it goes cite that line,
# whether the list runs by itself, in the background or as a pipeline
# stage. The parent's line stays as it was.
exec 2>&1
export LC_ALL=x0
f() {
  ( :
    { :; } >/nonexistent/a
  )
  LC_ALL=x1 :
  ( LC_ALL=x2 :
    for i in 1; do :; done
    LC_ALL=x3 :
  )
  ( { :; } >/nonexistent/b
  ) &
  wait
  ( { :; } >/nonexistent/c
  ) | cat
  : | ( { :; } >/nonexistent/d
  )
  ( :
    ( { :; } >/nonexistent/e
    )
  )
  x=$( ( { :; } >/nonexistent/f
  ) )
  echo "$LINENO"
}
f
( LC_ALL=x4 :
)
