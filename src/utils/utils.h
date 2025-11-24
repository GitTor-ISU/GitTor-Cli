#ifndef UTILS_UTILS_H_
#define UTILS_UTILS_H_

#include <git2.h>

extern int gittor_get_repo_id(git_oid* repo_id, git_repository* repo);

extern int gittor_git_push(git_repository* repo);

#endif  // UTILS_UTILS_H_
