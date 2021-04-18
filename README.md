# SENG2021 Project

A webserver which communicates with an API. The project, named CinemaScout, is an ultra-fast movie database traversal tool which procedurally generates a series of questions to filter the entire IMDb database. 

A live deployment of this project might be available at [cinemascout.xyz](http://cinemascout.xyz).

# Dependencies
Build dependencies:
- [CMake](https://cmake.org) : CMake is a cross platform, open-source build system generator.
- [Googletest](https://github.com/google/googletest) : Googletest is a modern C++ testing framework.
- [cpp-httplib](https://github.com/yhirose/cpp-httplib) : A C++11 single-file header-only cross platform HTTP/HTTPS library.
- [Boost](https://www.boost.org) : Boost provides free peer-reviewed portable C++ source libraries.
- [Curl](https://curl.se/libcurl) : Curl is a free and easy-to-use client side URL transfer library.

Runtime dependencies:
- [IMDb](https://www.imdb.com/interfaces) : IMDb is an online database of information related to films and other visual media.
- [OMDb](https://www.omdbapi.com) : The OMDb API is a RESTful web service to obtain movie information. <strong>(PRIVATE API KEY REQUIRED)</strong>
- [Bulma](https://www.bulma.io) : Bulma is a free and open source CSS framework.

# Building from Source

To download the latest available release, clone this repository over github.

```console
    $ git clone https://github.com/nJ3ahxac/seng2021_project.git
```

Then, initialise the submodules after moving into the project directory.

```console
    $ cd ./seng2021_project
    $ git submodule update --init
```

Finally, compile from source via cmake after moving into the SourceCode_and_Documentation directory.
```console
    $ cd ./SourceCode_and_Documentation
    $ cmake ./
    $ make
```

# Usage

The webserver binary should be executed when the current working directory is <strong>inside</strong> `SourceCode_and_Documentation`. To start the server, run the binary `webserver` located in `src`. It should look like the following:
```console
    $ ./src/webserver
```

On first launch, the webserver will update the database cache via the runtime dependencies. One of the dependencies, `OMDb`, requires an api key, which may be acquired [here](https://www.omdbapi.com/apikey.aspx). Put the key into `SourceCode_and_Documentation/key.txt`.

<strong>Warning</strong>: the freely provided api key does not provide enough bandwidth to fulfill our requirements. It will be necessary to acquire a premium key with greater bandwidth limits.

After the cache has been updated, the webserver can typically be accessed at `localhost:<PORT>`.

# Testing and Contributing

We are using the testing framework [Googletest](https://github.com/google/googletest). By default, unit tests are automatically executed during the build process. Ensure that no test failures are present before making a pull request. Attempt to follow the current testing layout style. For example, non-statically linked functions that may server a purpose in other areas should be placed in `src/util/util.cc` and tested accordingly in `test/tests/utiltest.cc`. 

For code which it is not viable to test directly, it is acceptable that the expected behaviour is instead tested through networked means (see `test/tests/servertest.cc` for an example of system tests).
