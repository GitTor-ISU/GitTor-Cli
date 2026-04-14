#ifndef _WIN32

#include <git2.h>
#include <glib.h>
#include <gpgme.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <examples/signature_extract.h>
#include <examples/signature_validate.h>

/*
 * Sets up an isolated GPG context and imports our trusted keys.
 * This ensures we ONLY validate against the keys in the repository file,
 * and we don't accidentally rely on the user's global ~/.gnupg keychain.
 */
gpgme_ctx_t setup_isolated_gpg_context(const char* keys_file_path) {
    char template[] = "/tmp/repo-gnupg-XXXXXX";
    char* gpg_home = mkdtemp(template);
    if (!gpg_home) {
        perror("Failed to create temp GPG home");
        return NULL;
    }

    setenv("GNUPGHOME", gpg_home, 1);

    gpgme_check_version(NULL);
    gpgme_ctx_t ctx;
    if (gpgme_new(&ctx) != GPG_ERR_NO_ERROR) {
        g_printerr("Failed to initialize GPGME context.\n");
        return NULL;
    }

    gpgme_data_t key_data;
    if (gpgme_data_new_from_file(&key_data, keys_file_path, 1) !=
        GPG_ERR_NO_ERROR) {
        g_printerr("Failed to read keys file at: %s\n", keys_file_path);
        gpgme_release(ctx);
        return NULL;
    }

    gpgme_error_t err = gpgme_op_import(ctx, key_data);
    if (err != GPG_ERR_NO_ERROR) {
        g_printerr("Failed to import keys: %s\n", gpgme_strerror(err));
    }

    gpgme_data_release(key_data);
    return ctx;
}

/*
 * Extracts the signature from a specific commit and verifies it
 * against our isolated GPG context.
 */
int verify_commit_signature(gpgme_ctx_t ctx,
                            git_repository* repo,
                            const char* commit_hash) {
    git_oid oid;
    if (git_oid_fromstr(&oid, commit_hash) < 0) {
        g_printerr("Invalid commit hash format.\n");
        return -1;
    }

    git_buf signature = {0};
    git_buf signed_data = {0};

    if (git_commit_extract_signature(&signature, &signed_data, repo, &oid,
                                     NULL) < 0) {
        const git_error* e = git_error_last();
        g_printerr("Commit %s has no signature or extraction failed: %s\n",
                   commit_hash, e ? e->message : "");
        return -1;
    }

    gpgme_data_t sig_data, text_data;
    gpgme_data_new_from_mem(&sig_data, signature.ptr, signature.size, 0);
    gpgme_data_new_from_mem(&text_data, signed_data.ptr, signed_data.size, 0);

    gpgme_op_verify(ctx, sig_data, text_data, NULL);
    gpgme_verify_result_t result = gpgme_op_verify_result(ctx);

    int is_valid = 0;
    if (result && result->signatures) {
        gpgme_signature_t sig = result->signatures;

        if (gpgme_err_code(sig->status) == GPG_ERR_NO_ERROR) {
            g_print("[SUCCESS] Valid signature from Key Fingerprint: %s\n",
                    sig->fpr);
            is_valid = 1;
        } else {
            g_print(
                "[FAILURE] Invalid signature or key missing. FPR: %s (Error: "
                "%s)\n",
                sig->fpr ? sig->fpr : "unknown", gpgme_strerror(sig->status));
        }
    } else {
        g_print(
            "[FAILURE] Verification failed to find a recognizable signature "
            "block.\n");
    }

    // Cleanup
    gpgme_data_release(sig_data);
    gpgme_data_release(text_data);

    git_buf_dispose(&signature);
    git_buf_dispose(&signed_data);

    return is_valid ? 0 : -1;
}

#endif
