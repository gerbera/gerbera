# Contributing

We welcome pull requests from everyone!

MediaTomb is an old project that we are working on modernising - there are a lot of cobwebs 🕸 .

For new code please use modern C++ (up to 17) constructs where possible, and avoid the `zmm` namespace.

1. Fork this repo.

2. Clone your fork:

    `git clone git@github.com:your-username/gerbera.git`

2. Make your change.

3. Push to your fork

4. [Submit a pull request](https://github.com/gerbera/gerbera/compare).

Some things that will increase the chance that your pull request is accepted:

* Stick to [Webkit style](https://webkit.org/code-style-guidelines/).
* Write a [good commit message](http://tbaggery.com/2008/04/19/a-note-about-git-commit-messages.html).

It is also a good idea to run cmake with`-DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_CXX_FLAGS="-Werror"`
options for development.
