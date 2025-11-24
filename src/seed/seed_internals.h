#ifndef SEED_SEED_INTERNALS_H_
#define SEED_SEED_INTERNALS_H_

#include <glib.h>

extern int gittor_seed_start(const git_oid* repo_id);

extern int gittor_seed_stop(const git_oid* repo_id);

extern int gittor_seed_remove(const git_oid* repo_id);

#endif  // SEED_SEED_INTERNALS_H_
