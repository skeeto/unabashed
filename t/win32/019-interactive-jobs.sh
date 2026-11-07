# shell: -i
# conrun: sleep:500 'type:PS1="$ "\n' 'type:{ sleep 30 & p=$!; sleep 30 & } 2>/dev/null; jobs\n' sleep:500 'type:fg\n' sleep:800 ctrl-c sleep:500 'type:echo st=$?; jobs\n' 'type:wait\n' sleep:800 ctrl-c sleep:500 'type:echo w=$?; kill -0 $p && echo alive; kill %1; wait %1; echo k=$?\n' sleep:500 'type:{ sleep 0.5 & } 2>/dev/null\n' sleep:1500 'type:\n' sleep:300 'type:exit\n'
# Interactive job control on Windows: jobs lists background programs,
# fg waits for one and Ctrl-C ends it; Ctrl-C interrupts wait (status
# 130) and the jobs run on; wait reports the job kill ended; a job that
# ends is reported before the next prompt.
