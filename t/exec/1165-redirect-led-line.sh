# A simple command that starts with a redirection takes bash's
# simplecmd_lineno: where its first redirection's word ended, or the
# line of a word read in command position that began no simple command
# (a case pattern), or the line of a command substitution starting with
# < that bash parses first to look for $(< file)
<<<"a
b" <nope
<"a
b"
case x in
y) ;;
*)
<nope;;
esac
case x in (*)
<nope;; esac
: "$(<<<$(:
echo $LINENO >&2))"
: "$(<"a
b" <nope)"
: "$(:; <"a
b" <nope)"
: $(<x)

<nope
f() { : $(<x <y); }
f

<nope
x=$(case x in y) ;; *) >d1/f3; echo $LINENO >&2;; esac)
echo "end $?"
