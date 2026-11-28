// The release of unabashed itself, and the commit it was built from.
//
// VERSION is the release, shown by --version. It is not the bash it
// clones: BASH_VERSION, BASH_VERSINFO, the prompt's \v and \V, and the
// first line of --version and help say 5.3.0, for scripts that sniff
// them.
//
// UNABASHED_DESCRIBE is `git describe --dirty` of the tree built, such
// as v0.4.0-234-g4e22353, which --version shows in place of VERSION.
// It comes from, in order:
// - -DUNABASHED_DESCRIBE="..." on the command line;
// - unabashed_describe.h, which the CMake build writes as it builds;
// - git archive, which fills in the $Format$ strings below, as this
//   file is export-subst (.gitattributes): a tarball names its commit.
// Otherwise, as in a checkout built by its one compiler command, the
// strings stay as written and --version shows VERSION alone.
//
// This is free and unencumbered software released into the public domain.

#define VERSION "0.8.0"

#ifndef UNABASHED_DESCRIBE
#  if __has_include("unabashed_describe.h")
#    include "unabashed_describe.h"
#  else
#    define UNABASHED_DESCRIBE "$Format:%(describe)$"
#    define UNABASHED_COMMIT   "$Format:%h$"  // older git: no %(describe)
#  endif
#endif
#ifndef UNABASHED_COMMIT
#  define UNABASHED_COMMIT ""
#endif

// The version --version shows: the description without its tag's v,
// else the release with the commit, else the release alone.
static Str unabashedversion(Arena *a)
{
    Str d = UNABASHED_DESCRIBE;
    Str c = UNABASHED_COMMIT;
    if (d.len && d[0]!='$' && d[0]!='%') {
        return d[0]=='v' ? cuthead(d, 1) : d;
    } else if (c.len && c[0]!='$' && c[0]!='%') {
        return concat3(a, VERSION " (", c, ")");
    }
    return VERSION;
}
