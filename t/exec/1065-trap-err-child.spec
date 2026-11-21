== stdout 447
-- jobs
  err 9 1
  err 18 1
  err 19 1
  err 20 1
  err 21 1
-- stages
  err 27 1
  err 9 1
  err 31 1
  err 32 1
  err 32 1
  err 33 1
  err 33 1
  err 33 1
  err 34 4
  err 34 4
-- background pipelines
-- pipefail
  err 44 1
  err 45 3
  err 46 6
  err 46 6
  err 48 1
-- lastpipe
  err 52 1
  err 52 1
  err 53 1
  err 53 1
  err 53 1
-- errtrace off
  err 58 1
  err 59 1
  err 59 1
  err 60 1
-- set -e
  status 3
  status 1
  status 7
done

== stderr 110
bash: line 17: /nonexistent: No such file or directory
bash: line 30: /nonexistent: No such file or directory

== merged 557
-- jobs
  err 9 1
bash: line 17: /nonexistent: No such file or directory
  err 18 1
  err 19 1
  err 20 1
  err 21 1
-- stages
  err 27 1
  err 9 1
bash: line 30: /nonexistent: No such file or directory
  err 31 1
  err 32 1
  err 32 1
  err 33 1
  err 33 1
  err 33 1
  err 34 4
  err 34 4
-- background pipelines
-- pipefail
  err 44 1
  err 45 3
  err 46 6
  err 46 6
  err 48 1
-- lastpipe
  err 52 1
  err 52 1
  err 53 1
  err 53 1
  err 53 1
-- errtrace off
  err 58 1
  err 59 1
  err 59 1
  err 60 1
-- set -e
  status 3
  status 1
  status 7
done

== status 0
