# nocasematch folds case in case, [[ ]] and ${x/p/r} patterns, ranges
# and classes included, but not in ${x#p} or pathname expansion.
shopt -s nocasematch
case HELLO in hel*) echo case ;; esac
case Q in [a-z]) echo range ;; esac
case q in [[:upper:]]) echo class upper ;; *) echo class kept ;; esac
case Q in [[:lower:]]) echo class lower ;; *) echo class kept ;; esac
[[ MiXeD == mixed ]] && echo cond
[[ ABC == @(a|x)bc ]] && echo cond extglob
[[ abc != ABC ]] || echo ne folded
x=FooBar
echo "${x/foo/_}" "${x//[A-Z]/-}" "${x#foo}" "${x%bar}"
cd /tmp && mkdir nm && cd nm
touch Name
echo n*
