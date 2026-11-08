# shell: -i
# conrun: sleep:500 'type:PS1="$ "\n' "type:{ busybox sh -c \"busybox sh -c 'sleep 2; echo leaked >leak'; :\" & } 2>/dev/null; sleep 0.3\n" sleep:800 'type:fg >/dev/null\n' sleep:800 ctrl-c sleep:500 'type:echo st=$?\n' sleep:3000 'type:case "$(cmd //c ver)" in *n\ 5.*|*n\ 6.[01].*) echo none;; *) cat leak 2>/dev/null || echo none;; esac\n' sleep:300 'type:exit\n'
# Ctrl-C to a background program fg brought forward ends what it
# started too, as the terminal's INT reaches a whole process group:
# the inner busybox does not finish its sleep and write the file.
# Windows 8 and later, where each background program has a job of its
# own; before, the file is written and not looked at (Windows 7 is
# version 6.1, XP 5.1).
