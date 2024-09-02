# gmlgcd

gmlgcd is your gemlog companion enabling your visitors to interact with your posts.
It accepts FastCGI connections from a gemini server and handles them as comment requests.

## Building

Requirements:
- `libevent`
- `libconfuse`
- `libbsd` (on linux)
- `meson` (build)
- `ninja` (build)
- `fish` (test)

```bash
$ meson setup builddir --buildtype release

# opional: (systemd)
$ meson configure builddir -Dsystemddir=/etc/systemd/system

$ ninja -C builddir
$ sudo ninja -C builddir install
```

### OpenBSD

While I am using this on Linux, I paid attention to make it compatible with and buildable on OpenBSD.
Platform specific functionality like landlock(7) is replaced by pledge(2) and unveil(2).
Your mileage may vary, I haven't thouroughly tested gmlgcd on that platform, however.

## Usage

Edit `/etc/gmlgcd.conf` or set a different config file with `gmlgcd -c my.conf`.
An example configuration file is provided under [`gmlgcd.conf`](/gmlgcd.conf).

Then, redirect a given location / subpath to gmlgcd via fastcgi.
For example, with [gmid](https://github.com/omar-polo/gmid):

```nginx
server example.tld {
  ...
  location "/add-comment/*" {
    fastcgi socket "/run/gmlgcd/fcgi.sock"
    # or, depending on gmlgcd.conf
    fastcgi socket tcp "127.0.0.1" port 1851
  }
}
```

Then, in your gemini server webroot, create a directory to store your comments.
Say, your gemini webroot is at `/srv/gemini/example.tld`;
For every file that is supposed to receive comments, a writeable file needs to exist in `/srv/gemini/example.tld/comments`.
So, for `/srv/gemini/example.tld/{about.gmi,blog/post.gmi}`, `/srv/gemini/example.tld/comments/{about.gmi,blog/post.gmi}` have to exist.
You can then serve the `comments/` location just like any other location using your gemini server.
If the file does not exist, users will receive a warning indicating that comments are not enabled.
If the file is not writeable for gmlgcd, users will receive a warning that comments are not allowed.

Users may supply a username by prefixing their username, followed by a colon and a space: `username: comment` to set their displayed username.
Otherwise, a name will be taken from the user certificate. User certificates are required.
Also, ratelimiting takes place when too many bad requests have been issued in a too short amound of time.

## todo

Non-exhaustive, randomly ordered list of things I still want to do, until I consider this to be complete: (Contributions welcome)

- fully specify comment formatting, using a configuration string
- configuration of quarantine (rate-limiting)
- replace user hashes with emojis (maybe, configurable)
- test compatibility with other gemini servers
- write `gmlgcd.conf(5)` (and switch to manpage generation via pandoc. yikes!)
