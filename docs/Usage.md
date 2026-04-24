## Build

```
make
```

To specify which build you want, pass in the arguments `dev` or `prod` like so.

```
make dev
```

All targets are as follows:

- `default` - Executed when no target is specified and calls `dev` and `prod`
- `dev` - Creates executable inside `bin/dev/` with debug symbols
- `prod` - Creates executable inside `bin/dev/` without debug symbols
- `test` - Creates test logs inside `bin/test/` and outputs the test results
- `report` - Creates test reports inside `site/`
- `format` - **DEPRECATED:** Formats code
- `lint` - Checks for any linting errors
- `clean` - Deletes all generated files

## Execute

The executable can be found at `bin/dev/gittor` or `bin/prod/gittor`.

```
Usage: gittor [OPTION...] COMMAND [ARGUMENTS...]
COMMANDS:

  init     Create an empty GitTor repository
  leech    Clone a GitTor repository into a new directory
  login    Authenticate with the GitTor server
  seed     Share the current state of the repository
  devs     Manage who can contribute to this repository
  verify   Verify all commits are from authorized developers
  config   Get and set GitTor local or global configurations
  service  Manage the GitTor service

OPTIONS:

  -p, --path=PATH            The path to the gittor repository
  -?, --help                 Give this help list
      --usage                Give a short usage message
```

### Init

```
Usage: gittor init [OPTION...] [DIRECTORY]
Initializes a new GitTor repository in the current directory.

  -?, --help                 Give this help list
      --usage                Give a short usage message
```

### Leech

```
Usage: gittor leech [OPTION...] [KEY] [DIRECTORY]
Downloads a repository given its key.

  -?, --help                 Give this help list
      --usage                Give a short usage message
```

Leeching downloads a repository given its KEY.

### Seed

```
Usage: gittor seed [OPTION...]
Uploads and shares the current state of the repository.

  -?, --help                 Give this help list
      --usage                Give a short usage message
```

Seed uploads and shares the current state of the repository.
If the local state is not from the newest UUID, users will be prompted if they want to get the most recent.

### Devs

```
Usage: gittor devs [-h|--help]
```

TODO: The functionality of this could be limited to reading and writing must be done within the configuration file, or this could have sub-commands like: ls, add, rm.

### Verify

```
Usage: gittor verify [-h|--help] [branch...]
```

Verifies that all commits are signed by authorized developers.

- Without arguments, verifies commits on every branch.
- When one or more branch names are specified, verifies commits only on those branches.

### Config

```
Usage: gittor config [OPTION...] <key> [<value>]
Read and write global (user-wide) or local (repository-wide) configurations.

  -g, --global               Use global (user-wide) configuration
  -l, --local                Use local (repository-wide) configuration
  -?, --help                 Give this help list
      --usage                Give a short usage message
```

Read and write global (user-wide) or local (repository-wide) configurations.
