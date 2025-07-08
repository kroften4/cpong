# Installation

- Install SDL3 library 

(idk the steps for each distro, but SDL headers
such as <SDL3/SDL.h> must be present in the end. For debian-based systems I
did not figure it out as SDL3 library is not in the repositories (classic
debian), but for fedora you can just do:

```
sudo dnf install SDL3-devel
```

- Compile the binaries with `make`

```
git clone https://github.com/kroften4/cpong
cd cpong
make
```

Congratulations! Run `bin/server` to start the server, 
`bin/client` to start the client

# Running

- run server on port 8889:

```
./bin/server 8889
```

if you want to test this locally, just run 2 clients with

```
./bin/client localhost 8889
```

The client will run the game, use W-S/Up-Down to move your paddle

