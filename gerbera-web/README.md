# Gerbera Web UI

The Gerbera Web UI is written with the following tools:

### Client-side

- JQuery v3.2.1
- Bootstrap v4.0.0-beta.2


### Development

- Node JS v6.11.4
- Jasmine
- Karma
- Mocha


## Developing the UI

### Setup the enviornment

Using the `/gerbera-web` as the root folder setup the NodeJS environment.

```
$ npm install

```

### Run the JS specification tests

The javascript test cases are written in Jasmine + Karma Runner.  You can run the specs by issuing the following command.

```
$ npm test
```

Test output looks similar to below:

```
    Gerbera UI App
      initialize()
        ✓ retrieves the configuration from the server using AJAX
        ✓ stores the TYPE and SID cookies to the document
        ✓ defaults the TYPE to `db` when none is set
  
    Gerbera Auth
      checkSID()
        ✓ calls the server for a new SID and sets the cookie for SID
        ✓ calls GERBERA.App.error when the server call fails
      SID()
        ✓ retrieves the session ID from the cookie
      authenticate()
        ✓ calls the server to get a token and then logs the user into the system
      logout()
        ✓ calls the server to logout the session and expires the session ID cookie
  
    Gerbera Items
      initialize()
        ✓ clears the breadcrumb
        ✓ loads the # of view items based on default(25) when app config is undefined
        ✓ loads the # of view items based on application config
  
    Gerbera Menu
      initialize()
        ✓ binds all menu items with the click event
        ✓ on click of Database calls to load the containers
        ✓ on click of File System calls to load the filesystem
        ✓ on click of menu item becomes active
        ✓ on click of HOME calls the app home
  
    Gerbera Tree
      initialize()
        ✓ loads the tree view
      loadTree()
        ✓ renders the tree when response is successful
        ✓ does not render the tree when response is not successful
        ✓ selects the parent item by default
      transformContainers
        ✓ transforms containers response suitable for treeview load
      selectType()
        ✓ calls the server for containers when type is db
        ✓ calls the server for containers when type is db
        ✓ on failure calls the app error handler
      onItemSelected()
        ✓ updates the breadcrumb based on the selected item
  
  PhantomJS 2.1.1 (Linux 0.0.0): Executed 25 of 25 SUCCESS (0.371 secs / 0.367 secs)
  TOTAL: 25 SUCCESS

```

### Run the UI End-to-End Tests

The user interface has **end-to-end** tests which validate successful operation of the user interface.  
You can run the e2e specs by issuing the following command.

```
$ npm run test:e2e
```

Test output looks similar to below:

```
   The home page
     ✓ without logging in the database and filetype menus are disabled (123ms)
     ✓ enables the database and filetype menus upon login (753ms)
     ✓ disables the database and filetype menus upon logout (517ms)
     ✓ loads the parent database container list when clicking Database (736ms)
     ✓ loads the parent filesystem container list when clicking Filesystem (200ms)
     ✓ clears the tree when Home menu is clicked (179ms)
 
   The login page
     ✓ hides the login form when no authentication is required (192ms)
     ✓ shows the login form when no session exists yet and accounts is required (237ms)
     ✓ requires user name and password to submit the login form (327ms)
     ✓ when successful login show logout button, and show form on logout (883ms)
```

This command launches 2 simultaneous processes.

1. A **mock** web server for the server side `/content/interface` API data
2. A Selenium suite to exercise the user interface

### Debugging the Mock API

In a scenario where you need to monitor the mock api response sequence, you can add the `NODE_ENV` variable
to the `npm start` command.

```
$ NODE_ENV=development npm start
```

When in **development** the mock api will output `console.log` statements.


### Run the UI Manually

You can also run the Gerbera UI using the **mock server**.  This capability is limited
to the available **mock** responses stored in `/gerbara-web/mock-api`.

1. Start the **mock webserver**

     ```
     $ npm start
     ```
     
2. Reset the stored **mock** responses, by sending a specific **GET** request to the mock server

     > In this example we want to view the `home.spec.js` flow

          `http://localhost:3000/reset?testName=home.test.requests.json`
          
     > The server responds with an **OK**

3. Visit the website at `http://localhost:3000`
4. Step through scenario as per `test/e2e/home.spec.js`

# Gerbera UI Documentation

This folder contains scripts to generate documentation for the Gerbera UI.

### Screenshot Script

The screenshot script is a mocha end-to-end that captures screenshots using Selenium. The screenshot script runs the mock-server and iterates through common scenarios to
generate screenshots for use in the **Gerbera Docs**.

#### Running the Screenshot Script

You can launch the script from NPM.

```shell
npm run doc:screenshot
```

The script stores the images captured in the `gerbera-web/doc/*.png` folder

#### Adding New Screenshots

If you want to add to the screenshot collection spec, take a look
at the `gerbera-web/test/e2e/screenshot.doc.js` spec file.  The spec
captures a picture and manipulates it to generate for web use.



