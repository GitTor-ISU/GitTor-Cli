#ifndef CMD_CMD_H_
#define CMD_CMD_H_

#include <limits.h>

/// @brief Arguments parsed from the command line
struct global_arguments {
    /// @brief Path to the gittor repository
    char path[PATH_MAX];
};

/**
 * @brief Parse the command line arguments to gittor
 *
 * @param argc Argument count
 * @param argv Argument array
 * @return int error code
 */
extern int cmd_parse(int argc, char** argv);

#endif  // CMD_CMD_H_
