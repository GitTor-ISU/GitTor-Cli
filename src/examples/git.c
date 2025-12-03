#include <git2.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "examples/git.h"

int git_example() {
    git_libgit2_init();

    const char* repo_path = ".";
    git_repository* repo = NULL;

    // Open the repo
    if (git_repository_open(&repo, repo_path) != 0) {
        const git_error* e = git_error_last();
        printf("Error %d: %s\n", e->klass, e->message);
        return 1;
    }

    git_reference* head = NULL;
    git_commit* commit = NULL;

    // Get the latest commit
    if (git_repository_head(&head, repo) == 0) {
        git_oid oid;
        git_reference_name_to_id(&oid, repo, git_reference_name(head));
        if (git_commit_lookup(&commit, repo, &oid) == 0) {
            printf("Latest commit: %s %s\n",
                   git_oid_tostr_s(git_commit_id(commit)),
                   git_commit_message(commit));
            git_commit_free(commit);
        }
        git_reference_free(head);
    }

    return 0;
}
