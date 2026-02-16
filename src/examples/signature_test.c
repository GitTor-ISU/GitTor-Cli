#include <git2.h>
#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include "examples/signature_extract.h"

int main() {
    git_libgit2_init();

    // 1. Discover the repository path
    char* repo_path = find_git_repo_path();
    if (!repo_path) {
        git_libgit2_shutdown();
        return 1;
    }

    // 2. Open the discovered repository
    git_repository* repo = NULL;
    if (git_repository_open(&repo, repo_path) < 0) {
        fprintf(stderr, "Failed to open repository at %s\n", repo_path);
        free(repo_path);
        git_libgit2_shutdown();
        return 1;
    }

    // 3. Get the HEAD commit SHA
    char* commit_sha = get_head_commit_sha(repo);
    if (!commit_sha) {
        git_repository_free(repo);
        free(repo_path);
        git_libgit2_shutdown();
        return 1;
    }

    printf("Found repo: %s\n", repo_path);
    printf("Found HEAD commit: %s\n", commit_sha);

    // 4. Extract the signature using the discovered info
    char* signature = signature_extract(repo_path, commit_sha);

    if (signature) {
        printf("\n--- GPG Signature ---\n%s\n", signature);
        free(signature);
    } else {
        g_printerr("\nCould not retrieve signature for HEAD commit.\n");
    }

    // 5. Cleanup
    free(commit_sha);
    git_repository_free(repo);
    free(repo_path);
    git_libgit2_shutdown();

    return 0;
}
