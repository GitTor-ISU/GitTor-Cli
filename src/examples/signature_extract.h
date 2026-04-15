#ifndef EXAMPLES_SIGNATURE_EXTRACT_H_
#define EXAMPLES_SIGNATURE_EXTRACT_H_

#include <git2.h>

extern char* signature_extract(const char* repo_path, const char* commit_sha);
extern char* get_head_commit_sha(git_repository* repo);
extern char* find_git_repo_path(void);

#endif  // EXAMPLES_SIGNATURE_EXTRACT_H_
