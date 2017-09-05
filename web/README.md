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

### Early Development

The initial version of the Gerbera Web UI does not contain a complete feature
set of the existing system.  Early availability allows for fixes and improvements to
be made while additional features are in-progress.

While features are in-progress, the new Gerbera Web UI will co-exist with the original user interface system.

#### gerbera.html

The `/web/gerbera.html` file is the new Gerbera Web User Interface.  A single page application
that lives alongside the existing `/web/index.html`.  

> You can continue to use the existing user interface, while Gerbera Web UI is under development

##### Accessing the Gerbera Web UI

Visit the `gerbera.html` page directly in a browser from a running instance of Gerbera.

```
http://localhost:49152/gerbera.html
```

### Feature List

The Gerbera Web UI contains the list of initial features below.

- Login/Logout of Gerbera
- Database Items View
- Filesystem Items View
- Gerbera Tree Expand/Collapse
- Gerbera Items Download


### Reporting an Issue

If, while using Gerbera Web UI, you have suggestions for improvement
or encounter a failure [please report an issue](https://github.com/v00d00/gerbera/issues).

Browser debug information is helpful to identify the root cause of an issue.


### Developing Gerbera Web UI

The Gerbera Web UI contains a development environment using Node JS.  The development environment
provides for unit tests, end-to-end tests, mocking of server-side responses to verify user interface
behavior.  The development environment is maintained out of the `/gerbera-web` folder within the source code.

> Interested to develop and test the Gerbera Web UI? Review the `/gerbera-web/README.md` for futher details