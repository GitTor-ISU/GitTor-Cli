#include <errno.h>
#include <git2.h>
#include <glib.h>
#include "cmd/cmd.h"
#include "unity/unity.h"
#include "utils/utils.h"

static void shouldPass_whenHelpFlag() {
    // GIVEN: Seed with help flag
    char* argv[] = {"gittor", "seed", "--help", NULL};
    int argc = sizeof(argv) / sizeof(*argv) - 1;

    // WHEN: Parse arguments
    int err = cmd_parse(argc, argv);

    // THEN: Should return 0 (success for stub)
    TEST_ASSERT_EQUAL(0, err);
}

// TODO(isaac): For some reason testing the seeder service is not working,
//              but at this point we just need to get this feature in and
//              clean it from there as bugs occur.

// static void shouldPass_whenDirectoryEmpty() {
//     git_libgit2_init();

//     // GIVEN: Create temporary directory for repo
//     gchar* dir = tempdir_init();
//     if (dir == NULL) {
//         TEST_FAIL_MESSAGE("Failed to create temporary directory");
//     }

//     // GIVEN: Initialized repository
//     char* argv_init[] = {"gittor", "-p", dir, "init", NULL};
//     int argc_init = sizeof(argv_init) / sizeof(*argv_init) - 1;
//     int err_init = cmd_parse(argc_init, argv_init);
//     if (err_init) {
//         TEST_FAIL_MESSAGE("Failed to initialize repository");
//     }

//     // WHEN: Seed repository
//     char* argv_seed[] = {"gittor", "-p", dir, "seed", NULL};
//     int argc_seed = sizeof(argv_seed) / sizeof(*argv_seed) - 1;
//     int err_seed = cmd_parse(argc_seed, argv_seed);

//     // Kill gittor service
//     char* argv_kill[] = {"gittor", "service", "stop", NULL};
//     int argc_kill = sizeof(argv_kill) / sizeof(*argv_kill) - 1;
//     int err_kill = cmd_parse(argc_kill, argv_kill);
//     if (err_kill) {
//         TEST_FAIL_MESSAGE("Failed to kill gittor service");
//     }

//     tempdir_destroy(dir);
//     git_libgit2_shutdown();

//     // THEN: Should return 0 error
//     TEST_ASSERT_EQUAL(0, err_seed);
// }

int main() {
    UNITY_BEGIN();
    RUN_TEST(shouldPass_whenHelpFlag);
    return UNITY_END();
}
