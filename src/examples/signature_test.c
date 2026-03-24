#include <git2.h>
#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include "examples/signature_extract.h"
#include "examples/signature_validate.h"

int main(int argc, char** argv) {
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
        g_printerr("Failed to open repository at %s\n", repo_path);
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
    // free(commit_sha);
    // git_repository_free(repo);
    // free(repo_path);
    // git_libgit2_shutdown();

    if (argc != 2) {
        printf("Usage: %s <path_to_repo> <path_to_keys.asc> <commit_hash>\n",
               argv[0]);
        return 1;
    }

    // repo_path = argv[1];
    // const char* keys_file = argv[2];
    // const char* commit_hash = argv[3];

    const char* keys_file = argv[1];
    const char* commit_hash = commit_sha;

    // Initialize libgit2
    git_libgit2_init();

    // Setup our trusted GPG environment
    gpgme_ctx_t gpg_ctx = setup_isolated_gpg_context(keys_file);
    if (!gpg_ctx)
        return 1;

    // Open the local Git repository
    if (git_repository_open(&repo, repo_path) < 0) {
        fprintf(stderr, "Could not open repository at %s\n", repo_path);
        return 1;
    }

    // Verify the commit
    int result = verify_commit_signature(gpg_ctx, repo, commit_hash);

    // Cleanup
    free(commit_sha);
    free(repo_path);
    git_repository_free(repo);
    gpgme_release(gpg_ctx);
    git_libgit2_shutdown();

    return result == 0 ? 0 : 1;
}
