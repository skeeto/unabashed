== stdout 653
-- top level
exit: fn=f g v=global args=a b st=1
trap -- 'echo ret' RETURN
-- exit, for comparison
exit: fn=f g v=local args=a b st=3
-- eval
exit: fn=f g v=global args=a b st=1
-- subshells
exit: fn=f g v=local args=a b st=1
exit: fn=f g v=local args=a b st=1
exit: fn=f g v=local args=a b st=1
exit: fn=f g v=global args=a b st=1
exit: fn=f g h v=local args=a b st=1
-- command substitutions
[exit: fn=f g v=global args=a b st=1]
[exit]
[exit]
-- redirections
exit: fn= v=global args= st=1
exit: fn=k v=global args=y st=1
exit: fn=k v=global args=y st=1
exit: fn=k v=global args=y st=1
exit: fn= v=global args= st=1
exit: fn= v=global args= st=1
done

== stderr 97
echo "exit: fn=${FUNCNAME[*]} v=$v args=$* st=$?": line 1: local: can only be used in a function

== merged 750
-- top level
exit: fn=f g v=global args=a b st=1
trap -- 'echo ret' RETURN
echo "exit: fn=${FUNCNAME[*]} v=$v args=$* st=$?": line 1: local: can only be used in a function
-- exit, for comparison
exit: fn=f g v=local args=a b st=3
-- eval
exit: fn=f g v=global args=a b st=1
-- subshells
exit: fn=f g v=local args=a b st=1
exit: fn=f g v=local args=a b st=1
exit: fn=f g v=local args=a b st=1
exit: fn=f g v=global args=a b st=1
exit: fn=f g h v=local args=a b st=1
-- command substitutions
[exit: fn=f g v=global args=a b st=1]
[exit]
[exit]
-- redirections
exit: fn= v=global args= st=1
exit: fn=k v=global args=y st=1
exit: fn=k v=global args=y st=1
exit: fn=k v=global args=y st=1
exit: fn= v=global args= st=1
exit: fn= v=global args= st=1
done

== status 0
