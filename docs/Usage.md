## Build

```
make
```

To specify which build you want, pass in the arguments `dev` or `prod` like so.

```
make dev
```

## Execute

The executable can be found at `bin/dev/gittor` or `bin/prod/gittor`.

```
gittor [-h|--help] <command> [<args>]

Commands:
    init    Create an empty GitTor repository
    leech   Clone a GitTor repository into a new directory
    seed    Share the current state of the repository
    devs    Manage who can contribute to this repository
    verify  Verify all commits are from authorized developer
    config  Get and set GitTor local or global options
```

### Init

```
gittor init [-h|--help]
```

### Leech

```
gittor leech [-h|--help] <magnet>
```

### Seed

```
gittor seed [-h|--help]
```

### Devs

```
gittor devs [-h|--help] <command> [<args>]

Commands:
    ls      List all developers able to contribute to this repository
    add     Add a developer to the list of contributors
    rm      Remove a developer from the list of contributors
```

#### Ls

```
gittor devs ls [-h|--help]
```

#### Add

```
gittor devs add [-h|--help] # todo: unknown arguments
```

#### Rm

```
gittor devs rm [-h|--help] # todo: some identifier
```

### Verify

```
gittor verify [-h|--help] [-b|--branch]
```

### Config

```
gittor config [-h|--help] [--global|--local] <key> [<value>]
```
