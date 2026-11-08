# B14: source bash-completion (github.com/scop/bash-completion, fetched
# outside the repository, never committed) 50 times; BASH_COMPLETION
# names its bash_completion file
f=${BASH_COMPLETION:?names the bash_completion file}
for ((i=0; i<50; i++)); do
    . "$f"
done
complete -p | wc -l
