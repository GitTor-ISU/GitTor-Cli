## Build

```
make
```

To specify which build you want, pass in the arguments `dev` or `prod` like so.

```
make dev
```

All targets are as follows:

-   `default` - Executed when no target is specified and calls `dev` and `prod`
-   `dev` - Creates executable inside `bin/dev/` with debug symbols
-   `prod` - Creates executable inside `bin/dev/` without debug symbols
-   `test` - Creates test logs inside `bin/test/` and outputs the test results
-   `report` - Creates test reports inside `site/`
-   `format` - **DEPRECATED:** Formats code
-   `lint` - Checks for any linting errors
-   `clean` - Deletes all generated files

## Execute

The executable can be found at `bin/dev/gittor` or `bin/prod/gittor`.

```
Usage: gittor [OPTION...] COMMAND [ARGUMENTS...]
COMMANDS:

  init    Create an empty GitTor repository
  leech   Clone a GitTor repository into a new directory
  seed    Share the current state of the repository
  devs    Manage who can contribute to this repository
  verify  Verify all commits are from authorized developers
  config  Get and set GitTor local or global configurations

OPTIONS:

  -p, --path=PATH            The path to the gittor repository
  -?, --help                 Give this help list
      --usage                Give a short usage message
```

### Init

```
Usage: gittor init [OPTION...] REPOSITORY
Initializes a new GitTor repository in the current directory.

  -n, --name=NAME            The name of the repository
  -?, --help                 Give this help list
      --usage                Give a short usage message
```

### Leech

```
gittor leech [-h|--help] <UUID>
```

Leeching downloads a repository given its UUID.
If the provided UUID is not the most recent for the given repository, the user will be prompted if they want to get the most recent.

### Seed

```
gittor seed [-h|--help]
```

Seed uploads and shares the current state of the repository.
If the local state is not from the newest UUID, users will be prompted if they want to get the most recent.

### Devs

```
gittor devs [-h|--help]
```

TODO: The functionality of this could be limited to reading and writing must be done within the configuration file, or this could have sub-commands like: ls, add, rm.

### Verify

```
gittor verify [-h|--help] [-b|--branch]
```

Verifies that all commits on the current branch are signed by authorized developers.

### Config

```
gittor config [-h|--help] [--global|--local] <key> [<value>]
```

Read and write global (user-wide) or local (repository-wide) configurations.
