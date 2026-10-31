# Canonical command printing in bash 5.3 (`declare -f`, `type`)

This is a spec for rebuilding bash 5.3's command printer byte for byte.
It covers what `declare -f`, `typeset -f`, `type`, `command -V`, `jobs`
and exported-function environment strings show. It also covers how
`$( )`, `${ ; }` and `<( )` bodies are rewritten when they are parsed.
Every rule was checked against the oracle (`unabashed/bash53`, real bash
5.3.0, C locale) on 2026-10-31. The source references are to the bash 5.3
tree: `print_cmd.c` unless another file is named. The descriptions,
pseudocode and probe scripts are our own.

## 0. Notation

Each example shows a probe script (the input to `bash --norc --noprofile
-s`) and then the exact output. In the output:

- `␣` stands for a trailing space. Many lines end in one, and they matter.
- `→` stands for a literal TAB byte.
- Nothing else has been changed. Indentation is in real spaces.

Example (the basic shape):

    f() { echo hi; }
    declare -f
  →
    f ()␣
    {␣
        echo hi
    }

## 1. Where printing happens and the three modes

Function definitions are kept as parse trees. They are printed when they
are asked for:

| Caller | Entry (source) | Output |
|---|---|---|
| `declare -f [name...]`, `typeset -f` | `named_function_string(name, body, MULTILINE\|EXTERNAL)` (builtins/declare.def:518, builtins/setattr.def:496/523) | the string, then `\n` |
| `type f`, `command -V f` | same (builtins/type.def:293) | `f is a function\n`, then the string, then `\n` |
| exported function in the environment | `named_function_string(NULL, body, 0)` (variables.c:4875) | single-indent form (§14) |
| `jobs`, `$BASH_COMMAND`, error and trace text | `make_command_string` (top-level mode) | no trailing newline |
| `$( )`, `${ ; }`, `${\| ; }`, `<( )`, `>( )` at **parse time** | `print_comsub` (parse.y:4632) | text replaces the original in the word |

The printer works in one of three modes. The mode controls only how the
list connectors `;` and newline are printed, and how `{ }` groups are laid
out. Control structures (`if`, `for`, `case` and so on) are always printed
over several lines with the same layout.

- **Function mode** (the function-depth counter is above 0): inside the
  body of any function being printed, including a nested definition. A `;`
  or newline connector becomes `;` and a newline, and the next command is
  indented. Groups are printed over several lines.
- **Comsub mode** (the comsub flag is set, function depth is 0): while a
  `$( )` body is reprinted at parse time. A newline connector stays a
  newline, a `;` stays `; `, and groups are printed on one line.
- **Top-level mode** (neither): `jobs` and `$BASH_COMMAND`. Both
  connectors become `; ` and groups are printed on one line.

## 2. Printer state

Pseudocode below uses this state. It is all global to one printing
operation.

- `out`: the output buffer. Some rules look at its last one or two bytes.
- `level`: the current indentation in spaces. One step is **4** spaces
  (the env form uses steps of 0 with a base of 1, see §14).
- `skip`: a counter. When a command starts printing and `skip > 0`, it is
  decremented and no indentation is printed. Otherwise `level` spaces are
  printed.
- `fdepth`: the function-body depth, which selects function mode.
- `incomsub`: true while printing a comsub body.
- `cdepth`: how deep we are in nested *connection* nodes (lists, `&&`,
  `||`, `|`, `&`).
- `pending`: here-documents whose bodies are deferred (§9).
- `hd`: a flag meaning "the last thing printed was a here-document body".
  It suppresses the next `;` connector (§9.4). This is a source of
  visible quirks.

Helpers (our names):

- `nl(s)`: print `\n`, then `level` spaces, then `s`.
- `semi()`: print `;` unless `out` ends in `\n`, or ends in ` &` (space,
  ampersand).
- `flush(conn)`: deferred here-documents, §9.2.

## 3. Function definitions

### 3.1 The top-level header (`declare -f`, `type`)

`named_function_string` (line 1400). With the multiline flag that every
user-visible caller passes:

1. If the name looks like an assignment word (a valid identifier, then an
   optional subscript or `+`, then `=`, as decided by `assignment()`), print
   `function `. This is the only case where the keyword shows. It applies
   in posix mode too, because `valid_function_name` gets flags without bit
   1 (general.c:419).
2. Print the name, then ` () ` (space, parens, **trailing space**), then
   `\n`.
3. Print `{ ` (**trailing space**) and `\n`. The brace is never indented.
4. Set `level = 4`, raise `fdepth`. If the body is a `{ }` group, take its
   redirections off (they print after the closing brace) and print the
   group's *inner* command list. Otherwise print the body command itself
   (a subshell, `[[ ]]`, `(( ))`, `for`, `if` and so on), so it ends up
   wrapped in braces.
5. `flush("")`, then `level = 0` and lower `fdepth`.
6. If redirections were taken off: `nl("} ")`, then the redirection list
   (§8). Otherwise `nl("}")` and clear `hd`.
7. The caller prints one `\n`.

How you write the header (`f()`, `f ()`, `function f`, `function f ()`)
makes no difference:

    f() { echo hi; }
    function g { echo hi; }
    function h() { echo hi; }
    declare -f
  →
    f ()␣
    {␣
        echo hi
    }
    g ()␣
    {␣
        echo hi
    }
    h ()␣
    {␣
        echo hi
    }

Names that are not identifiers still get no keyword. Assignment-like names
do:

    function a-b { :; }
    function 1x { :; }
    function if { :; }
    function x=y { :; } 2>/dev/null
    declare -f
  →
    1x ()␣
    {␣
        :
    }
    a-b ()␣
    {␣
        :
    }
    if ()␣
    {␣
        :
    }
    function x=y ()␣
    {␣
        :
    } 2> /dev/null

(The `2>/dev/null` attaches to the body group, so it is printed as a
function redirection. The output is the same with `set -o posix`.)

### 3.2 Non-group bodies are wrapped in braces

    f1() ( echo sub )
    f2() [[ -n x ]]
    f3() (( x = 1 ))
    f4() for i in a b; do echo $i; done
    f5() if true; then :; fi
    f7() while false; do :; done
    declare -f
  →
    f1 ()␣
    {␣
        ( echo sub )
    }
    f2 ()␣
    {␣
        [[ -n x ]]
    }
    f3 ()␣
    {␣
        (( x = 1 ))
    }
    f4 ()␣
    {␣
        for i in a b;
        do
            echo $i;
        done
    }
    f5 ()␣
    {␣
        if true; then
            :;
        fi
    }
    f7 ()␣
    {␣
        while false; do
            :;
        done
    }

### 3.3 Redirections on the definition

Only the redirections of a **group** body move after the closing brace.
On a subshell body they stay on the subshell, inside the braces:

    f() { :; } > out 2>&1
    g() ( : ) > out
    k() { :; } > o1 < i1
    declare -f
  →
    f ()␣
    {␣
        :
    } > out 2>&1
    g ()␣
    {␣
        ( : ) > out
    }
    k ()␣
    {␣
        :
    } > o1 < i1

A here-document on the definition prints its body after the `}` line.
The body ends in `\n`, and `declare` adds one more, so a blank line
follows:

    h() { cat; } <<EOF
    body
    EOF
    declare -f
  →
    h ()␣
    {␣
        cat
    } <<EOF
    body
    EOF
    

### 3.4 Nested function definitions (inside a body or a comsub)

`print_function_def` (line 1323) prints a definition that is a command
inside something else:

1. The usual command prefix: indentation, time, `!` (§4).
2. Outside posix mode: always `function NAME () ` then `\n`. In posix mode:
   `NAME () ` then `\n`, unless the name looks like an assignment, which
   gets `function `.
3. `level` spaces, then `{ ` then `\n`.
4. `fdepth++`, `level += 4`. The group or body is printed as in §3.1 step 4,
   then `flush("")`, `level -= 4`, `fdepth--`.
5. With group redirections: `nl("} ")` and the list. Otherwise `nl("}")`
   and `hd = 0`.

    f() {
      g() { echo g; }
      function h { echo h; }
      k() ( echo k )
      m() { n() { echo n; }; }
      p() { :; } > out
    }
    declare -f f
  →
    f ()␣
    {␣
        function g ()␣
        {␣
            echo g
        };
        function h ()␣
        {␣
            echo h
        };
        function k ()␣
        {␣
            ( echo k )
        };
        function m ()␣
        {␣
            function n ()␣
            {␣
                echo n
            }
        };
        function p ()␣
        {␣
            :
        } > out
    }

In posix mode (`set -o posix; declare -f`) the same nested `g` prints as
`g ()␣`, and `h-i` as `h-i ()␣`, without `function`.

### 3.5 Empty and comment-only bodies

`f() { }` is a syntax error, so a body always has at least one command.
Comments are dropped by the lexer:

    f() { # comment
      :  # another
      # whole line
    }
    declare -f
  →
    f ()␣
    {␣
        :
    }

## 4. Printing one command (the dispatcher)

`make_command_string_internal` (line 176). For a null command, print
nothing at all, not even indentation. Otherwise:

1. If `skip > 0`, then `skip--`. Otherwise print `level` spaces.
2. If the command has the `time` flag: print `time `, and `-p ` if it was
   `time -p`.
3. If it has the invert flag: print `! `.
4. Print the type-specific body (§5 to §7).
5. If the *command node* has redirections, which happens only for compound
   commands, print ` ` and the redirection list (§8). A simple command
   prints its own redirections inside step 4.

The output order is always `time`, then `-p`, then `!`, whatever the
source order. `!` toggles, so a pair cancels (parse.y:1413):

    f() {
      ! l
      time m
      time -p n
      ! time o
      time ! p
      ! time -p a
      time
      ! ! a; ! ! ! b
    }
    declare -f f
  →
    f ()␣
    {␣
        ! l;
        time m;
        time -p n;
        time ! o;
        time ! p;
        time -p ! a;
        time ;
        a;
        ! b
    }

(A bare `time` is a time-flagged null command, so it prints `time ` and
the connector then adds `;`.)

## 5. Lists and pipelines (connection nodes)

The parser builds lists **left-deep**. `a; b; c` is `((a ; b) ; c)`. A
trailing `;` or newline adds no node. A trailing `&` gives a node with no
right side. **`a &` followed by a newline and `b` is the single node
`a & b`**: the newline is absorbed.

A newline between list elements becomes a `;` connector, **unless the list is
parsed inside a comsub body** (parse.y:1299-1305). Only comsub bodies,
and functions defined inside them, hold real newline connectors. So the
newline cases below matter in comsub mode. In function mode they print
the same as `;`.

`a |& b` is rewritten by the parser to `a 2>&1 | b` (parse.y:1473). A
`2>&1` redirection is appended to the left command: to its own list for a
simple command, or to the command node for a compound command. So it
always prints as `2>&1 |`.

Printing a connection `L op R`:

    skip++ ; cdepth++
    print(L)
    case op:
      '&', '|':  flush(" &" or " |")
                 if op is '|' or R exists: print " " ; skip++
      '&&','||': flush(" && " or " || ") ; if R exists: skip++
      ';', NL:   sep  = incomsub ? op : ';'     (literal ';' or newline byte)
                 wasnl = (pending empty) and (not hd) and (op is NL)
                 if pending is empty:
                     if not hd: print sep  else: hd = 0
                 else:
                     flush(fdepth ? "" : ";")
                 if fdepth:                      print "\n"         (next cmd indents)
                 else if incomsub and op is NL and not wasnl: print "\n"  (next cmd indents)
                 else: if op is ';': print " " ;  if R exists: skip++
    print(R)
    if cdepth == 1: flush("")
    cdepth--

So, with no here-documents:

| connector | function mode | comsub mode | top-level mode |
|---|---|---|---|
| `;` | `;` `\n` then indented | `; ` | `; ` |
| newline | `;` `\n` then indented | `\n`, next at column 0 | stored as `;`, so `; ` |
| `&` (with R) | ` & ` | ` & ` | ` & ` |
| `&` (no R) | ` &` | ` &` | ` &` |
| `\|` | ` \| ` | ` \| ` | ` \| ` |
| `&&` / `\|\|` | ` && ` / ` \|\| ` | same | same |

The skip counter means that the right operand of every connector except a
function-mode `;`/newline gets no indentation of its own. That is why a
newline inside a comsub `if` body leaves the next command at column 0
(§12.3).

    f() {
      a; b
      c &
      d & e
      f && g || h
      i | j |& k
    }
    declare -f f
  →
    f ()␣
    {␣
        a;
        b;
        c & d & e;
        f && g || h;
        i | j 2>&1 | k
    }

A trailing `&` leaves the right side empty:

    f() { a; b; c & }
    g() { a && b & }
    h() { a & b & c; }
    declare -f
  →
    f ()␣
    {␣
        a;
        b;
        c &
    }
    g ()␣
    {␣
        a && b &
    }
    h ()␣
    {␣
        a & b & c
    }

The `|&` rewrite, also on a compound command:

    f() { a 2>x |& b; a |& b |& c; { a; } |& b; }
    declare -f f
  →
    f ()␣
    {␣
        a 2> x 2>&1 | b;
        a 2>&1 | b 2>&1 | c;
        {␣
            a
        } 2>&1 | b
    }

Top-level mode, shown with `jobs`:

    { sleep 1
    sleep 2
    } &
    sleep 1 && sleep 2 &
    ( sleep 1 ) >/dev/null &
    jobs
  →
    [1]   Running                    { sleep 1; sleep 2; } &
    [2]-  Running                    sleep 1 && sleep 2 &
    [3]+  Running                    ( sleep 1 ) > /dev/null &

## 6. Simple commands and words

### 6.1 Layout

`print_simple_command` (line 1002) prints all the words (assignment words
included) joined by single spaces. Then, if there are redirections, it
prints ` ` (only when there were words) and the redirection list.
**Redirections always come after all the words**, in source order among
themselves, wherever they were written:

    f() {
      < x a b
      2>&1 >/dev/null c x=1
      x=1 >o
      > onlyredir
    }
    declare -f f
  →
    f ()␣
    {␣
        a b < x;
        c x=1 2>&1 > /dev/null;
        x=1 > o;
        > onlyredir
    }

### 6.2 Word text

Each word prints as its stored text. That text is the source spelling,
quotes and backslashes kept, with these changes made **by the lexer or
parser**, not by the printer:

1. Line continuations (backslash-newline) are removed, except inside
   single quotes.
2. Comments are dropped.
3. Aliases are expanded, so the printed text is the expansion.
4. `$'...'` is decoded and re-quoted with single quotes: `'` becomes
   `'\''`, and a result that is exactly one `'` becomes `\'`. Control bytes
   stay raw. Inside double quotes, `$'...'` stays as written, except
   inside `${...}` in double quotes (see the example below).
5. `$"..."` becomes `"..."` (no catalogue in the C locale).
6. `$( )`, `${ ; }`, `${| ; }`, `<( )` and `>( )` bodies are parsed and
   reprinted (§12), wherever they occur: unquoted, in double quotes, inside
   `${...}`, `$(( ))`, `(( ))`, `[[ ]]`, case words and patterns, for lists,
   array assignments and redirection targets. Backquotes, here-document
   bodies and single quotes are never rewritten.
7. Nothing else changes: `${...}`, `$((...))`, `$[...]`, globs, braces,
   `~`, `name=(...)` compound assignments and extra spaces inside `(( ))`
   all print verbatim.

Bytes 0x01 (CTLESC) and 0x7f (CTLNUL) come out raw: the `EXTERNAL` flag
strips the internal quoting (verified with `od -c`).

    f() {
      echo 'sq' "dq $x" $'a\tb' $"loc" \$y a\ b "a\"b" ${x:-def} ${#x} ${x//a/b} ~ ~/x
      echo $((1+2)) $[1+2] "$(( x*2 ))" `echo bq` "`echo bq2`"
      echo <(true) >(cat) a{b,c}d *.c
      x=1 y=(a b) z+=3 echo assign
      echo line \
        continued
      # comment
      echo after # trailing comment
    }
    declare -f f
  →
    f ()␣
    {␣
        echo 'sq' "dq $x" 'a→b' "loc" \$y a\ b "a\"b" ${x:-def} ${#x} ${x//a/b} ~ ~/x;
        echo $((1+2)) $[1+2] "$(( x*2 ))" `echo bq` "`echo bq2`";
        echo <(true) >(cat) a{b,c}d *.c;
        x=1 y=(a b) z+=3 echo assign;
        echo line continued;
        echo after
    }

ANSI-C and locale strings:

    f() {
      echo $'it\'s' $'a b' $'\x41' $'\n' "$'x'" $'' $"" $"a\"b" $'a'b'c' $'\\' x$'y'z
      echo $'\'' ${x:-$'a\tb'} "${x:-$'a\tb'}" $"a $x"
      echo "${x#$'\t'}" "${x:-$'\t'}" ${x#$'\t'} "${x/$'a'/$'b'}"
    }
    declare -f f
  →
    f ()␣
    {␣
        echo 'it'\''s' 'a b' 'A' '
    ' "$'x'" '' "" "a\"b" 'a'b'c' '\' x'y'z;
        echo \' ${x:-'a→b'} "${x:-a→b}" "a $x";
        echo "${x#'→'}" "${x:-→}" ${x#'→'} "${x/'a'/'b'}"
    }

(Inside double quotes and `${...}`: after a pattern operator such as `#`,
`%` or `/`, the decoded text is single-quoted. After `:-` and similar
operators it is inserted bare. See parse.y:4053-4093. This belongs to the
lexer, but it shows in the printed output.)

    f() {
      echo "a\
    b" 'c\
    d' e\
    f $(g\
    h) ${x\
    }
    }
    declare -f f
  →
    f ()␣
    {␣
        echo "ab" 'c\
    d' ef $(gh) ${x}
    }

Aliases are expanded in the stored text:

    shopt -s expand_aliases
    alias ll='ls -l'
    alias pp='echo a |'
    f() { ll x; pp cat; }
    declare -f f
  →
    f ()␣
    {␣
        ls -l x;
        echo a | cat
    }

## 7. Compound commands

In all the layouts below, "body" means the command is printed with the
dispatcher at the new `level`. Its first line is indented, because `skip`
is 0 there.

### 7.1 `for`, `select`

    "for NAME in " + words joined by " " + ";"
    nl("do\n") ; level += 4 ; body ; flush("") ; semi() ; level -= 4 ; nl("done")

- With no `in`, the parser supplies the single word `"$@"` (parse.y:895).
  An empty `in` list prints `for i in ;`.
- `for i in a b; { ...; }` prints as `do`/`done`.
- `select` is the same with `select`.

    f() {
      for i; do echo $i; done
      for i in; do echo $i; done
      for i in a 'b c' $(ls) {1..3}
      do
        break
      done
      for i in a b; { echo brace2; }
      select s; do :; done
    }
    declare -f f
  →
    f ()␣
    {␣
        for i in "$@";
        do
            echo $i;
        done;
        for i in ;
        do
            echo $i;
        done;
        for i in a 'b c' $(ls) {1..3};
        do
            break;
        done;
        for i in a b;
        do
            echo brace2;
        done;
        select s in "$@";
        do
            :;
        done
    }

### 7.2 Arithmetic `for`

    "for ((" + INIT + "; " + TEST + "; " + STEP + "))"
    nl("do\n") ; level += 4 ; body ; flush("") ; semi() ; level -= 4 ; nl("done")

There is no `;` after `))`. The text between `((` and `))` is split at
top-level `;` (make_cmd.c:240). Leading blanks of each part are skipped,
and the raw text up to the `;` is kept, trailing blanks included. An empty
part becomes `1`.

    f() {
      for ((i=0;i<3;i++)); do echo $i; done
      for ((;;)); do :; done
      for (( i = 0 ; i < 3 ; i++ )) { echo brace; }
      for ((   i = 0   ;   ;  )); do :; done
    }
    declare -f f
  →
    f ()␣
    {␣
        for ((i=0; i<3; i++))
        do
            echo $i;
        done;
        for ((1; 1; 1))
        do
            :;
        done;
        for ((i = 0 ; i < 3 ; i++ ))
        do
            echo brace;
        done;
        for ((i = 0   ; 1; 1))
        do
            :;
        done
    }

### 7.3 `while`, `until`

    "while " (or "until ") ; skip++ ; print(test) ; flush("") ; semi()
    if hd: level spaces + "do\n" ; hd = 0       else: " do\n"
    level += 4 ; body ; flush("") ; level -= 4 ; semi() ; nl("done")

A list as the test prints in the current mode, so in function mode
`while a; b; do` becomes two lines:

    f() {
      while a; b; do c; d; done
      until ! a; do :; done
    }
    declare -f f
  →
    f ()␣
    {␣
        while a;
        b; do
            c;
            d;
        done;
        until ! a; do
            :;
        done
    }

### 7.4 `if` / `elif` / `else`

    "if " ; skip++ ; print(test) ; flush("") ; semi()
    if hd: 4 spaces (the STEP, not level!) + "then\n" ; hd = 0     else: " then\n"
    level += 4 ; print(then-part) ; flush("") ; level -= 4
    if else-part: semi() ; nl("else\n") ; level += 4 ; print(else-part) ; flush("") ; level -= 4
    semi() ; nl("fi")

`elif` is parsed as `else` with a nested `if`, and prints that way:

    f() {
      if a; then b; elif c; then d; elif e; then :; else g; fi
      if a; b; then c; fi
      if ! a && b; then :; fi
    }
    declare -f f
  →
    f ()␣
    {␣
        if a; then
            b;
        else
            if c; then
                d;
            else
                if e; then
                    :;
                else
                    g;
                fi;
            fi;
        fi;
        if a;
        b; then
            c;
        fi;
        if ! a && b; then
            :;
        fi
    }

The "4 spaces, not `level`" quirk for a here-document in the test is
shown in §9.6.

### 7.5 `case`

    "case " + WORD + " in "                         (trailing space)
    level += 4
    for each clause (first = true):
        if not incomsub or not first: nl("")
        if the first pattern word is exactly "esac": print "("
        patterns joined by " | " ; print ")\n"
        level += 4 ; print(action) ; level -= 4      (null action prints nothing)
        flush("")
        nl(";;" or ";&" or ";;&")
    level -= 4
    nl("esac")

- The leading `(` is never printed, except before a literal first pattern
  `esac`.
- An empty clause prints a completely empty line (no spaces) between
  `pat)` and `;;`.
- In comsub mode the first clause stays on the `case ... in ` line (§12).

    f() {
      case $x in
        a) echo a;;
        b|c) echo bc ;&
        (d) echo d; echo d2 ;;&
        e)
          ;;
        'q'|"r"|s*) x ;;
      esac
      case $x in esac
      case $x in (esac) echo e;; esac
      case x in (a|esac) ;; esac
    }
    declare -f f
  →
    f ()␣
    {␣
        case $x in␣
            a)
                echo a
            ;;
            b | c)
                echo bc
            ;&
            d)
                echo d;
                echo d2
            ;;&
            e)
    
            ;;
            'q' | "r" | s*)
                x
            ;;
        esac;
        case $x in␣
        esac;
        case $x in␣
            (esac)
                echo e
            ;;
        esac;
        case x in␣
            a | esac)
    
            ;;
        esac
    }

### 7.6 `{ }` groups

    "{ "
    function mode:  "\n" ; level += 4 ; print(list) ; flush("") ; "\n" ; level -= 4 ; level spaces
    otherwise:      skip++ ; print(list) ; flush("") ; semi() ; " "
    "}" ; hd = 0

So in function mode `{` is followed by a trailing space and a newline:

    f() {
      { a; b; }
      { a; } > out
      { { a; }; }
      { ( a ) }
      echo x | { read y; } | ( cat )
      { a & }
      a && { b; c; } || d
    }
    declare -f f
  →
    f ()␣
    {␣
        {␣
            a;
            b
        };
        {␣
            a
        } > out;
        {␣
            {␣
                a
            }
        };
        {␣
            ( a )
        };
        echo x | {␣
            read y
        } | ( cat );
        {␣
            a &
        };
        a && {␣
            b;
            c
        } || d
    }

On one line (comsub and top-level modes): `{ a; b; }`, `{ a & }`.

### 7.7 `( )` subshells

    "( " ; skip++ ; print(list) ; flush("") ; " )" ; hd = 0

There is never a `;` before `)`. In function mode an inner list still uses
`;` and a newline, and the continuation is indented to the *current*
`level`, not deeper:

    f() {
      ( a; b )
      ( a ) 2>/dev/null
      ( ( a ) )
      ( { a; } )
      ( a & )
    }
    declare -f f
  →
    f ()␣
    {␣
        ( a;
        b );
        ( a ) 2> /dev/null;
        ( ( a ) );
        ( {␣
            a
        } );
        ( a & )
    }

### 7.8 `(( ))`

`((` + raw text + `))`. The text is exactly what was between the
parentheses, spaces kept. Comsubs inside it are reprinted.

    f() {
      ((x=1))
      ((  x  +=  2  ))
      (( a[$i] = $(echo 3) ))
      ! (( x ))
    }
    declare -f f
  →
    f ()␣
    {␣
        ((x=1));
        ((  x  +=  2  ));
        (( a[$i] = $(echo 3) ));
        ! (( x ))
    }

### 7.9 `[[ ]]`

`[[ ` + node + ` ]]`, where a node prints as:

- `! ` first if the node's invert flag is set. Repeated `!` toggles, so
  pairs cancel.
- A parenthesised node: `( ` + node + ` )`.
- `&&` / `||`: left + ` && ` / ` || ` + right. The tree follows the parse,
  with no extra parentheses.
- Unary: op + ` ` + operand.
- Binary: left + ` ` + op + ` ` + right. This covers `==`, `=`, `!=`,
  `=~`, `<`, `>` and the `-eq` family.
- Term: the word text verbatim. Regex and pattern words keep their
  quoting.
- **A lone word becomes unary `-n WORD`** (parse.y:5181), including words
  like `-xy` and `a=~b`.

Source spacing is normalised.

    f() {
      [[ ! a && (b || c) ]]
      [[ -f $x&&-d  "$y" ]]
      [[ $a =~ ^[0-9]+(x|y)\ z$ ]]
      [[ $x =~ a\ b"c d"[[:space:]] ]]
      [[ ! ! x ]]
      [[ ( ( x ) ) ]]
      [[ a < b && c > d ]]
      [[ -xy ]]
      [[ a=~b ]]
      [[ a == @(b|c) ]]
      [[ ! ! ! c ]]
    }
    declare -f f
  →
    f ()␣
    {␣
        [[ ! -n a && ( -n b || -n c ) ]];
        [[ -f $x && -d "$y" ]];
        [[ $a =~ ^[0-9]+(x|y)\ z$ ]];
        [[ $x =~ a\ b"c d"[[:space:]] ]];
        [[ -n x ]];
        [[ ( ( -n x ) ) ]];
        [[ a < b && c > d ]];
        [[ -n -xy ]];
        [[ -n a=~b ]];
        [[ a == @(b|c) ]];
        [[ ! -n c ]]
    }

### 7.10 `coproc`

`coproc `, then, **only if the coprocess command is not a simple command**,
its name and a space. The name is `COPROC` when none was given
(parse.y:1127). Then the command, with `skip++` so it is not indented:

    f() {
      coproc cat
      coproc myco { cat; }
      coproc ( cat )
      coproc NAME ( cat )
      coproc while :; do :; done
      coproc cat > out
      coproc N { a; } > out
    }
    declare -f f
  →
    f ()␣
    {␣
        coproc cat;
        coproc myco {␣
            cat
        };
        coproc COPROC ( cat );
        coproc NAME ( cat );
        coproc COPROC while :; do
            :;
        done;
        coproc cat > out;
        coproc N {␣
            a
        } > out
    }

### 7.11 Trailing `&` inside bodies

`semi()` prints nothing after ` &`, so a body ending in `&` gets no `;`:

    f() {
      if a; then b & fi
      while a; do b & done
      case x in x) a & ;; esac
      a &
    }
    declare -f f
  →
    f ()␣
    {␣
        if a; then
            b &
        fi;
        while a; do
            b &
        done;
        case x in␣
            x)
                a &
            ;;
        esac;
        a &
    }

## 8. Redirections

### 8.1 The list

`print_redirection_list` (line 1070) clears `hd` first, then:

    for each redirection r (in source order), separated by " ":
        if r is a here-document: print its header (§9.1) and append a copy to a local list H
        else: print r as in the table
    if H is not empty:
        if cdepth > 0: pending = H                     (replaces pending)
        else: print "\n" ; for each h in H: body(h) + delimiter(h) + "\n" ; hd = 1

### 8.2 Spelling of every form

`N` is the source fd. It is printed only where the table says. `{v}`
replaces the fd number wherever one would print. "word" is the target word
text, which follows the operator after a single space in the file forms
and directly in the dup forms.

| Source | Printed | Notes |
|---|---|---|
| `<w`, `N<w` | `< w`, `N< w` | N printed if not 0 |
| `>w`, `N>w` | `> w`, `N> w` | N printed if not 1 |
| `>>w` | `>> w` | N printed if not 1 |
| `>\|w` | `>\| w` | N printed if not 1 |
| `<>w` | `<> w` | N printed if not 0 |
| `<<<w` | `<<< w` | N printed if not 0. A `$'..'` word is pre-decoded (§6.2) |
| `<&N2`, `N<&N2` | `0<&N2`, `N<&N2` | fd **always** printed |
| `>&N2`, `N>&N2` | `1>&N2`, `N>&N2` | fd always printed |
| `<&word` (not a number) | `<&word` | N printed if not 0 |
| `>&word` (not a number) | `>&word` | N printed if not 1. Not turned into `&>` |
| `<&N2-` / `>&N2-` | `0<&N2-` / `1>&N2-` | fd always printed |
| `<&word-` / `>&word-` | `0<&word-` / `1>&word-` | fd always printed |
| `<&-`, `N<&-`, `>&-`, `N>&-` | `0>&-`, `N>&-`, `1>&-` | **always `>&-`**, fd always printed |
| `&>w` | `&> w` | |
| `&>>w` | `&>> w` | |
| `{v}OPw` | `{v}OP w` / `{v}OPw` | `{v}<&-` prints `{v}>&-` |
| `<<D`, `<<-D` | §9 | N printed if not 0 |

- An explicit default fd is dropped: `0<in` prints `< in`, `1>out` prints
  `> out`.
- An fd that does not fit in an int is not a redirection fd. It stays a
  separate word: `a 99999999999>x` prints `a 99999999999 > x`.
  `2147483647>y` prints as `2147483647> y`.

    f() {
      a <in >out >>app 2>err 2>>err2 0<in0 1>out1 3<in3 4>o4
      b >|clob 3>|c3 <>rw 5<>rw5 0<>rw0 &>both &>>both2 >&both3
      c <<<here 3<<<"h3" <<< $'x\ty'
      d <&3 >&4 2>&1 3<&- 3>&- <&- >&- 4<&5- 4>&5- <&$fd >&$fd 3<&$fd- 3>&$fd-
      e {v}>file {v}<file {v}>>f {v}<>f {v}>|f {v}<&3 {v}>&4 {v}>&- {v}<&- {v}<<<str {v}>&3- {v}<&$x-
      g 99>x 2147483647>y
      a <&3- >&3- 2<&3- <&$fd- >&$fd-
      d >&"$x" 2>&"2" 3>&2""
      e > $file >"q f" >${x}y
    }
    declare -f f
  →
    f ()␣
    {␣
        a < in > out >> app 2> err 2>> err2 < in0 > out1 3< in3 4> o4;
        b >| clob 3>| c3 <> rw 5<> rw5 <> rw0 &> both &>> both2 >&both3;
        c <<< here 3<<< "h3" <<< 'x→y';
        d 0<&3 1>&4 2>&1 3>&- 3>&- 0>&- 1>&- 4<&5- 4>&5- <&$fd >&$fd 3<&$fd- 3>&$fd-;
        e {v}> file {v}< file {v}>> f {v}<> f {v}>| f {v}<&3 {v}>&4 {v}>&- {v}>&- {v}<<< str {v}>&3- {v}<&$x-;
        g 99> x 2147483647> y;
        a 0<&3- 1>&3- 2<&3- 0<&$fd- 1>&$fd-;
        d >&"$x" 2>&"2" 3>&2"";
        e > $file > "q f" > ${x}y
    }

## 9. Here-documents

### 9.1 Header and body text

- Header: an optional fd (printed when not 0) or `{v}`, then `<<`, then `-`
  for `<<-`, then the delimiter. There is no space after `<<`.
- If any part of the delimiter was quoted, it is printed after quote
  removal, wrapped by the single-quote rule of §6.2: `'Q'`, `"D"` and `\E`
  all print as single-quoted, `E"F"G` prints `'EFG'`, `"a'b"` prints
  `'a'\''b'`, and `<<''` prints `<<''`. An unquoted delimiter prints as
  written (`<<$x`).
- The body is the stored text. With `<<-`, leading tabs are already
  stripped, so they do not come back. The body is not expanded or
  reformatted: comsubs and backquotes in it stay verbatim, and trailing
  spaces are kept. It ends with its last newline (an empty body is empty).
  It is followed by the delimiter and `\n`.

    f() {
      cat <<-EOF
    →tabbed
    →EOF
      cat <<'Q'
    lit $x
    Q
      cat <<"D"
    d
    D
      cat <<\E
    e
    E
      cat <<E"F"G
    efg
    EFG
      cat <<"a'b"
    x
    a'b
      cat <<EOF
    EOF
      cat <<EOF
    $(a;  b) `c;  d` ${x}
    EOF
      cat 3<<X 4<<-Y {v}<<Z
    x
    X
    →y
    →Y
    z
    Z
      echo done
    }
    declare -f f
  →
    f ()␣
    {␣
        cat <<-EOF
    tabbed
    EOF
    
        cat <<'Q'
    lit $x
    Q
    
        cat <<'D'
    d
    D
    
        cat <<'E'
    e
    E
    
        cat <<'EFG'
    efg
    EFG
    
        cat <<'a'\''b'
    x
    a'b
    
        cat <<EOF
    EOF
    
        cat <<EOF
    $(a;  b) `c;  d` ${x}
    EOF
    
        cat 3<<X 4<<-Y {v}<<Z
    x
    X
    y
    Y
    z
    Z
    
        echo done
    }

### 9.2 When bodies print: right away or deferred

All the headers of one command print in order, on the command's line,
mixed with its other redirections. The bodies print as one block:
`\n`, then for each here-document `body + delimiter + \n`, and `hd`
becomes 1.

- If the command is **not inside any connection** (`cdepth == 0`), the
  block prints right after the command's redirections. Examples: the only
  command of a function body, the only command of a comsub, or a function
  definition's own redirections.
- Otherwise the here-documents go to `pending` and come out at the next
  `flush`.

`flush(conn)` (`print_deferred_heredocs`, line 1053):

    if conn is non-empty and conn is not exactly ";": print conn
    if pending is not empty:
        print the body block ("\n", each body+delim+"\n") ; hd = 1
        if conn is non-empty and not exactly ";": print " "
        pending = empty
    (hd stays 1 after this)

`flush("")` is called after the inner part of `for`, `select`, arithmetic
`for`, `while`/`until` (test and body), `if` (each part), case actions,
groups, subshells and function bodies, and at the end of the outermost
connection (`cdepth == 1`).

### 9.3 In lists, after a connector

In function mode a `;`/newline connector with pending bodies prints the
block and then a newline. Since the block already ends in `\n`, **a blank
line follows every here-document** before the next command:

    f() {
      cat <<A; cat <<B
    a
    A
    b
    B
      echo end
    }
    declare -f f
  →
    f ()␣
    {␣
        cat <<A
    a
    A
    
        cat <<B
    b
    B
    
        echo end
    }

With `|`, `&&`, `||` and `&`, the connector symbol prints *before* the
bodies, and the next command starts on the line after the delimiter.
`&&`/`||` print their trailing space, then the block, then one more space:

    f() {
      cat <<A | grep x <<B && echo yes <<C || no
    a
    A
    b
    B
    c
    C
      cat <<A &
    a
    A
      echo after
    }
    declare -f f
  →
    f ()␣
    {␣
        cat <<A |
    a
    A
      grep x <<B &&␣
    b
    B
     echo yes <<C ||␣
    c
    C
     no
        cat <<A &
    a
    A
      echo after
    }

(` no` has no `;` before the newline. Its connector found `hd` still set
from the bodies flushed at `||`, as §9.4 describes.
After `|` the next line starts with two spaces: the one printed after
the block and the normal ` ` after `|`. After `&&`/`||` there is one. The
`&`/newline/`echo after` case is the absorbed-newline node of §5.)

A pipeline of three: the first body comes out at the first `|`. The last
command's body waits for the enclosing list connector:

    f() {
      a <<A | b | c <<C
    a
    A
    c
    C
      echo z
    }
    declare -f f
  →
    f ()␣
    {␣
        a <<A |
    a
    A
      b | c <<C
    c
    C
    
        echo z
    }

### 9.4 The `hd` carry quirk (missing `;`)

After a connector prints deferred bodies, `hd` stays 1. It is cleared only
by the next `;`/newline connector (which then prints **no `;`**), by
`print_redirection_list` (any command that has redirections), by
groups, subshells and function definitions, and by the `if`/`while`
"test ended in a heredoc" paths. So in function mode, **the command after
a here-document command loses its `;`** if it has no redirections of its
own and is not a group, subshell or function:

    f() {
      cat <<EOF
    x
    EOF
      echo a
      echo b
      echo c
    }
    g() {
      cat <<EOF
    x
    EOF
      for i in 1; do :; done
      echo b
    }
    h() {
      cat <<EOF
    x
    EOF
      ( sub )
      echo b
    }
    k() {
      cat <<EOF
    x
    EOF
      echo a > out
      echo b
    }
    declare -f
  →
    f ()␣
    {␣
        cat <<EOF
    x
    EOF
    
        echo a
        echo b;
        echo c
    }
    g ()␣
    {␣
        cat <<EOF
    x
    EOF
    
        for i in 1;
        do
            :;
        done
        echo b
    }
    h ()␣
    {␣
        cat <<EOF
    x
    EOF
    
        ( sub );
        echo b
    }
    k ()␣
    {␣
        cat <<EOF
    x
    EOF
    
        echo a > out;
        echo b
    }

The same happens after a compound command whose body ends in a
here-document (here `hd` is set by the inner `flush("")`):

    f() {
      if true; then cat <<A; fi
    a
    A
      while read x; do echo $x; done <<EOF
    a
    EOF
      echo after
      case x in x) cat <<A;; esac
    a
    A
      { read y; } <<EOF
    b
    EOF
    }
    declare -f f
  →
    f ()␣
    {␣
        if true; then
            cat <<A
    a
    A
    
        fi
        while read x; do
            echo $x;
        done <<EOF
    a
    EOF
    
        echo after
        case x in␣
            x)
                cat <<A
    a
    A
    
            ;;
        esac
        {␣
            read y
        } <<EOF
    b
    EOF
    
    }

### 9.5 Last command, and here-documents inside groups

A here-document on the last command (or the only one) is followed by the
block's `\n` and then `nl("}")`, which gives a blank line before `}`:

    f() {
      cat <<EOF
    x
    EOF
    }
    g() { cat <<EOF; }
    y
    EOF
    m() {
      echo pre
      cat <<EOF
    x
    EOF
    }
    declare -f
  →
    f ()␣
    {␣
        cat <<EOF
    x
    EOF
    
    }
    g ()␣
    {␣
        cat <<EOF
    y
    EOF
    
    }
    m ()␣
    {␣
        echo pre;
        cat <<EOF
    x
    EOF
    
    }

`cdepth` is global, so a here-document anywhere inside a list (even
inside a group in that list) is deferred and flushed by the innermost
construct that calls `flush("")`:

    f() {
      cat <<A &
    a
    A
      { cat <<A; } > out
    a
    A
      echo z
    }
    declare -f f
  →
    f ()␣
    {␣
        cat <<A &
    a
    A
      {␣
            cat <<A
    a
    A
    
        } > out;
        echo z
    }

### 9.6 Here-document in an `if`/`while` test

When the test ends with a here-document (`hd` set), the keyword goes on
its own line. `while`/`until` indent `do` by `level`. **`if` indents
`then` by exactly 4 spaces, whatever the depth**:

    f() {
      if a; then
        if cat <<A; then
    a
    A
          :
        fi
        while cat <<W; do
    w
    W
          :
        done
      fi
    }
    declare -f f
  →
    f ()␣
    {␣
        if a; then
            if cat <<A
    a
    A
        then
                :;
            fi;
            while cat <<W
    w
    W
            do
                :;
            done;
        fi
    }

(The inner `then` sits at column 4 although the `if` is at column 8. The
`fi;` and `done;` get their `;` because these paths clear `hd`. In an
`elif` the same thing happens, since `elif` is a nested `if`.)

### 9.7 Here-documents in nested function definitions

    f() {
      g() { cat; } <<EOF
    hd
    EOF
      echo after
    }
    declare -f f
  →
    f ()␣
    {␣
        function g ()␣
        {␣
            cat
        } <<EOF
    hd
    EOF
    
        echo after
    }

## 10. `declare -f`, `declare -F`, `type`: listing rules

- **No names:** every function, sorted. The sort compares the first bytes
  as **signed** chars (x86), then the whole names with `strcmp`
  (variables.c:4256). It does not depend on the locale. Bytes 0x80 and up
  in the first position therefore sort *before* ASCII. Functions are
  printed back to back, with no blank line between them.
- After a function with attributes other than plain `f`, `declare -f`
  prints a line `declare -f<attrs> name` (builtins/setattr.def:496-510).
  The letters seen are `r`, `t` and `x`, in that order (`-frtx`).
- `declare -f name...`: the names in argument order, repeats included,
  and **no attribute lines**. A missing name prints nothing, and the exit
  status is 1.
- `declare -F`: `declare -f name` lines in the same sorted order.
- `type f` and `command -V f`: `f is a function` then the definition.
  `type -t f`: `function`.

    zz() { :; }
    aa() { :; }
    B() { :; }
    _x() { :; }
    é() { :; }
    declare -F
  →
    declare -f é
    declare -f B
    declare -f _x
    declare -f aa
    declare -f zz

Attribute lines:

    f() { :; }
    g() { :; }
    h() { :; }
    declare -ft f
    declare -fx g
    declare -fr g
    export -f h
    readonly -f h
    declare -ft h
    declare -f
    echo ---
    declare -f g
  →
    f ()␣
    {␣
        :
    }
    declare -ft f
    g ()␣
    {␣
        :
    }
    declare -frx g
    h ()␣
    {␣
        :
    }
    declare -frtx h
    ---
    g ()␣
    {␣
        :
    }

`type` and missing names:

    f() { echo hi; }
    type f
    type -t f
    declare -f nonexist; echo "status $?"
    declare -f f nonexist f; echo "status $?"
  →
    f is a function
    f ()␣
    {␣
        echo hi
    }
    function
    status 1
    f ()␣
    {␣
        echo hi
    }
    f ()␣
    {␣
        echo hi
    }
    status 1

## 11. The final newline

The function string never ends in a newline of its own. It ends in `}`,
in `} REDIRS`, or in `DELIM\n` when the definition has a here-document.
Every caller adds exactly one `\n`. That is where the blank line after a
definition-level here-document comes from (§3.3).

## 12. Command substitution bodies (parse-time reprint, 5.2+)

When the parser meets `$(`, `${ `, `${|`, `<(` or `>(`, it parses the
body as a full command list. It then **replaces the body text** with the
printer's output in comsub mode (`print_comsub`, parse.y:4632). The
printer state starts fresh: `level = 0`, `fdepth = 0`, `hd = 0`, nothing
pending. The word then holds this text, so the reprint shows in
`declare -f`, `jobs` and everywhere else. Its line breaks are relative to
column 0 and are **not** re-indented when the enclosing function is
printed.

### 12.1 Wrapping

- `$( )`, `<( )`, `>( )`: the open token, then the text, then `)`. If the
  text starts with `(`, a space goes before it (`$( ( a ))`), so that it
  cannot be read as `$((`. No space is added at the end. An empty body,
  even one of only blanks or newlines, gives `$()`.
- `${ ... }` and `${| ... }`: after the `${`, print ` ` for the plain
  form, or `|` for the `|` form (the blanks the user typed after `|` are
  lost). Then the text, then a terminator chosen by the **last token
  before `}`** (parse.y:4552-4560, 4648-4668):
  - newline: print `\n`;
  - `;`: print `;`;
  - otherwise, if the text does not end in `\n`, `;` or `&`: print `;`.

  Then ` }`. An empty body gives `${ }`.

### 12.2 Body text

- The comsub-mode connector rules from §5 apply: `;` gives `; `, a
  newline stays a newline (the next command at column 0), `&` gives
  ` & `. Leading and trailing newlines, a trailing `;`, and blank lines
  vanish. Comments vanish.
- Groups print on one line (`{ a; b; }`). Control structures print on
  several lines with 4-space steps from column 0.
- `case`: the first clause stays on the `case W in ` line (`case $z in
  a)`). The later clauses start on new lines. A `case` with no clauses
  prints `case x in ` then a newline, then `esac`.
- A function defined inside a comsub prints in function mode (§3.4,
  `function g () `), and so does its body. A `case` inside it still keeps
  its first clause inline, because the comsub flag is still set.
- Here-documents: in a lone command they print right away (§9.2). In a
  list they are deferred, and at a `;` connector the `;` is not printed,
  and a single space follows the block.

    f() {
      echo $(echo   a;echo b) "$(  cd /; pwd  )"
      x=$(
        echo one
        echo two
      )
      y=$(case $z in a) echo A;; b|c) echo BC;; esac)
      w=$( (echo sub) )
      v=$(if true; then echo t; else echo f; fi)
    }
    declare -f f
  →
    f ()␣
    {␣
        echo $(echo a; echo b) "$(cd /; pwd)";
        x=$(echo one
    echo two);
        y=$(case $z in a)
            echo A
        ;;
        b | c)
            echo BC
        ;;
    esac);
        w=$( ( echo sub ));
        v=$(if true; then
        echo t;
    else
        echo f;
    fi)
    }

More cases:

    f() {
      a=$({ a; b; })
      b=$(echo $(x;y) "$(p;q)")
      c=$(echo a # comment
    echo b)
      d=$(a;)
      e=$(a &
    b)
      g=$(a;
    
    b)
      h=$( ( a ) | b )
      i=$(while a; do b; done)
      j=$(case x in a) ;; b) x;;& c) y;& esac)
      k=$(g() { a; b; }; { c; d; })
      m=$(! a | b)
      n=$(time a)
      o=$([[ -n x ]] && (( y )))
      p=$(a 2>&1 >/dev/null <in)
      q="${x:-$(a;b)}"
      r=$(echo `a;  b`)
      s=$(a |& b)
      t=$()
      u=$( )
      v=$(
    )
    }
    declare -f f
  →
    f ()␣
    {␣
        a=$({ a; b; });
        b=$(echo $(x; y) "$(p; q)");
        c=$(echo a
    echo b);
        d=$(a);
        e=$(a & b);
        g=$(a; b);
        h=$( ( a ) | b);
        i=$(while a; do
        b;
    done);
        j=$(case x in a)
    
        ;;
        b)
            x
        ;;&
        c)
            y
        ;&
    esac);
        k=$(function g ()␣
    {␣
        a;
        b
    }; { c; d; });
        m=$(! a | b);
        n=$(time a);
        o=$([[ -n x ]] && (( y )));
        p=$(a 2>&1 > /dev/null < in);
        q="${x:-$(a; b)}";
        r=$(echo `a;  b`);
        s=$(a 2>&1 | b);
        t=$();
        u=$();
        v=$()
    }

### 12.3 Newlines inside comsub control structures

The right side of a newline connector is printed with `skip` set, so in
comsub mode **it is not indented, even inside an indented body**:

    f() {
      x=$(if a; then b; c; fi)
      y=$(if a; then
        b
        c
      fi)
      z=$(while a
      do
        b
        c
      done)
      w=$( { a
      b; } )
    }
    declare -f f
  →
    f ()␣
    {␣
        x=$(if a; then
        b; c;
    fi);
        y=$(if a; then
        b
    c;
    fi);
        z=$(while a; do
        b
    c;
    done);
        w=$({ a
    b; })
    }

Top-level mode does the same with `; ` (shown with `jobs`):

    { if sleep 1; then sleep 2; sleep 3; fi; } &
    jobs
  →
    [1]+  Running                    { if sleep 1; then
        sleep 2; sleep 3;
    fi; } &

### 12.4 Here-documents in comsubs

    f() {
      x=$(cat <<EOF
    body $y
    EOF
    )
      x=$(cat <<A
    a
    A
    echo two
    echo three)
      z=$(cat <<A; cat <<B
    a
    A
    b
    B
    )
      l=$(cat <<A; echo after
    x
    A
    )
      w=$(cat <<A | wc -l
    a
    A
    )
      v=$(case a in a) cat <<A;; esac
    a
    A
    )
    }
    declare -f f
  →
    f ()␣
    {␣
        x=$(cat <<EOF
    body $y
    EOF
    );
        x=$(cat <<A
    a
    A
    
    echo two
    echo three);
        z=$(cat <<A
    a
    A
     cat <<B
    b
    B
    );
        l=$(cat <<A
    x
    A
     echo after);
        w=$(cat <<A |
    a
    A
      wc -l);
        v=$(case a in a)
            cat <<A
    a
    A
    
        ;;
    esac)
    }

(The `)` after a body that ends in a here-document lands at column 0 on
the line after the delimiter.)

### 12.5 `${ ; }` and `${| ; }`

    f() {
      echo ${ echo a; } ${|REPLY=x;} ${ echo b
      echo c; }
      echo ${ :;}
      x=${ echo a
    }
      y=${ a & }
      z=${| a; b
      }
      w=${ ( a ) ; }
      v=${ if a; then b; fi; }
      u=${ if a; then b; fi }
    }
    declare -f f
  →
    f ()␣
    {␣
        echo ${ echo a; } ${|REPLY=x; } ${ echo b
    echo c; };
        echo ${ :; };
        x=${ echo a
     };
        y=${ a & };
        z=${|a; b
     };
        w=${ ( a ); };
        v=${ if a; then
        b;
    fi; };
        u=${ if a; then
        b;
    fi; }
    }

A funsub whose body ends in a here-document gets the terminator after the
delimiter line. Bash also writes `bash: line 2: warning: command
substitution: 1 unterminated here-document` to stderr, because the body
is read after the line:

    exec 2>/dev/null
    f() {
      y=${ cat <<A; }
    a
    A
    }
    declare -f f
  →
    f ()␣
    {␣
        y=${ cat <<A
    a
    A
    ; }
    }

### 12.6 Process substitution

    f() {
      diff <( a;  b ) >( c
    d )
      cat < <(x)
    }
    declare -f f
  →
    f ()␣
    {␣
        diff <(a; b) >(c
    d);
        cat < <(x)
    }

### 12.7 Comsubs in other word positions

    f() {
      case $(a;b) in $(c;d)) ;; esac
      [[ $(a;b) == x ]]
      (( $(a;b) + 1 ))
      for i in $(a;b); do :; done
      echo $(( $(a;b) + 1 )) $(( (1+2) ))
      echo > $(a;b)
      echo $(echo $(echo a;b);c)
      echo ${x/$(a;b)/c} "${x/$(a;b)/c}"
      a=($(a;b) "$(c;d)")
    }
    declare -f f
  →
    f ()␣
    {␣
        case $(a; b) in␣
            $(c; d))
    
            ;;
        esac;
        [[ $(a; b) == x ]];
        (( $(a; b) + 1 ));
        for i in $(a; b);
        do
            :;
        done;
        echo $(( $(a; b) + 1 )) $(( (1+2) ));
        echo > $(a; b);
        echo $(echo $(echo a; b); c);
        echo ${x/$(a; b)/c} "${x/$(a; b)/c}";
        a=($(a; b) "$(c; d)")
    }

If a here-document command comes before this `case`, as in our first
probe, the `esac` loses its `;`. That is the §9.4 carry, not a comsub
effect.

## 13. Indentation summary

- One step is 4 spaces. The top-level function body starts at 4, and
  every `do`/`then`/`else`/group/function body adds 4.
- Case: patterns at +4 from the `case`, actions at +8, terminators at +4,
  and `esac` level with `case`.
- `do` (`for`, `select`, arithmetic `for`) sits on its own line at the
  loop's level. `while`/`until` put ` do` on the test line, and `if` puts
  ` then` there, unless the test ended in a here-document (§9.6).
- `{` of a top-level function is at column 0. A nested function's
  `function g () ` line is at its own command indentation, and its `{` and
  `}` are at that level too.
- Lines inside here-document bodies and comsub texts are verbatim, at
  their own columns.

## 14. Exported-function environment form

`export -f f` puts `BASH_FUNC_f%%=` plus `named_function_string(NULL,
body, 0)` into the environment. With no name and no multiline flag, the
header is `() ` and then `{ ` on the same line. The base indentation is 1
and the step is 0, so every indented line starts with exactly one space,
at any depth. The closing `}` is at column 0. Connectors are still in
function mode.

    f() { a; b | c; if x; then y; fi; }
    g() { cat <<EOF
    hd
    EOF
    }
    h() { :; } >out
    export -f f g h
    env | grep -A6 BASH_FUNC
  → (bytes, from od -c)
    BASH_FUNC_h%%=() {  :\n} > out\n
    BASH_FUNC_g%%=() {  cat <<EOF\nhd\nEOF\n\n}\n
    BASH_FUNC_f%%=() {  a;\n b | c;\n if x; then\n y;\n fi\n}\n

(The cleanup in `named_function_string` that deletes a byte when the
third byte is `\n` never fires for this form, which starts with `() {`.)

## 15. `set -x`, `$BASH_COMMAND` (low priority)

These use other code paths (`xtrace_*` in print_cmd.c) and print
**expanded** values:

    set -x
    echo "a b" 'c' >/dev/null
    [[ -n "a b" ]]
    (( 1 + 2 ))
    for i in a; do :; done
    x=$'a\tb'
  → (stderr)
    + echo 'a b' c
    + [[ -n a b ]]
    + ((  1 + 2  ))
    + for i in a
    + :
    + x=$'a\tb'

(`(( ))` traces as `(( ` + raw text + ` ))`, so the spaces double. Words
are quoted for re-input, and `[[ ]]` operands are not quoted.)

`$BASH_COMMAND` (a DEBUG trap) holds the top-level-mode print of the
current simple command. For `case`, it holds the head only:

    trap 'echo "[$BASH_COMMAND]"' DEBUG
    x=1 echo hi >/dev/null 2>&1
    echo $(echo a;  echo b) >/dev/null
    [[ -n x ]]
    (( 1 ))
    case a in a) : ;; esac
  →
    [x=1 echo hi > /dev/null 2>&1]
    [echo $(echo a; echo b) > /dev/null]
    [[[ -n x ]]]
    [(( 1 ))]
    [case a in ]
    [:]

## 16. Checklist

Verified against the oracle, with an example above:

- [x] Header forms `f()`, `function f`, `function f()` all print as `f () `
  with trailing spaces on `f () ` and `{ `.
- [x] `function` prefix only for assignment-like names at top level;
  always for nested definitions outside posix mode; posix-mode variant.
- [x] Non-group bodies: subshell, `[[ ]]`, `(( ))`, `for`, `if`, `case`,
  `while` are wrapped in braces.
- [x] Redirections on the definition: group (after `}`) vs subshell
  (inside). A here-document on the definition gives a blank line.
- [x] 4-space indentation and nesting levels.
- [x] `;` vs newline in the three modes. The absorbed newline after `&`.
  `&&`, `||`, `|`, `|&` printed as `2>&1 |`, trailing `&`.
- [x] `!`, `time`, `time -p`: output order, toggling, bare `time`.
- [x] Simple commands: assignments, words, redirections moved last.
- [x] Every redirection operator: fd omission rules, `{var}`, the
  always-`>&-` close form, the always-fd dup and move forms, oversized fd.
- [x] Here-documents: `<<-` (tabs gone), quoted delimiters (all forms and
  an embedded `'`), unquoted `$x` delimiter, empty body, empty delimiter,
  trailing spaces, several on one command, mixed with other redirections.
- [x] Here-document placement: immediate vs deferred, in pipelines, in
  `&&`/`||`, after `&`, in groups, subshells, `if`/`while`/`until`
  tests and bodies, case actions, for bodies, nested function definitions,
  comsubs.
- [x] The `hd` carry (missing `;`), and the `then` at a fixed 4 spaces.
- [x] `if`/`elif`/`else`, `while`/`until`, `for` with and without `in`,
  empty `in`, brace-bodied `for`, arithmetic `for` (splitting, `1`
  defaults, kept spaces), `select`.
- [x] `case`: `|`, the three terminators, empty clauses, the leading `(`
  only for `esac`, `case x in esac`.
- [x] `(( ))` verbatim. `[[ ]]`: operator spacing, `-n` insertion, `!`
  toggling, parentheses, `=~` quoting kept.
- [x] Subshells, groups (both layouts), coproc (unnamed simple, `COPROC`
  default name, named, with redirections).
- [x] Words: quotes, escapes, `$'...'` re-quoting (including inside `${}`
  in double quotes), `$"..."`, raw control bytes, line continuations,
  comments, aliases.
- [x] Comsub reprinting: `$( )` (layout, `$( (`, empty, nested, in quotes
  and `${}`), backquotes untouched, `${ ; }` / `${| ; }` terminators,
  `<( )`/`>( )`, here-documents inside, case inside, function inside,
  column-0 continuation lines.
- [x] `declare -f` ordering (signed first byte), attribute lines, named
  lookups, missing names, `declare -F`, `type`, `command -V`.
- [x] Final newline. Exported env form. `jobs` top-level form.

Not fully resolved:

1. Whether stale printer state (`skip`, `cdepth`) can leak from a
   parse-time comsub reprint into a later `declare -f`. The source resets
   only the buffer, `hd`, `pending` and the comsub flag at each entry
   point. No probe has shown a leak. Reset everything per print, unless a
   counterexample turns up.
2. Here-document bodies with no final newline (the delimiter missing at
   EOF), and here-documents whose delimiter needs a newline the body
   lacks: not probed.
3. The exact rules for `$'...'` and `$"..."` inside `${...}` in double
   quotes, for every operator (`DOLBRACE_*` states), and with `shopt -u
   extquote` or `noexpand_translation`: only `#`, `/` and `:-` were probed.
4. `$"..."` when a message catalogue translates the string
   (`singlequote_translations` path): not probed (C locale only).
5. The sort order on a target where `char` is unsigned (ARM): it
   would differ for names whose first byte is 0x80 or higher. We should
   match x86 bash.
6. `set -x` and `$BASH_COMMAND` are covered only by the samples in §15.
7. A `case` with no clauses inside a function inside a comsub, and
   pathological `pending` overwrites (a compound command that has
   here-documents in its own redirections *and* in a still-pending inner
   command at the same connection depth): not probed.
