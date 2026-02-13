#include <git2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "examples/signature_extract.h"

#define GET_CWD getcwd

/**
 * Private helper method for error message output
 *
 * @param error Error code
 * @param message Custom message for error
 */

static void log_git_error(int error, const char* message) {
    if (error < 0) {
        const git_error* e = giterr_last();
        fprintf(stderr, "Error %d: %s - %s\n", error, message,
                e ? e->message : "Unknown");
    }
}

/**
 * Gets the Git Repository Path for the current working directory.
 *
 * @return The cwd git repository path. Returns NULL on failure.
 */
char* find_git_repo_path(void) {
    git_buf repo_path_buf = {0};
    char* repo_path = NULL;
    char cwd[1024];

    if (!GET_CWD(cwd, sizeof(cwd))) {
        perror("Could not get current working directory");
        return NULL;
    }

    int error = git_repository_discover(&repo_path_buf, cwd, 1, NULL);
    if (error < 0) {
        log_git_error(error, "Failed to discover repository");
        git_buf_dispose(&repo_path_buf);
        return NULL;
    }

    repo_path = malloc(repo_path_buf.size + 1);
    if (repo_path) {
        strncpy(repo_path, repo_path_buf.ptr, repo_path_buf.size);
        repo_path[repo_path_buf.size] = '\0';
    }

    git_buf_dispose(&repo_path_buf);
    return repo_path;
}

/**
 * Gets the SHA string of the commit that HEAD points to. AKA the
 * most recent commit.
 *
 * @param repo A previously opened git_repository object.
 * @return A dynamically allocated 40-character string with the SHA,
 *         which the caller must free. Returns NULL on failure.
 */
char* get_head_commit_sha(git_repository* repo) {
    if (!repo) {
        fprintf(stderr,
                "Error: get_head_commit_sha called with NULL repository.\n");
        return NULL;
    }

    git_oid oid;
    char* sha_string = NULL;

    int error = git_reference_name_to_id(&oid, repo, "HEAD");
    if (error < 0) {
        log_git_error(error, "Failed to resolve HEAD to a commit");
        return NULL;
    }

    sha_string = malloc(GIT_OID_HEXSZ + 1);
    if (sha_string) {
        git_oid_tostr(sha_string, GIT_OID_HEXSZ + 1, &oid);
    }

    return sha_string;
}

/**
 * Gets the GPG Signature for the git commit given.
 *
 * @param repo_path A path to the git repository for the commit
 * @param commit_sha The sha for the commit we want to check
 * @return A GPG Signature. Returns NULL on failure.
 */
char* signature_extract(const char* repo_path, const char* commit_sha) {
    git_repository* repo = NULL;
    git_oid oid;
    git_buf signature_buf = {0};
    git_buf signed_data_buf = {0};
    char* signature_string = NULL;

    // Gets the Git Repository Object (repo)
    int error = git_repository_open(&repo, repo_path);
    if (error < 0) {
        log_git_error(error, "Failed to open repository");
        return NULL;
    }

    // Gets the Git Object ID
    error = git_oid_fromstr(&oid, commit_sha);  // Git Object ID
    if (error < 0) {
        log_git_error(error, "Failed to parse commit SHA");
        git_repository_free(repo);
        return NULL;
    }

    // Extracts GPG Signature
    error = git_commit_extract_signature(&signature_buf, &signed_data_buf, repo,
                                         &oid, NULL);
    if (error == GIT_ENOTFOUND) {
        fprintf(stderr, "No GPG signature found on commit %s\n", commit_sha);

    } else if (error < 0) {
        log_git_error(error, "Failed to extract signature");
    } else {
        signature_string = malloc(signature_buf.size + 1);
        if (signature_string) {
            memcpy(signature_string, signature_buf.ptr, signature_buf.size);
            signature_string[signature_buf.size] = '\0';
        }
    }

    git_buf_dispose(&signature_buf);
    git_buf_dispose(&signed_data_buf);
    git_repository_free(repo);

    return signature_string;
}
