#ifndef UTILS_UTILS_H_
#define UTILS_UTILS_H_

#include <git2.h>
#include <limits.h>

// TODO(isaac): comment
extern int gittor_get_repo_id(git_oid* repo_id, git_repository* repo);

// TODO(isaac): comment
extern int gittor_git_push(git_repository* repo);

// TODO(isaac): comment
extern const char* gittor_remote_dir();

// TODO(isaac): comment
extern int gittor_remote_path(char buf[PATH_MAX], const git_oid* repo_id);

#endif  // UTILS_UTILS_H_
