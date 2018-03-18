# Gerbera Test

This file outlines how the Gerbera system uses the **GoogleTest**
framework to provide unit testing for the C++ code-base.

## Required Software

- CMake ^3.9
- GoogleTest

## Enable Testing

The CMake build enables testing using the `WITH_TESTS` flag

```text
$ cmake ../gerbera -DWITH_TESTS=1
```

## Running All Tests

Build the project and run all the tests

```
$ make && make test
```

The tests output looks similar to below:

```
Running tests...
Test project /development/gerbera/build
    Start 1: testdictionary
1/2 Test #1: testdictionary ...................   Passed    0.02 sec
    Start 2: testruntime
2/2 Test #2: testruntime ......................   Passed    0.02 sec

100% tests passed, 0 tests failed out of 2

Total Test time (real) =   0.05 sec

```

## Running Specific Test

You can run specific tests by using `ctest`. Below is an example of running the
**testdictionary** test only.


```
$ ctest -R testdictionary
Test project /development/gerbera/build
    Start 1: testdictionary
1/1 Test #1: testdictionary ...................   Passed    0.02 sec

100% tests passed, 0 tests failed out of 1

Total Test time (real) =   0.03 sec
```

## Creating a New Test

Adding a new test to Gerbera is easy.  The process amounts to a few steps:

1. Create a new feature test folder similar to `/test/test_myfeature`
2. Create `CMakeLists.txt` within your new folder.
3. Create a `main.cc` Google Test file
4. Add your files to your `CMakeLists.txt` within your `/test/test_myfeature` folder
4. Add sub-directory `test_myfeature` to the parent `/test/CMakeLists.txt` file