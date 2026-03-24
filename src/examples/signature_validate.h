#ifndef EXAMPLES_SIGNATURE_VALIDATE_H_
#define EXAMPLES_SIGNATURE_VALIDATE_H_

#include <git2.h>
#include <gpgme.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

extern int verify_commit_signature(gpgme_ctx_t ctx,
                                   git_repository* repo,
                                   const char* commit_hash);
extern gpgme_ctx_t setup_isolated_gpg_context(const char* keys_file_path);

#endif  // EXAMPLES_SIGNATURE_VALIDATE_H_
