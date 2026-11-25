== stdout 1561
-- eval "$(cat d.sh)"
bash: command substitution: line 7: syntax error near unexpected token `echo' while looking for matching `)'
bash: command substitution: line 7: `esac echo no)]"'
after 1
exit 0
-- eval "$(cat n.sh)"
bash: command substitution: line 7: syntax error near unexpected token `echo' while looking for matching `)'
bash: command substitution: line 7: `esac echo no)")"'
exit 127
-- eval "$(cat u.sh)"
bash: command substitution: line 9: syntax error near unexpected token `echo' while looking for matching `)'
bash: command substitution: line 9: `esac echo no))'
exit 127
-- . ./n.sh
./n.sh: command substitution: line 7: syntax error near unexpected token `echo' while looking for matching `)'
./n.sh: command substitution: line 7: `esac echo no)")"'
exit 127
-- ( . ./n.sh; echo in ); echo "sub $?"
./n.sh: command substitution: line 7: syntax error near unexpected token `echo' while looking for matching `)'
./n.sh: command substitution: line 7: `esac echo no)")"'
sub 1
after 0
exit 0
-- f() { . ./n.sh; echo in; }; f
./n.sh: command substitution: line 7: syntax error near unexpected token `echo' while looking for matching `)'
./n.sh: command substitution: line 7: `esac echo no)")"'
exit 127
d.sh: command substitution: line 7: syntax error near unexpected token `echo' while looking for matching `)'
d.sh: command substitution: line 7: `esac echo no)]"'
script 1
n.sh: command substitution: line 7: syntax error near unexpected token `echo' while looking for matching `)'
n.sh: command substitution: line 7: `esac echo no)")"'
script 2

== stderr 0

== merged 1561
-- eval "$(cat d.sh)"
bash: command substitution: line 7: syntax error near unexpected token `echo' while looking for matching `)'
bash: command substitution: line 7: `esac echo no)]"'
after 1
exit 0
-- eval "$(cat n.sh)"
bash: command substitution: line 7: syntax error near unexpected token `echo' while looking for matching `)'
bash: command substitution: line 7: `esac echo no)")"'
exit 127
-- eval "$(cat u.sh)"
bash: command substitution: line 9: syntax error near unexpected token `echo' while looking for matching `)'
bash: command substitution: line 9: `esac echo no))'
exit 127
-- . ./n.sh
./n.sh: command substitution: line 7: syntax error near unexpected token `echo' while looking for matching `)'
./n.sh: command substitution: line 7: `esac echo no)")"'
exit 127
-- ( . ./n.sh; echo in ); echo "sub $?"
./n.sh: command substitution: line 7: syntax error near unexpected token `echo' while looking for matching `)'
./n.sh: command substitution: line 7: `esac echo no)")"'
sub 1
after 0
exit 0
-- f() { . ./n.sh; echo in; }; f
./n.sh: command substitution: line 7: syntax error near unexpected token `echo' while looking for matching `)'
./n.sh: command substitution: line 7: `esac echo no)")"'
exit 127
d.sh: command substitution: line 7: syntax error near unexpected token `echo' while looking for matching `)'
d.sh: command substitution: line 7: `esac echo no)]"'
script 1
n.sh: command substitution: line 7: syntax error near unexpected token `echo' while looking for matching `)'
n.sh: command substitution: line 7: `esac echo no)")"'
script 2

== status 0
