// The library's entry points (unabashed.h), for its roots
// (libunabashed_linux.cpp, libunabashed_darwin.cpp,
// libunabashed_win32.cpp), after the platform's layers.
// This is free and unencumbered software released into the public domain.

namespace unabashed {
namespace {
// From a fiber of a run only: the host's has no run to abort
[[noreturn]] static void os_oom()
{
    Lib *l = runlib();
    if (!l) __builtin_trap();
    lib_abort(&l->os, E_NOMEM);
}

static OsVtable const *osvtable(Os *)
{
    return &libvtable;
}
}  // namespace
}  // namespace unabashed

using namespace unabashed;

ub_os *ub_new(ub_config const *c)
{
    return (ub_os *)libnew(c);
}

int ub_run(ub_os *os, ub_cmd const *cmd)
{
    return librun((Lib *)os, cmd);
}

int ub_signal(ub_os *os, int sig)
{
    return libsignal((Lib *)os, sig);
}

void ub_cancel(ub_os *os)
{
    libcancel((Lib *)os);
}

void *ub_user(ub_os *os)
{
    return ((Lib *)os)->cfg.user;
}

void ub_free(ub_os *os)
{
    libfree((Lib *)os);
}

int ub_system(char const *cmd)
{
    ub_os *os = ub_new(0);
    if (!os) return -UB_ENOMEM;
    char const *argv[] = {"bash", "-c", cmd, 0};
    ub_cmd c = {};
    c.argc   = 3;
    c.argv   = argv;
    for (int i = 0; i < 3; i++) {
        c.std[i] = lib_hoststd(i);
    }
    int r = ub_run(os, &c);
    ub_free(os);
    return r;
}
