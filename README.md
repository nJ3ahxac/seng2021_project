# SENG2021 Project

A webserver which communicates with an API.

# Dependencies
Build dependencies:
- [CMake](https://cmake.org) : CMake is a cross platform, open-source build system generator.
- [Googletest](https://github.com/google/googletest) : Googletest is a modern C++ testing framework.
- [Pistache](http://pistache.io) : Pistache is a modern and elegant asynchronous HTTP and REST framework for C++.
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

The webserver binary should be executed when the current working directory is `seng2021_project/SourceCode_and_Documentation`. To start the server, run the binary `webserver` located in `./src`.
```console
    $ ./src/webserver
```
You may provide two optional arguments to the webserver, specifying the port and the the number of threads of execution. If no arguments are provided, the default values of 9080 and 1 are used respectively. If an argument of 0 is provided to threads, the amount of threads utilised will be equal to the return value of `std::thread::hardware_concurrency`.

On first launch, the webserver will update the database cache via the runtime dependencies. One of the dependencies, `OMDb`, requires an api key, which may be acquired [here](https://www.omdbapi.com/apikey.aspx). Put the key into `SourceCode_and_Documentation/key.txt`.

<strong>Warning: the freely provided api key does not provide enough bandwidth to fulfill our requirements. It will be necessary to acquire a premium key with greater bandwidth limits.</strong>

After the cache has been updated, the webserver can typically be accessed at `localhost:<PORT>`.
