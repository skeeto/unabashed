// help: the help builtin's topics, and every builtin's --help
//
// Synopses are bash's, since scripts and its messages depend on them;
// the descriptions are this project's own words (notes/m7.md).
// This is free and unencumbered software released into the public domain.


struct HelpTopic {
    Str name;
    Str synopsis;
    Str text;  // first line: the one-line summary (help -d)
};

// Sorted by name, as help lists them
static HelpTopic const helptopics[] = {
    {"!", "! PIPELINE",
     "Run a pipeline and invert its status.\n"
     "\n"
     "The pipeline runs as usual; ! turns a zero status into 1 and any\n"
     "other status into 0. Under set -e, a negated pipeline never ends\n"
     "the shell.\n"
     "\n"
     "Exit Status:\n"
     "The logical negation of the pipeline's status."},
    {"%", "job_spec [&]",
     "Continue a job in the foreground or background.\n"
     "\n"
     "A job specification alone, such as %1 or %vim, works like fg with\n"
     "that job; followed by &, it works like bg. %% and %+ name the\n"
     "current job, %- the previous one.\n"
     "\n"
     "Exit Status:\n"
     "The status of the resumed job in the foreground, else 0."},
    {"(( ... ))", "(( expression ))",
     "Evaluate an arithmetic expression.\n"
     "\n"
     "The expression is evaluated by the rules of arithmetic expansion,\n"
     "the same as let \"expression\".\n"
     "\n"
     "Exit Status:\n"
     "0 if the expression is non-zero, else 1."},
    {".", ". [-p path] filename [arguments]",
     "Read and run commands from a file in the current shell.\n"
     "\n"
     "A FILENAME without a slash is looked for in PATH, or in PATH when\n"
     "-p gives one, and then (outside POSIX mode, and without -p) in the\n"
     "working directory. ARGUMENTS become the positional parameters\n"
     "while the file runs; otherwise it sees the current ones.\n"
     "\n"
     "Options:\n"
     "  -p PATH\tsearch the directories in PATH, a colon-separated\n"
     "\t\tlist, rather than $PATH\n"
     "\n"
     "Exit Status:\n"
     "The status of the last command in the file, or 1 if it cannot be\n"
     "read."},
    {":", ":",
     "Do nothing.\n"
     "\n"
     "Arguments are expanded, and redirections performed, but nothing\n"
     "else happens.\n"
     "\n"
     "Exit Status:\n"
     "Always 0."},
    {"[", "[ arg... ]",
     "Evaluate a conditional expression.\n"
     "\n"
     "The same as test, except that the last argument must be a ] that\n"
     "closes the [.\n"
     "\n"
     "Exit Status:\n"
     "As test."},
    {"[[ ... ]]", "[[ expression ]]",
     "Evaluate a conditional expression, without word splitting.\n"
     "\n"
     "The primaries of test are available, combined with !, &&, || and\n"
     "parentheses. Words are not split and pathnames not expanded. The\n"
     "right side of == and != is a pattern unless quoted, and =~ matches\n"
     "an extended regular expression, leaving the matches in BASH_REMATCH.\n"
     "< and > compare strings in the current locale.\n"
     "\n"
     "Exit Status:\n"
     "0 if the expression is true, 1 if false, 2 on a syntax error."},
    {"alias", "alias [-p] [name[=value] ... ]",
     "Define aliases or show them.\n"
     "\n"
     "Each NAME=VALUE defines NAME as an alias for VALUE; each NAME alone\n"
     "prints that alias. With no arguments, or with -p, every alias is\n"
     "printed in a form that can be read back in. A value ending in a\n"
     "blank makes the word after the alias subject to alias expansion.\n"
     "\n"
     "Options:\n"
     "  -p\tprint all aliases in a reusable form\n"
     "\n"
     "Exit Status:\n"
     "0, unless some NAME has no alias."},
    {"bg", "bg [job_spec ...]",
     "Resume jobs in the background.\n"
     "\n"
     "Each job continues running as if it had been started with &. With\n"
     "no JOB_SPEC, the current job.\n"
     "\n"
     "Exit Status:\n"
     "0, unless job control is off or a job cannot be found."},
    {"bind", "bind [-lpsvPSVX] [-m keymap] [-f filename] [-q name] [-u name] [-r keyseq] [-x keyseq:shell-command] [keyseq:readline-function or readline-command]",
     "Change or show the line editor's key bindings and variables.\n"
     "\n"
     "An argument in the form of an inputrc line binds a key sequence.\n"
     "\n"
     "Options:\n"
     "  -m KEYMAP\tact on KEYMAP, such as emacs or vi-insert\n"
     "  -l\t\tlist the names of the editing functions\n"
     "  -P, -p\tlist the bindings of each function (-p reusably)\n"
     "  -S, -s\tlist the key sequences bound to macros\n"
     "  -V, -v\tlist the editor's variables\n"
     "  -q NAME\tshow the keys that run NAME\n"
     "  -u NAME\tunbind the keys that run NAME\n"
     "  -r KEYSEQ\tremove the binding of KEYSEQ\n"
     "  -f FILE\tread bindings from FILE\n"
     "  -x KEYSEQ:COMMAND\trun the shell COMMAND on KEYSEQ\n"
     "  -X\t\tlist the -x bindings reusably\n"
     "\n"
     "Exit Status:\n"
     "0, unless an option or binding is invalid."},
    {"break", "break [n]",
     "Leave loops.\n"
     "\n"
     "Ends the innermost enclosing for, while, until or select loop, or\n"
     "the Nth enclosing one.\n"
     "\n"
     "Exit Status:\n"
     "0, unless N is not a positive integer."},
    {"builtin", "builtin [shell-builtin [arg ...]]",
     "Run a builtin, bypassing functions.\n"
     "\n"
     "Runs SHELL-BUILTIN with ARGs even when a function of the same name\n"
     "exists, as a function that wraps a builtin needs.\n"
     "\n"
     "Exit Status:\n"
     "The builtin's status, or 1 if SHELL-BUILTIN is not one."},
    {"caller", "caller [expr]",
     "Show where the current function or sourced file was called from.\n"
     "\n"
     "Without EXPR, prints the line number and file of the current call.\n"
     "With EXPR, prints the line, function name and file of the call\n"
     "EXPR frames up the stack, 0 being the current one: a debugger's\n"
     "stack trace.\n"
     "\n"
     "Exit Status:\n"
     "0, unless the shell is not in a function or EXPR is out of range."},
    {"case", "case WORD in [PATTERN [| PATTERN]...) COMMANDS ;;]... esac",
     "Run commands chosen by pattern matching.\n"
     "\n"
     "WORD is matched against each PATTERN in turn, and the COMMANDS of\n"
     "the first match run. A clause ended by ;& goes on into the next\n"
     "clause's commands; one ended by ;;& tests the following patterns.\n"
     "\n"
     "Exit Status:\n"
     "The status of the last command run, or 0 if no pattern matched."},
    {"cd", "cd [-L|[-P [-e]]] [-@] [dir]",
     "Change the working directory.\n"
     "\n"
     "DIR defaults to $HOME, and - means $OLDPWD. A relative DIR is\n"
     "looked for in the directories of CDPATH (a colon-separated list)\n"
     "before the working directory; a DIR found there is printed. With\n"
     "cdable_vars set, a DIR that is not a directory may name a variable\n"
     "that holds one. PWD and OLDPWD are updated.\n"
     "\n"
     "Options:\n"
     "  -L\tfollow symbolic links, resolving .. textually (the default)\n"
     "  -P\tuse the physical directory structure\n"
     "  -e\twith -P, fail if the new directory cannot be determined\n"
     "  -@\taccepted for compatibility\n"
     "\n"
     "Exit Status:\n"
     "0 if the directory changed, else non-zero."},
    {"command", "command [-pVv] command [arg ...]",
     "Run a command, bypassing functions, or describe it.\n"
     "\n"
     "Runs COMMAND with ARGs as a builtin or a program even if a function\n"
     "has its name. A special builtin run this way loses its special\n"
     "properties.\n"
     "\n"
     "Options:\n"
     "  -p\tsearch a default PATH that finds the standard utilities\n"
     "  -v\tprint the word or path that would run COMMAND\n"
     "  -V\tdescribe COMMAND in words\n"
     "\n"
     "Exit Status:\n"
     "COMMAND's status, or 127 if it cannot be found."},
    {"compgen", "compgen [-V varname] [-abcdefgjksuv] [-o option] [-A action] [-G globpat] [-W wordlist] [-F function] [-C command] [-X filterpat] [-P prefix] [-S suffix] [word]",
     "Generate completions.\n"
     "\n"
     "Prints the completions of WORD that the options select, one per\n"
     "line, or stores them in the array VARNAME with -V. The options are\n"
     "those of complete, apart from -p and -r.\n"
     "\n"
     "Exit Status:\n"
     "0, unless an option is invalid or nothing matched."},
    {"complete", "complete [-abcdefgjksuv] [-pr] [-DEI] [-o option] [-A action] [-G globpat] [-W wordlist] [-F function] [-C command] [-X filterpat] [-P prefix] [-S suffix] [name ...]",
     "Say how the arguments of commands are completed.\n"
     "\n"
     "Each NAME gets the completion the options describe. With no\n"
     "options, or -p, the specifications are printed reusably.\n"
     "\n"
     "Options:\n"
     "  -p\tprint specifications\n"
     "  -r\tremove specifications (all, without NAMEs)\n"
     "  -D\tthe default, for commands with no specification\n"
     "  -E\tfor an empty line\n"
     "  -I\tfor the first word of a line\n"
     "\n"
     "The action letters (-a aliases, -b builtins, -c commands, -d\n"
     "directories, -e exported variables, -f files, -g groups, -j jobs,\n"
     "-k keywords, -s services, -u users, -v variables) and -o, -A, -G,\n"
     "-W, -F, -C, -X, -P and -S choose and filter the candidates.\n"
     "\n"
     "Exit Status:\n"
     "0, unless an option is invalid."},
    {"compopt", "compopt [-o|+o option] [-DEI] [name ...]",
     "Change completion options.\n"
     "\n"
     "Turns completion OPTIONs on (-o) or off (+o) for each NAME, or for\n"
     "the completion running when there is no NAME; with no options,\n"
     "prints them. -D, -E and -I act as in complete.\n"
     "\n"
     "Exit Status:\n"
     "0, unless an option is invalid or NAME has no specification."},
    {"continue", "continue [n]",
     "Go on to the next iteration of a loop.\n"
     "\n"
     "Skips the rest of the body of the innermost enclosing for, while,\n"
     "until or select loop, or of the Nth enclosing one.\n"
     "\n"
     "Exit Status:\n"
     "0, unless N is not a positive integer."},
    {"coproc", "coproc [NAME] command [redirections]",
     "Start a command as a coprocess.\n"
     "\n"
     "COMMAND runs asynchronously with pipes to and from the shell. The\n"
     "array NAME (default COPROC) holds the descriptors: NAME[0] reads\n"
     "its output and NAME[1] writes to its input. NAME_PID holds its\n"
     "process ID.\n"
     "\n"
     "Exit Status:\n"
     "0."},
    {"declare", "declare [-aAfFgiIlnrtux] [name[=value] ...] or declare -p [-aAfFilnrtux] [name ...]",
     "Declare variables and give them attributes.\n"
     "\n"
     "Sets each NAME's attributes, and its VALUE if given. With no NAMEs,\n"
     "lists the variables having the attributes named. In a function,\n"
     "the variables are local, as with local, unless -g is given.\n"
     "\n"
     "Options:\n"
     "  -f\trestrict to functions (with -p or no NAMEs, list them)\n"
     "  -F\tlist function names only (with extdebug, line and file too)\n"
     "  -g\tcreate global variables inside a function\n"
     "  -I\ta new local variable takes the attributes and value of the\n"
     "\tvariable it hides\n"
     "  -p\tprint the attributes and values of each NAME reusably\n"
     "\n"
     "Attributes (+ instead of - turns one off):\n"
     "  -a\tindexed array\n"
     "  -A\tassociative array\n"
     "  -i\tinteger: assignments are evaluated arithmetically\n"
     "  -l\tlowercase on assignment\n"
     "  -n\tname reference to the variable its value names\n"
     "  -r\treadonly\n"
     "  -t\ttrace: functions inherit the DEBUG and RETURN traps\n"
     "  -u\tuppercase on assignment\n"
     "  -x\texport to the environment of commands\n"
     "\n"
     "Exit Status:\n"
     "0, unless an option is invalid or an assignment fails."},
    {"dirs", "dirs [-clpv] [+N] [-N]",
     "Show the directory stack.\n"
     "\n"
     "The stack holds the working directory followed by the directories\n"
     "pushd saved. Entries under $HOME are shown with ~.\n"
     "\n"
     "Options:\n"
     "  -c\tclear the stack\n"
     "  -l\tshow full paths, without ~\n"
     "  -p\tone entry per line\n"
     "  -v\tone entry per line, numbered\n"
     "\n"
     "Arguments:\n"
     "  +N\tshow the Nth entry, counting from 0 at the left\n"
     "  -N\tshow the Nth entry, counting from 0 at the right\n"
     "\n"
     "Exit Status:\n"
     "0, unless an option is invalid or N is out of range."},
    {"disown", "disown [-h] [-ar] [jobspec ... | pid ...]",
     "Remove jobs from the job table.\n"
     "\n"
     "Each JOBSPEC (default: the current job) is forgotten, so the shell\n"
     "no longer reports or waits for it.\n"
     "\n"
     "Options:\n"
     "  -a\tall jobs\n"
     "  -h\tkeep the job, but do not send it SIGHUP when the shell gets one\n"
     "  -r\tonly running jobs\n"
     "\n"
     "Exit Status:\n"
     "0, unless an option or JOBSPEC is invalid."},
    {"echo", "echo [-neE] [arg ...]",
     "Write arguments to standard output.\n"
     "\n"
     "The ARGs are written separated by spaces and followed by a newline.\n"
     "\n"
     "Options:\n"
     "  -n\tno trailing newline\n"
     "  -e\tinterpret backslash escapes\n"
     "  -E\tdo not interpret them (the default, unless xpg_echo is set)\n"
     "\n"
     "The escapes are \\a, \\b, \\c (end the output here), \\e, \\E, \\f,\n"
     "\\n, \\r, \\t, \\v, \\\\, \\0NNN (octal), \\xHH (hexadecimal), and\n"
     "\\uHHHH and \\UHHHHHHHH (Unicode characters).\n"
     "\n"
     "Exit Status:\n"
     "0, unless writing fails."},
    {"enable", "enable [-a] [-dnps] [-f filename] [name ...]",
     "Enable or disable builtins.\n"
     "\n"
     "A disabled builtin is not found as a command, so a program of the\n"
     "same name runs instead. With no NAMEs, or with -p, the builtins are\n"
     "listed.\n"
     "\n"
     "Options:\n"
     "  -a\tlist all builtins, enabled or not\n"
     "  -n\tdisable each NAME, or list the disabled builtins\n"
     "  -p\tlist reusably\n"
     "  -s\tlist only the POSIX special builtins\n"
     "  -f\tload a builtin from a shared object (not available)\n"
     "  -d\tremove a loaded builtin\n"
     "\n"
     "Exit Status:\n"
     "0, unless a NAME is not a builtin or an error occurs."},
    {"eval", "eval [arg ...]",
     "Run arguments as shell commands.\n"
     "\n"
     "The ARGs are joined with spaces, and the result is parsed and run\n"
     "in the current shell.\n"
     "\n"
     "Exit Status:\n"
     "The status of the commands, or 0 if there are none."},
    {"exec", "exec [-cl] [-a name] [command [argument ...]] [redirection ...]",
     "Replace the shell with a program, or redirect the shell itself.\n"
     "\n"
     "With COMMAND, the program replaces the shell. Without one, the\n"
     "redirections apply to the shell from here on.\n"
     "\n"
     "Options:\n"
     "  -a NAME\tpass NAME as the program's zeroth argument\n"
     "  -c\t\tclear the environment\n"
     "  -l\t\tprefix the zeroth argument with a dash, as for a login\n"
     "\n"
     "A non-interactive shell exits when COMMAND cannot be run, unless\n"
     "execfail is set.\n"
     "\n"
     "Exit Status:\n"
     "0 without COMMAND, unless a redirection fails."},
    {"exit", "exit [n]",
     "Exit the shell.\n"
     "\n"
     "The status is N, or that of the last command. The EXIT trap runs\n"
     "first.\n"
     "\n"
     "Exit Status:\n"
     "N, or the last command's."},
    {"export", "export [-fn] [name[=value] ...] or export -p [-f]",
     "Mark variables for the environment of commands.\n"
     "\n"
     "Each NAME, assigned VALUE if given, is passed to the commands the\n"
     "shell runs from now on. With no NAMEs, or with -p, the exported\n"
     "names are listed reusably.\n"
     "\n"
     "Options:\n"
     "  -f\tthe NAMEs are functions\n"
     "  -n\tstop exporting each NAME\n"
     "  -p\tlist exported variables (with -f, functions)\n"
     "\n"
     "Exit Status:\n"
     "0, unless an option or NAME is invalid."},
    {"false", "false",
     "Fail.\n"
     "\n"
     "Exit Status:\n"
     "Always 1."},
    {"fc", "fc [-e ename] [-lnr] [first] [last] or fc -s [pat=rep] [command]",
     "List or edit and rerun commands from the history.\n"
     "\n"
     "FIRST and LAST choose a range of history entries, by number (a\n"
     "negative number counts back from the current command) or by the\n"
     "start of a command. The range goes to an editor, and what it saves\n"
     "is run.\n"
     "\n"
     "Options:\n"
     "  -e ENAME\tthe editor (default: $FCEDIT, $EDITOR, then vi)\n"
     "  -l\t\tlist the commands instead\n"
     "  -n\t\twithout numbers, when listing\n"
     "  -r\t\tin reverse order\n"
     "\n"
     "With -s, COMMAND (default: the last one) is run again at once,\n"
     "after replacing PAT with REP.\n"
     "\n"
     "Exit Status:\n"
     "That of the commands run, or 0 when listing, unless an error occurs."},
    {"fg", "fg [job_spec]",
     "Bring a job to the foreground.\n"
     "\n"
     "The job (default: the current one) becomes the current job and the\n"
     "shell waits for it.\n"
     "\n"
     "Exit Status:\n"
     "The job's status, or non-zero if it cannot be resumed."},
    {"for", "for NAME [in WORDS ... ] ; do COMMANDS; done",
     "Run commands for each word of a list.\n"
     "\n"
     "NAME is set to each of the expanded WORDS in turn, and COMMANDS run\n"
     "for each. Without in WORDS, the positional parameters are used.\n"
     "\n"
     "Exit Status:\n"
     "The status of the last command run, or 0."},
    {"for ((", "for (( exp1; exp2; exp3 )); do COMMANDS; done",
     "Run commands in an arithmetic loop.\n"
     "\n"
     "EXP1 is evaluated once; then, as long as EXP2 is non-zero, COMMANDS\n"
     "run and EXP3 is evaluated. A missing expression counts as 1.\n"
     "\n"
     "Exit Status:\n"
     "The status of the last command run, or 0."},
    {"function", "function name { COMMANDS ; } or name () { COMMANDS ; }",
     "Define a function.\n"
     "\n"
     "NAME becomes a command that runs COMMANDS, with its arguments as\n"
     "the positional parameters and $FUNCNAME set to NAME.\n"
     "\n"
     "Exit Status:\n"
     "0, unless NAME is readonly or invalid."},
    {"getopts", "getopts optstring name [arg ...]",
     "Parse options from the positional parameters.\n"
     "\n"
     "Each call puts the next option letter in NAME and the index of the\n"
     "next argument to look at in OPTIND (1 at the start). OPTSTRING lists\n"
     "the letters; one followed by a colon takes an argument, which goes\n"
     "in OPTARG. An unknown letter sets NAME to ?, and a missing argument\n"
     "too; with a leading colon in OPTSTRING (or OPTERR=0), no message is\n"
     "printed, and OPTARG holds the letter, NAME being : for a missing\n"
     "argument. ARGs, if given, are parsed instead of the positional\n"
     "parameters.\n"
     "\n"
     "Exit Status:\n"
     "0 when an option was found, 1 at the end of the options."},
    {"hash", "hash [-lr] [-p pathname] [-dt] [name ...]",
     "Remember or show the full paths of commands.\n"
     "\n"
     "Each NAME is looked up in PATH and remembered. With no NAMEs, the\n"
     "remembered commands are listed.\n"
     "\n"
     "Options:\n"
     "  -d\t\tforget each NAME\n"
     "  -l\t\tlist reusably\n"
     "  -p PATHNAME\tremember PATHNAME as the path of NAME\n"
     "  -r\t\tforget everything\n"
     "  -t\t\tprint the path remembered for each NAME\n"
     "\n"
     "Exit Status:\n"
     "0, unless a NAME is not found or an option is invalid."},
    {"help", "help [-dms] [pattern ...]",
     "Describe builtin commands.\n"
     "\n"
     "Prints the description of each topic that PATTERN matches (by\n"
     "pathname pattern, or failing that by prefix); with no PATTERN, a\n"
     "list of all topics.\n"
     "\n"
     "Options:\n"
     "  -d\ta one-line summary of each topic\n"
     "  -m\tin the form of a manual page\n"
     "  -s\tthe synopsis only\n"
     "\n"
     "Exit Status:\n"
     "0, unless nothing matched or an option is invalid."},
    {"history", "history [-c] [-d offset] [n] or history -anrw [filename] or history -ps arg [arg...]",
     "Show or change the command history.\n"
     "\n"
     "With no options, the history is listed with entry numbers, or its\n"
     "last N entries. With HISTTIMEFORMAT set, each entry shows its time.\n"
     "\n"
     "Options:\n"
     "  -c\t\tclear the history\n"
     "  -d OFFSET\tdelete an entry, or a range START-END (negative\n"
     "\t\toffsets count from the end)\n"
     "  -a\t\tappend this session's new entries to the history file\n"
     "  -n\t\tread the entries of the file not read yet\n"
     "  -r\t\tread the file into the history\n"
     "  -w\t\twrite the history to the file\n"
     "  -p\t\thistory-expand each ARG and print it, without saving\n"
     "  -s\t\tadd the ARGs as one entry\n"
     "\n"
     "FILENAME defaults to $HISTFILE.\n"
     "\n"
     "Exit Status:\n"
     "0, unless an option or offset is invalid or an error occurs."},
    {"if", "if COMMANDS; then COMMANDS; [ elif COMMANDS; then COMMANDS; ]... [ else COMMANDS; ] fi",
     "Run commands depending on conditions.\n"
     "\n"
     "The COMMANDS after if run, and when their status is 0 the then\n"
     "part runs. Otherwise each elif is tried the same way, and if none\n"
     "succeeds, the else part runs.\n"
     "\n"
     "Exit Status:\n"
     "The status of the last command run, or 0 if no part ran."},
    {"jobs", "jobs [-lnprs] [jobspec ...] or jobs -x command [args]",
     "List jobs.\n"
     "\n"
     "Shows each JOBSPEC, or every job, with its number and state.\n"
     "\n"
     "Options:\n"
     "  -l\twith process IDs\n"
     "  -n\tonly jobs whose state changed since last reported\n"
     "  -p\tprocess IDs only\n"
     "  -r\tonly running jobs\n"
     "  -s\tonly stopped jobs\n"
     "\n"
     "With -x, COMMAND runs with each job specification among its ARGS\n"
     "replaced by the job's process group ID.\n"
     "\n"
     "Exit Status:\n"
     "0, unless an option or JOBSPEC is invalid."},
    {"kill", "kill [-s sigspec | -n signum | -sigspec] pid | jobspec ... or kill -l [sigspec]",
     "Send a signal to processes or jobs.\n"
     "\n"
     "Sends SIGTERM, or the signal named, to each PID or JOBSPEC.\n"
     "\n"
     "Options:\n"
     "  -s SIG\tthe signal, by name\n"
     "  -n NUM\tthe signal, by number\n"
     "  -l\tlist signal names, or translate between names and numbers\n"
     "  -L\tthe same as -l\n"
     "\n"
     "Exit Status:\n"
     "0, unless an argument is invalid or a signal cannot be sent."},
    {"let", "let arg [arg ...]",
     "Evaluate arithmetic expressions.\n"
     "\n"
     "Each ARG is an expression, in fixed-width integers with no check\n"
     "for overflow, using the operators of C, from highest precedence:\n"
     "\n"
     "\tid++ id--\t\tpostfix increment and decrement\n"
     "\t++id --id\t\tprefix increment and decrement\n"
     "\t- +\t\t\tnegation and identity\n"
     "\t! ~\t\t\tlogical and bitwise not\n"
     "\t**\t\t\texponentiation\n"
     "\t* / %\t\t\tmultiplication, division, remainder\n"
     "\t+ -\t\t\taddition, subtraction\n"
     "\t<< >>\t\t\tshifts\n"
     "\t<= >= < >\t\tcomparisons\n"
     "\t== !=\t\t\tequality\n"
     "\t&\t\t\tbitwise and\n"
     "\t^\t\t\tbitwise exclusive or\n"
     "\t|\t\t\tbitwise or\n"
     "\t&&\t\t\tlogical and\n"
     "\t||\t\t\tlogical or\n"
     "\texpr ? expr : expr\tconditional\n"
     "\t= *= /= %= += -= <<= >>= &= ^= |=\tassignment\n"
     "\t,\t\t\tsequence\n"
     "\n"
     "Variables need no $; their values are evaluated as expressions.\n"
     "Parentheses group. Numbers may be written as 0x1f, 017 (octal) or\n"
     "BASE#DIGITS with a base from 2 to 64.\n"
     "\n"
     "Exit Status:\n"
     "1 if the last ARG evaluates to 0, else 0."},
    {"local", "local [option] name[=value] ...",
     "Create variables local to a function.\n"
     "\n"
     "Each NAME is visible in the function and those it calls, and its\n"
     "previous value returns when the function does. The options are\n"
     "those of declare. local - saves the shell options, restoring them\n"
     "when the function returns.\n"
     "\n"
     "Exit Status:\n"
     "0, unless used outside a function, an option is invalid, or an\n"
     "assignment fails."},
    {"logout", "logout [n]",
     "Exit a login shell.\n"
     "\n"
     "As exit, but only in a login shell.\n"
     "\n"
     "Exit Status:\n"
     "N, or the last command's."},
    {"mapfile", "mapfile [-d delim] [-n count] [-O origin] [-s count] [-t] [-u fd] [-C callback] [-c quantum] [array]",
     "Read lines into an indexed array.\n"
     "\n"
     "Lines from standard input, or from descriptor FD with -u, become\n"
     "the elements of ARRAY (default MAPFILE).\n"
     "\n"
     "Options:\n"
     "  -d DELIM\tend lines with DELIM rather than newline\n"
     "  -n COUNT\tread at most COUNT lines (0: all)\n"
     "  -O ORIGIN\tstore from index ORIGIN (default 0)\n"
     "  -s COUNT\tskip the first COUNT lines\n"
     "  -t\t\tremove the delimiter from each line\n"
     "  -u FD\t\tread from descriptor FD\n"
     "  -C CALLBACK\trun CALLBACK every QUANTUM lines, with the index\n"
     "\t\tand line as arguments\n"
     "  -c QUANTUM\tlines between callbacks (default 5000)\n"
     "\n"
     "Without -O, ARRAY is emptied first.\n"
     "\n"
     "Exit Status:\n"
     "0, unless an option is invalid or ARRAY cannot be assigned."},
    {"popd", "popd [-n] [+N | -N]",
     "Remove a directory from the directory stack.\n"
     "\n"
     "With no argument, the top entry goes, and the shell changes to the\n"
     "new top. Then the stack is shown, as dirs shows it.\n"
     "\n"
     "Options:\n"
     "  -n\tchange only the stack, not the working directory\n"
     "\n"
     "Arguments:\n"
     "  +N\tremove the Nth entry counting from 0 at the left\n"
     "  -N\tremove the Nth entry counting from 0 at the right\n"
     "\n"
     "Exit Status:\n"
     "0, unless an argument is invalid or the directory change fails."},
    {"printf", "printf [-v var] format [arguments]",
     "Format and print arguments.\n"
     "\n"
     "FORMAT is printed with its escapes interpreted and each conversion\n"
     "replaced by the next ARGUMENT, as C's printf does; FORMAT is used\n"
     "again while ARGUMENTS remain. Beyond C's conversions, %b expands\n"
     "backslash escapes in its argument, %q quotes it for reuse as\n"
     "shell input, %Q does so after applying the precision, and\n"
     "%(FORMAT)T formats a time (seconds since the epoch; -1 is now, -2\n"
     "is when the shell started) with strftime.\n"
     "\n"
     "Options:\n"
     "  -v VAR\tassign the output to VAR instead\n"
     "\n"
     "Exit Status:\n"
     "0, unless an option is invalid or a write or assignment fails."},
    {"pushd", "pushd [-n] [+N | -N | dir]",
     "Add a directory to the directory stack, or rotate it.\n"
     "\n"
     "DIR is pushed and becomes the working directory. With no argument,\n"
     "the top two entries trade places. Then the stack is shown, as dirs\n"
     "shows it.\n"
     "\n"
     "Options:\n"
     "  -n\tchange only the stack, not the working directory\n"
     "\n"
     "Arguments:\n"
     "  +N\trotate the Nth entry, counting from 0 at the left, to the top\n"
     "  -N\trotate the Nth entry, counting from 0 at the right, to the top\n"
     "  dir\tpush DIR and change to it\n"
     "\n"
     "Exit Status:\n"
     "0, unless an argument is invalid or the directory change fails."},
    {"pwd", "pwd [-LP]",
     "Print the working directory.\n"
     "\n"
     "Options:\n"
     "  -L\tas $PWD holds it, with any symbolic links (the default)\n"
     "  -P\twith symbolic links resolved\n"
     "\n"
     "Exit Status:\n"
     "0, unless an option is invalid or the directory cannot be read."},
    {"read", "read [-Eers] [-a array] [-d delim] [-i text] [-n nchars] [-N nchars] [-p prompt] [-t timeout] [-u fd] [name ...]",
     "Read a line and split it into variables.\n"
     "\n"
     "A line from standard input is split into words as IFS directs, and\n"
     "each NAME gets a word, the last one the rest of the line. Without\n"
     "NAMEs, the line goes to REPLY. Backslashes escape the next\n"
     "character, including a newline, unless -r is given.\n"
     "\n"
     "Options:\n"
     "  -a ARRAY\tstore the words in ARRAY, from index 0\n"
     "  -d DELIM\tread up to DELIM rather than a newline\n"
     "  -e\t\tuse the line editor when reading from a terminal\n"
     "  -E\t\tthe same, with the shell's own completion\n"
     "  -i TEXT\tstart the editor's line with TEXT\n"
     "  -n NCHARS\treturn after NCHARS characters, or the delimiter\n"
     "  -N NCHARS\treturn after exactly NCHARS characters, delimiters\n"
     "\t\tincluded, splitting nothing\n"
     "  -p PROMPT\tprint PROMPT on a terminal first\n"
     "  -r\t\tbackslashes are ordinary characters\n"
     "  -s\t\tdo not echo terminal input\n"
     "  -t TIMEOUT\tgive up after TIMEOUT seconds (0: see whether input\n"
     "\t\tis ready); the default comes from TMOUT\n"
     "  -u FD\t\tread from descriptor FD\n"
     "\n"
     "Exit Status:\n"
     "0, unless input ended, the time ran out (above 128), an assignment\n"
     "failed, or an option is invalid."},
    {"readarray", "readarray [-d delim] [-n count] [-O origin] [-s count] [-t] [-u fd] [-C callback] [-c quantum] [array]",
     "Read lines into an indexed array.\n"
     "\n"
     "Another name for mapfile."},
    {"readonly", "readonly [-aAf] [name[=value] ...] or readonly -p",
     "Make variables unchangeable.\n"
     "\n"
     "Each NAME, assigned VALUE if given, can no longer be assigned or\n"
     "unset. With no NAMEs, or with -p, the readonly names are listed.\n"
     "\n"
     "Options:\n"
     "  -a\tindexed arrays\n"
     "  -A\tassociative arrays\n"
     "  -f\tfunctions\n"
     "  -p\tlist reusably\n"
     "\n"
     "Exit Status:\n"
     "0, unless an option or NAME is invalid."},
    {"return", "return [n]",
     "Return from a function or sourced file.\n"
     "\n"
     "The status is N, or that of the last command.\n"
     "\n"
     "Exit Status:\n"
     "N, or the last command's; 1 outside a function or sourced file."},
    {"select", "select NAME [in WORDS ... ;] do COMMANDS; done",
     "Run commands on a word picked from a menu.\n"
     "\n"
     "The expanded WORDS (default: the positional parameters) are listed\n"
     "on standard error, numbered, and the PS3 prompt is shown. A line\n"
     "read with a listed number sets NAME to that word, anything else\n"
     "sets it to null; the line itself goes in REPLY. COMMANDS run after\n"
     "each line, until break; end of input ends the loop too.\n"
     "\n"
     "Exit Status:\n"
     "The status of the last command run."},
    {"set", "set [-abefhkmnptuvxBCEHPT] [-o option-name] [--] [-] [arg ...]",
     "Set shell options and positional parameters.\n"
     "\n"
     "Without arguments, the variables and functions are listed. -X turns\n"
     "an option on, +X turns it off. Arguments after the options become\n"
     "the positional parameters; -- makes even none of them do so, and -\n"
     "ends the options and turns off -x and -v.\n"
     "\n"
     "Options:\n"
     "  -a  allexport\texport variables when they are assigned\n"
     "  -b  notify\treport finished background jobs at once\n"
     "  -e  errexit\texit when a command fails\n"
     "  -f  noglob\tno pathname expansion\n"
     "  -h  hashall\tremember command paths\n"
     "  -k  keyword\tassignments anywhere in a command go to its environment\n"
     "  -m  monitor\tjob control\n"
     "  -n  noexec\tread commands without running them\n"
     "  -o NAME\tturn on the option NAME; with no NAME, list the options\n"
     "  -p  privileged\tignore startup files from the environment\n"
     "  -t  onecmd\texit after one command\n"
     "  -u  nounset\texpanding an unset variable is an error\n"
     "  -v  verbose\techo input lines as they are read\n"
     "  -x  xtrace\techo commands as they run\n"
     "  -B  braceexpand\tbrace expansion\n"
     "  -C  noclobber\t> does not overwrite files\n"
     "  -E  errtrace\tfunctions inherit the ERR trap\n"
     "  -H  histexpand\t! history expansion\n"
     "  -P  physical\tresolve symbolic links in cd and pwd\n"
     "  -T  functrace\tfunctions inherit the DEBUG and RETURN traps\n"
     "\n"
     "Long names alone: emacs, vi (editing modes), history, ignoreeof,\n"
     "interactive-comments, nolog, pipefail (a pipeline fails with its\n"
     "last failing command), posix.\n"
     "\n"
     "Exit Status:\n"
     "0, unless an option is invalid."},
    {"shift", "shift [n]",
     "Shift the positional parameters.\n"
     "\n"
     "Drops the first N (default 1) positional parameters, renumbering\n"
     "the rest from $1.\n"
     "\n"
     "Exit Status:\n"
     "0, unless N is negative or more than $#."},
    {"shopt", "shopt [-pqsu] [-o] [optname ...]",
     "Set or show shell options.\n"
     "\n"
     "With no options, shows each OPTNAME, or all of them, with its\n"
     "setting.\n"
     "\n"
     "Options:\n"
     "  -o\tthe OPTNAMEs are those of set -o\n"
     "  -p\tlist reusably\n"
     "  -q\tprint nothing; the status tells whether all are on\n"
     "  -s\tturn each OPTNAME on (with none, list those on)\n"
     "  -u\tturn each OPTNAME off (with none, list those off)\n"
     "\n"
     "Exit Status:\n"
     "0 if every OPTNAME is on (or was set), else 1; 2 for an invalid\n"
     "option."},
    {"source", "source [-p path] filename [arguments]",
     "Read and run commands from a file in the current shell.\n"
     "\n"
     "Another name for the . command, which see."},
    {"suspend", "suspend [-f]",
     "Stop the shell until it gets SIGCONT.\n"
     "\n"
     "Options:\n"
     "  -f\tstop even a login shell, or without job control\n"
     "\n"
     "Exit Status:\n"
     "0, unless job control is off or an error occurs."},
    {"test", "test [expr]",
     "Evaluate a conditional expression.\n"
     "\n"
     "File primaries (true if FILE exists and):\n"
     "  -a FILE, -e FILE\texists\n"
     "  -b FILE\t\tis a block device\n"
     "  -c FILE\t\tis a character device\n"
     "  -d FILE\t\tis a directory\n"
     "  -f FILE\t\tis a regular file\n"
     "  -g FILE\t\thas its set-group-ID bit\n"
     "  -h FILE, -L FILE\tis a symbolic link\n"
     "  -k FILE\t\thas its sticky bit\n"
     "  -p FILE\t\tis a named pipe\n"
     "  -r FILE\t\tis readable by you\n"
     "  -s FILE\t\tis not empty\n"
     "  -S FILE\t\tis a socket\n"
     "  -t FD\t\t\tdescriptor FD is open on a terminal\n"
     "  -u FILE\t\thas its set-user-ID bit\n"
     "  -w FILE\t\tis writable by you\n"
     "  -x FILE\t\tis executable by you\n"
     "  -O FILE\t\tis owned by you\n"
     "  -G FILE\t\tis owned by your group\n"
     "  -N FILE\t\twas modified since it was last read\n"
     "  F1 -nt F2\t\tF1 is newer than F2, or F2 does not exist\n"
     "  F1 -ot F2\t\tF1 is older than F2, or F1 does not exist\n"
     "  F1 -ef F2\t\tF1 and F2 are the same file\n"
     "\n"
     "String primaries:\n"
     "  -z S, -n S\tS is empty, not empty\n"
     "  S\t\tS is not empty\n"
     "  S1 = S2\tthe strings are equal (also ==)\n"
     "  S1 != S2\tthey differ\n"
     "  S1 < S2\tS1 sorts before S2\n"
     "  S1 > S2\tS1 sorts after S2\n"
     "\n"
     "Other primaries:\n"
     "  -o OPT\tthe shell option OPT is on\n"
     "  -v VAR\tthe variable VAR is set\n"
     "  -R VAR\tVAR is a name reference\n"
     "  N1 -eq N2\tinteger comparisons, also -ne, -lt, -le, -gt, -ge\n"
     "  ! EXPR\tEXPR is false\n"
     "  E1 -a E2\tboth are true\n"
     "  E1 -o E2\teither is true\n"
     "  ( EXPR )\tgrouping\n"
     "\n"
     "How the arguments are read depends on how many there are, as POSIX\n"
     "specifies.\n"
     "\n"
     "Exit Status:\n"
     "0 if EXPR is true, 1 if false or absent, 2 if it is invalid."},
    {"time", "time [-p] pipeline",
     "Time a pipeline.\n"
     "\n"
     "After PIPELINE ends, the elapsed, user and system times it took are\n"
     "printed on standard error, as TIMEFORMAT directs.\n"
     "\n"
     "Options:\n"
     "  -p\tthe POSIX format\n"
     "\n"
     "Exit Status:\n"
     "PIPELINE's status."},
    {"times", "times",
     "Show the CPU times used.\n"
     "\n"
     "Prints the user and system times of the shell, then of the\n"
     "programs it has waited for.\n"
     "\n"
     "Exit Status:\n"
     "Always 0."},
    {"trap", "trap [-Plp] [[action] signal_spec ...]",
     "Run commands when signals or other events arrive.\n"
     "\n"
     "ACTION runs whenever the shell receives one of the SIGNAL_SPECs.\n"
     "An empty ACTION ignores the signals; ACTION - (or none, with one\n"
     "signal) restores their defaults. Besides signal names and numbers,\n"
     "EXIT (0) runs on exit, DEBUG before each simple command, RETURN\n"
     "when a function or sourced file returns, and ERR when a command\n"
     "fails where set -e would act.\n"
     "\n"
     "Options:\n"
     "  -l\tlist the signal names and numbers\n"
     "  -p\tprint the trap of each SIGNAL_SPEC, or all, reusably\n"
     "  -P\tprint only the actions of each SIGNAL_SPEC\n"
     "\n"
     "Exit Status:\n"
     "0, unless a SIGNAL_SPEC or option is invalid."},
    {"true", "true",
     "Succeed.\n"
     "\n"
     "Exit Status:\n"
     "Always 0."},
    {"type", "type [-afptP] name [name ...]",
     "Describe how each name would be run.\n"
     "\n"
     "Says whether NAME is an alias, keyword, function, builtin or file.\n"
     "\n"
     "Options:\n"
     "  -a\tevery place NAME is found, not just the first\n"
     "  -f\tskip functions\n"
     "  -p\tthe file that would run, if any\n"
     "  -P\tsearch PATH for a file even if NAME is something else\n"
     "  -t\tone word: alias, keyword, function, builtin or file\n"
     "\n"
     "Exit Status:\n"
     "0 if every NAME was found, else 1."},
    {"typeset", "typeset [-aAfFgiIlnrtux] name[=value] ... or typeset -p [-aAfFilnrtux] [name ...]",
     "Declare variables and give them attributes.\n"
     "\n"
     "Another name for declare."},
    {"ulimit", "ulimit [-SHabcdefiklmnpqrstuvxPRT] [limit]",
     "Show or set resource limits.\n"
     "\n"
     "Limits apply to the shell and the programs it starts. A resource\n"
     "option without a LIMIT shows the limit; with one, sets it. LIMIT is\n"
     "a number in the units shown, or unlimited, soft or hard. With no\n"
     "resource, -f is meant.\n"
     "\n"
     "Options:\n"
     "  -S\tthe soft limit\n"
     "  -H\tthe hard limit (setting without -S or -H sets both)\n"
     "  -a\tshow every limit\n"
     "  -c\tcore file size\n"
     "  -d\tdata segment size\n"
     "  -e\tscheduling priority\n"
     "  -f\tsize of files written\n"
     "  -i\tpending signals\n"
     "  -l\tlocked memory\n"
     "  -m\tresident set size\n"
     "  -n\topen file descriptors\n"
     "  -p\tpipe buffer size\n"
     "  -q\tPOSIX message queue bytes\n"
     "  -r\treal-time priority\n"
     "  -s\tstack size\n"
     "  -t\tCPU seconds\n"
     "  -u\tprocesses\n"
     "  -v\tvirtual memory\n"
     "  -x\tfile locks\n"
     "  -R\treal-time CPU time before blocking\n"
     "\n"
     "Not every system has every limit.\n"
     "\n"
     "Exit Status:\n"
     "0, unless an option is invalid or a limit cannot be changed."},
    {"umask", "umask [-p] [-S] [mode]",
     "Show or set the file creation mask.\n"
     "\n"
     "MODE is an octal number, or a symbolic mode as chmod takes it that\n"
     "says which permissions files may have.\n"
     "\n"
     "Options:\n"
     "  -p\tshow the mask reusably\n"
     "  -S\tshow it symbolically\n"
     "\n"
     "Exit Status:\n"
     "0, unless MODE or an option is invalid."},
    {"unalias", "unalias [-a] name [name ...]",
     "Remove aliases.\n"
     "\n"
     "Options:\n"
     "  -a\tremove all of them\n"
     "\n"
     "Exit Status:\n"
     "0, unless a NAME is not an alias."},
    {"unset", "unset [-f] [-v] [-n] [name ...]",
     "Remove variables or functions.\n"
     "\n"
     "Without options, each NAME is a variable, or else a function.\n"
     "\n"
     "Options:\n"
     "  -f\tthe NAMEs are functions\n"
     "  -v\tthe NAMEs are variables\n"
     "  -n\tremove a name reference itself, not what it refers to\n"
     "\n"
     "Readonly names cannot be removed.\n"
     "\n"
     "Exit Status:\n"
     "0, unless a NAME is readonly or an option is invalid."},
    {"until", "until COMMANDS; do COMMANDS-2; done",
     "Run commands until a test succeeds.\n"
     "\n"
     "COMMANDS-2 runs for as long as COMMANDS fail.\n"
     "\n"
     "Exit Status:\n"
     "The status of the last command run."},
    {"variables", "variables - Names and meanings of some shell variables",
     "Some variables the shell uses.\n"
     "\n"
     "BASH_VERSION\tthe version of the shell\n"
     "CDPATH\t\tdirectories cd searches, separated by colons\n"
     "GLOBIGNORE\tpatterns of names pathname expansion leaves out\n"
     "HISTFILE\twhere the history is saved\n"
     "HISTFILESIZE\thow many lines the history file keeps\n"
     "HISTSIZE\thow many commands the history keeps\n"
     "HOME\t\tyour home directory\n"
     "HOSTNAME\tthe name of this machine\n"
     "HOSTTYPE\tthe kind of processor\n"
     "IGNOREEOF\thow many end-of-file characters an interactive shell\n"
     "\t\tignores before exiting\n"
     "MACHTYPE\tthe system, as processor-vendor-system\n"
     "MAILCHECK\tseconds between checks for new mail\n"
     "MAILPATH\tfiles to check for new mail\n"
     "OSTYPE\t\tthe operating system\n"
     "PATH\t\tdirectories searched for commands\n"
     "PROMPT_COMMAND\tcommands run before each primary prompt\n"
     "PS1\t\tthe primary prompt\n"
     "PS2\t\tthe prompt for continued lines\n"
     "PWD\t\tthe working directory\n"
     "SHELLOPTS\tthe set -o options that are on\n"
     "TERM\t\tthe kind of terminal\n"
     "TIMEFORMAT\thow time shows its results\n"
     "auto_resume\tlets a command word resume a stopped job of that name\n"
     "histchars\tthe history expansion, quick substitution and comment\n"
     "\t\tcharacters\n"
     "HISTIGNORE\tpatterns of commands not saved in the history\n"
     "HISTCONTROL\tignorespace, ignoredups, ignoreboth or erasedups"},
    {"wait", "wait [-fn] [-p var] [id ...]",
     "Wait for jobs to finish.\n"
     "\n"
     "Waits for each ID, a process ID or job specification, or for all\n"
     "background jobs when there is none.\n"
     "\n"
     "Options:\n"
     "  -n\twait for the next job to finish, among the IDs if any\n"
     "  -p VAR\tput the process ID of that job in VAR\n"
     "  -f\twait for the job to finish, not merely change state\n"
     "\n"
     "Exit Status:\n"
     "The status of the last ID, or 127 if it is unknown; 0 for all jobs."},
    {"while", "while COMMANDS; do COMMANDS-2; done",
     "Run commands while a test succeeds.\n"
     "\n"
     "COMMANDS-2 runs for as long as COMMANDS succeed.\n"
     "\n"
     "Exit Status:\n"
     "The status of the last command run."},
    {"{ ... }", "{ COMMANDS ; }",
     "Group commands.\n"
     "\n"
     "COMMANDS run in the current shell as one unit, for example to\n"
     "redirect them together.\n"
     "\n"
     "Exit Status:\n"
     "The status of the last command."},
};

static HelpTopic const *helptopic(Str name)
{
    for (iz i = 0; i < countof(helptopics); i++) {
        if (helptopics[i].name == name) return helptopics + i;
    }
    return 0;
}

static Str helpsynopsis(Str name)
{
    HelpTopic const *t = helptopic(name);
    return t ? t->synopsis : Str{};
}

// The long text, each line indented four spaces
static void helptext(Buf *b, HelpTopic const *t)
{
    for (Str rest = t->text;;) {
        Cut c = cut(rest, '\n');
        print(b, "    ");
        print(b, c.head);
        putbyte(b, '\n');
        if (!c.ok) break;
        rest = c.tail;
    }
}

// The first line of help, help -m: the shell and its version
static Str helpversion(Shell *sh, Arena *a)
{
    Str mach = arrayelem(sh, a, "BASH_VERSINFO", 5);
    return concat3(a, "unabashed, version 5.3.0(1)-release (", mach, ")");
}

// A topic is disabled when it names a builtin that enable turned off
static b32 helpdisabled(Shell *sh, Str name)
{
    Builtin *b = anybuiltin(sh, name);
    return b && isdisabled(sh, b);
}

// One column of the listing: a star for a disabled builtin, then the
// synopsis, ended by > from cut characters on (whether or not that
// is all of it, as bash's own layout does), padded to padto
static void helpcolumn(Shell *sh, Buf *b, HelpTopic const *t, iz cut, iz padto)
{
    putbyte(b, helpdisabled(sh, t->name) ? '*' : ' ');
    Str s = t->synopsis;
    iz  n = 1;
    if (s.len >= cut) {
        print(b, takehead(s, cut));
        putbyte(b, '>');
        n += cut + 1;
    } else {
        print(b, s);
        n += s.len;
    }
    for (; n < padto; n++) putbyte(b, ' ');
}

static void helplist(Shell *sh, Arena a)
{
    Buf b(&a, 8192);
    print(&b, helpversion(sh, &a));
    print(&b,
        "\n"
        "These are the shell's builtin commands and keywords. Type `help'\n"
        "for this list, and `help NAME' to read about NAME; patterns work\n"
        "too, as in `help \"read*\"'. The shell's manual page covers the\n"
        "rest. `man -k' and `info' find other commands.\n"
        "\n"
        "A star (*) marks a builtin that enable has turned off.\n"
        "\n");
    Got cols  = getvar(sh, &a, "COLUMNS");
    i64 ncols = 0;
    if (!cols.set || !parsei64(cols.value, &ncols) || ncols<=0) ncols = 80;
    iz width = (iz)(ncols/2 < 128 ? ncols/2 : 128);
    width = width>3 ? width : 40;
    iz n      = countof(helptopics);
    iz height = (n + 1)/2;
    for (iz i = 0; i < height; i++) {
        b32 second = 2*i<n && i+height<n;
        // the left column is cut one character later than the right
        helpcolumn(sh, &b, helptopics+i, width-3, second ? width : 0);
        if (second) {
            helpcolumn(sh, &b, helptopics+i+height, width-4, 0);
        }
        putbyte(&b, '\n');
    }
    bprint(sh, finish(&b));
}

static void helpman(Shell *sh, Buf *b, Arena *a, HelpTopic const *t)
{
    Cut first = cut(t->text, '\n');
    print(b, "NAME\n    ");
    print(b, t->name);
    print(b, " - ");
    print(b, first.head);
    print(b, "\n\nSYNOPSIS\n    ");
    print(b, t->synopsis);
    print(b, "\n\nDESCRIPTION\n");
    helptext(b, t);
    print(b, "\nSEE ALSO\n    bash(1)\n\nIMPLEMENTATION\n    ");
    print(b, helpversion(sh, a));
    print(b, "\n    Public domain: free and unencumbered software\n");
}

// help [-dms] [pattern ...]: each pattern selects the topics whose names
// it matches, exactly or as a pattern, else those it begins
static i32 b_help(Shell *sh, Str *args, iz nargs, Arena scratch)
{
    b32 dflag = 0, mflag = 0, sflag = 0;
    iz  i     = 1;
    for (; i<nargs && args[i].len>1 && args[i][0]=='-'; i++) {
        if (args[i] == "--") { i++; break; }
        for (iz k = 1; k < args[i].len; k++) {
            switch (args[i][k]) {
            case 'd': dflag = 1; break;
            case 'm': mflag = 1; break;
            case 's': sflag = 1; break;
            default:
                invalidoption(sh, scratch, "help", span(args[i].data+k-1, args[i].data+k+1));
                return 2;
            }
        }
    }
    if (i == nargs) {
        helplist(sh, scratch);
        sh->nowriteerr = 1;  // as bash, which checks only the other forms
        return 0;
    }

    Buf b(&scratch, 4096);
    if (hasglob(args[i], 0)) {
        print(&b, i+1<nargs ? Str("Shell commands matching keywords `")
                            : Str("Shell commands matching keyword `"));
        for (iz k = i; k < nargs; k++) {
            if (k > i) print(&b, ", ");
            print(&b, args[k]);
        }
        print(&b, "'\n\n");
    }
    iz  found   = 0;
    Str pattern = {};
    for (; i < nargs; i++) {
        pattern = args[i];
        for (i32 pass = 1; pass <= 2; pass++) {
            b32 here = 0;
            for (iz k = 0; k < countof(helptopics); k++) {
                HelpTopic const *t = helptopics + k;
                b32 m = pass==1 ? t->name==pattern || patmatch(pattern, t->name, PM_EXTGLOB, scratch)
                                : startswith(t->name, pattern);
                if (!m) continue;
                here = 1;
                found++;
                if (dflag) {
                    print(&b, t->name);
                    print(&b, " - ");
                    print(&b, cut(t->text, '\n').head);
                    putbyte(&b, '\n');
                } else if (mflag) {
                    helpman(sh, &b, &scratch, t);
                } else {
                    print(&b, t->name);
                    print(&b, ": ");
                    print(&b, t->synopsis);
                    putbyte(&b, '\n');
                    if (!sflag) helptext(&b, t);
                }
            }
            if (here) break;
        }
    }
    bprint(sh, finish(&b));
    if (!found) {
        Str msg = concat3(&scratch, "no help topics match `", pattern, "'.  Try `help help' or `man -k ");
        msg = concat3(&scratch, msg, pattern, "' or `info ");
        msg = concat3(&scratch, msg, pattern, "'.");
        builtinerror(sh, scratch, "help", msg);
        return 1;
    }
    return 0;
}

// NAME --help for a builtin: its synopsis and description, status 2
static i32 builtinhelp(Shell *sh, Str name, Arena scratch)
{
    HelpTopic const *t = helptopic(name);
    if (!t) return 2;
    Buf b(&scratch, 2048);
    print(&b, name);
    print(&b, ": ");
    print(&b, t->synopsis);
    putbyte(&b, '\n');
    helptext(&b, t);
    bprint(sh, finish(&b));
    return 2;
}
