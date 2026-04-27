# GitTor CLI

A decentralized Git hosting platform where repositories are synchronized between users via a P2P protocol using torrenting. The purpose of this application has been to eliminate the issue of a single point of failure, which affects other platforms such as GitHub and GitLab. The problems that users identified with centralized platforms were as follows:

If the hosting platform goes down, there is no automatic fallback, meaning collaboration on a project is completely halted until an alternative means of sharing is implemented. The switch from one hosting platform to another can take anywhere from a few hours to a few days, depending on the scale and complexity of a project.

When cloning a repository, the hosting platform can trivially add/remove any commits from the top of the log without it being obvious. The "author" and commit messages can be identical to the actual state of the repository, with the only difference being the repository content and commit hashes. Something that, in large projects, is rarely checked in depth.

By default, Git uses SHA-1 for commit hashes, which has been broken. With enough time and computing power, both of which Microsoft (the owner of GitHub) has, a commit hash collision could be found to inject malicious code in the middle of a project's history. This tactic would be even more challenging to identify than the last, since it can occur at any point in the project's log, not just at the top, and the only difference would be the repository contents.

Finally, when the hosting platform is the only means of collaboration, they are the judge, jury, and executioner. Even if they don't change the state of the repositories, they can decide to block one’s commits, make malicious commits in a contributor’s name, or even delete repositories (ideally, there would still be an instance on someone's machine).

To address all of these issues, we minimize the power of the hosting platform as much as possible. Instead of hosting repositories through GitHub or GitLab,  everyone who contributes to it hosts the repository and shares it with others via a P2P protocol. When sharing the repository, GitTor seeds it with a .torrent file and by sharing this .torrent file with someone else, they are able to leech the repository, downloading it directly from the other contributors.

This strategy clearly resolved issue one; even if one computer is off, others are seeding the repository, allowing anyone with the .torrent file to leech it. The solutions to problems two and three are less clear. The .torrent file contains the hash of the contents, among other things. By using this to share repositories, any peer that downloads the repository can verify that every byte matches the original, no matter which seeder it came from. Lastly, we must address issue four. Since the contributors are the ones hosting the repository, they — as a collective — are the judge, jury, and executioner. They control the state of the repository, and it is up to the other contributors to decide if they wish to leech anyone's changes and work off of them.

However, this approach was not without some of its own concerns that need to be addressed. The primary problem with this approach was how to share changes to a repository among the contributors. Since any change to the repository would result in a different .torrent file that needs to be shared with all contributors. While this could be done by any means, email, paper, or even carrier pigeon, none of these would be convenient for GitTor users. Instead, we created a web application that allowed users to find, view, and obtain .torrent files for repositories. The difference between this and a typical hosting platform is that its power is significantly limited, as it has no control over the repository contents. Additionally, a simple, documented API is available, allowing alternative web applications to be easily created and switched to in minutes.

Another concern was that anyone with access to the repository could modify it, and there would be no way to restrict write access to verified contributors other than through the web application, which we wanted to limit its power as much as possible. To ensure that only authorized individuals can commit to a repository, a file in the repository stores the public GPG keys of those allowed to commit, which is used to verify the validity of the repository. To add another contributor, an already verified contributor must add the new contributor's GPG key to this file. When a new GitTor repository is initialized, it initially contains only the creator's GPG key.

To summarize, the overall structure of this project is two parts:

A command-line tool that allows users to easily publish the new state of their repository and seed it for other users to leech. The command-line tool also enables users to validate the commits via their GPG signatures and the valid contributor's keys file.

A web application for users to publish their .torrent files to and see other users' repositories with contents and pull requests. The application should also validate the signatures when people publish a new .torrent file and start seeding the repository itself.

As for the tools we used:

- CLI:
  - C for the source code
  - Many C libraries, such as libcurl, glib, argp, libtorrent, etc.
  - Unity C testing library with Gcovr coverage reports and Valgrind memory analysis
  - CppLint and Clang-Tidy
  - Pipeline for rule checks, tests, reports, and builds
- Web App:
  - API:
    - Spring Boot Java source code
    - JUnit integration and unit testing with JaCoCo coverage reports and context profiling
    - Autogenerated OpenAPI specification with documentation
    - Checkstyle rules
  - UI:
    - Angular 21 source code
    - Nginx runtime with reverse proxy to API
    - Cypress end-to-end testing
    - Tailwind styling with ZardUI component library
    - Autogenerated API connection code via OpenAPI specification
    - Prettier and ESLint rules
  - A PostgreSQL relational database
  - A MinIO Simple Storage Service
  - Docker Compose for a Dockerized application stack
  - Pipeline for rule checks, tests, reports, and builds

## Table of Contents

- [GitTor CLI](#gittor-cli)
  - [Table of Contents](#table-of-contents)
  - [Installation](#installation)
    - [Linux](#linux)
    - [Windows](#windows)
  - [Configuration](#configuration)
  - [Usage](#usage)

## Installation

### Linux

Before installing GitTor on Linux, you must first install all the dependencies:

- build-essential
- libtorrent-rasterbar-dev
- libglib2.0-dev
- libgit2-dev
- libcurl4-openssl-dev
- libjson-glib-dev

While this installation process is distribution-specific, if you are on a Debian-based distribution, the installation can be done with these commands:

```bash
sudo apt update
sudo apt install -y build-essential \
    libtorrent-rasterbar-dev \
    libglib2.0-dev \
    libgit2-dev \
    libcurl4-openssl-dev \
    libjson-glib-dev
```

As of now, no prebuilt binaries are provided for any Linux distributions. So, once you have all the necessary dependencies for GitTor, you can build the program from source with this command:

```bash
git clone https://github.com/GitTor-ISU/GitTor-Cli.git
cd GitTor-Cli
make
make install
```

During installation, the program may or may not prompt you to run a command to add GitTor to your PATH. Once this is complete, you should be able to call GitTor directly without any issues.
gittor --help

If you are encountering any issues installing GitTor on your system, please refer to the devcontainer configuration to help troubleshoot any discrepancies.

### Windows

To install GitTor on Windows, download the binary from the most recent build artifact [here](https://gittor-isu.github.io/GitTor-Cli/gittor-exe.zip). Extract this archive to a location on your system where you won't delete it, and add its path to your PATH environment variable. There are many tutorials online that can show how to edit your PATH environment variables.

From there, you should be able to call GitTor directly without any issues.

```bash
gittor --help
```

## Configuration

GitTor supports many custom configurations that should ideally be set up before you begin using the application; however, these can be changed at any time. In order to configure GitTor, open up a file named '.gittorconfig' in your HOME directory and begin by adding this content:

```ini
[network]
port=12345
api_url=https://gittor.rent/api
tracker1=https://tracker.moeblog.cn:443/announce
tracker2=https://tr.nyacat.pw:443/announce
tracker3=https://tr.highstar.shop:443/announce
tracker4=https://tracker.gcrenwp.top:443/announce
```

Some will recognize this format as the same as the '.gitconfig' configuration file for Git. This design choice was intentional to make an easier transition for our users already familiar with Git.

The 'port' configuration value is the port that GitTor uses for seeding repositories. While not entirely necessary, seeding repositories is much more effective when you have a public-facing port that can be configured here. However, we acknowledge that it is often outside the reach of everyday users to port forward their machine, but when possible, it makes leeching much faster for other users.

The 'api_url' configuration value is used to specify which instance of the GitTor Web application you wish to connect to for finding and uploading repository torrents. Here we have shown the URL to our GitTor web's API; however, if you decide to use your own deployment or someone else's, that would be configured here.

The trackers are a list of URLs to public torrent trackers, which are used to orchestrate connections between seeders and leechers for any given torrent. This list of tracker URLs can be anywhere from one to one hundred trackers, which will be used whenever you are creating a new torrent (i.e., pushing new content to a repository). The trackers shown here are just a few that our team used with some reliability throughout our development process, though it is entirely up to the user which trackers they want to use for their own torrents.

## Usage
GitTor has many sub-commands, many flags, and many parameters. So, while all possible uses of the CLI cannot be expressed here, the typical usage looks like this:

1. First, the repository is initialized, and in this case, the optional folder parameter is used with 'project.'
2. The user enters the repository and makes some basic changes.
3. These changes are applied using the typical git add and commit commands.
4. The repository is seeded for others, and since the web app does not recognize this project yet, the user is prompted to enter a name and description.

```bash
isaac@tux-dev:~/Documents$ gittor init project
isaac@tux-dev:~/Documents$ cd project/
isaac@tux-dev:~/Documents/project·main
·$ echo "Hello, World!" > file.txt
isaac@tux-dev:~/Documents/project·main
·$ git add file.txt
isaac@tux-dev:~/Documents/project·main
·$ git commit -m "Hello message"
[main 8af6dc6] Hello message
 1 file changed, 1 insertion(+)
 create mode 100644 file.txt
isaac@tux-dev:~/Documents/project·main
·$ gittor seed
GitTor service started.
Torrent name: project
Torrent description (optional): Simple hello project
Seeding Repository!
Repository ID: 26671ac0e1a590bba36f97d7ac9c29e51382254f
Torrent File: /home/isaac/.config/gittor/repos/26671ac0e1a590bba36f97d7ac9c29e51382254f.torrent
Magnet Link: 'magnet:?xt=urn:btih:8b45d8b3ebdcad3b8f9733145fe1daedb094b500&xt=urn:btmh:1220eac8460ca2bbe1b0430791a1e66f51fe22789d3320dfe6fdef78df699f5f1af2&tr=https%3a%2f%2ftr.highstar.shop%3a443%2fannounce&tr=https%3a%2f%2ftracker.gcrenwp.top%3a443%2fannounce&tr=https%3a%2f%2ftr.nyacat.pw%3a443%2fannounce&tr=https%3a%2f%2ftracker.moeblog.cn%3a443%2fannounce'
```

In the output, we see three different identifiers for the seeded repository: the repository ID, torrent file, and magnet link. Any one of these could be provided to another user to leech the repository and make changes; however, each has its trade-offs.

The repository ID is by far the shortest, simplest, and, as a bonus, it doesn't change when new updates are made to the repository. The trade-off is that it relies on the web application being functional, since it effectively translates these repository IDs into the most recent torrent files.

As for the magnet link and torrent file, they are quite similar in most ways except that the magnet link is a single string of text that can be easily shared with others rather than an entire file.

The recommended workflow is to use repository IDs, as they are the simplest and will only cause issues if the web application goes down. However, if there is an issue, use the magnet links until it is resolved, then return to the repository IDs. In the end, though, these are just different identifiers that can be transitioned between easily and serve primarily to give our users more options than anything else.

As far as the leeching side is concerned, this is typically what that would look like:

```bash
isaac@tux-dev:~/Documents$ gittor leech \ 26671ac0e1a590bba36f97d7ac9c29e51382254f project
GitTor service started.
Leech complete. Saving session state...
Closing leeching client...
isaac@tux-dev:~/Documents$ cd project/
isaac@tux-dev:~/Documents/project·main
·$ cat file.txt
Hello, World!
isaac@tux-dev:~/Documents/project·main
·$ git log
commit 8af6dc666076f24bbd43160cb54d070d1092f077 (HEAD -> main, origin/main, origin/HEAD)
Author: Isaac Denning <117934828+idenning2003@users.noreply.github.com>
Date:   Wed Apr 22 16:13:20 2026 -0500

    Hello message
```

Another important thing to note about leeching is that once a repository has been leeched, the user can simply run 'gittor leech' within the repository, without any identifier, and the repository ID will be automatically used to retrieve the latest state via torrenting.
The last command used frequently is the login command, which connects the CLI to the API. This is a basic command, but since session tokens regularly expire, it may need to be run daily.

```bash
isaac@tux-dev:~/Documents/project·main
·$ gittor login
Email or username: isaac
Password:
Login successful.
```

While not likely to be used often, there is one final command that may be useful to know: the service subcommand. This allows users to check on and manage the seeder service's state, which is best practice to restart after any changes to the '.gittorconfig' file.

```bash
isaac@tux-dev:~/Documents$ gittor service status
up
isaac@tux-dev:~/Documents$ gittor service stop
GitTor service stopped.
isaac@tux-dev:~/Documents$ gittor service status
down
isaac@tux-dev:~/Documents$ gittor service start
GitTor service started.
isaac@tux-dev:~/Documents$ gittor service restart
GitTor service stopped.
GitTor service started.
```
