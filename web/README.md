# Gerbera Web UI

This readme outlines the feature availability, design, and development 
of the Gerbera Web User Interface.

### History

The existing user interface (_originally MediaTomb_) uses `frameset` to
manage various content elements such as directory list, files, login/logout.

The HTML5 specification deprecates the `<frameset>` tag.  A new user interface
is needed to work within the recent technologies of the web.  The Gerbera
user interface leverages new conventions to improve the maintainability of the
code and provide new user features.

### Reporting an Issue

If, while using Gerbera Web UI, you have suggestions for improvement
or encounter a failure [please report an issue](https://github.com/gerbera/gerbera/issues).

Browser debug information is helpful to identify the root cause of an issue.


### Developing Gerbera Web UI

The Gerbera Web UI contains a development environment using Node JS.  The development environment
provides for unit tests, end-to-end tests, mocking of server-side responses to verify user interface
behavior.  The development environment is maintained out of the `/gerbera-web` folder within the source code.

> Interested to develop and test the Gerbera Web UI? Review the `/gerbera-web/README.md` for futher details