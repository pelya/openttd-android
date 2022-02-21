## How to build with Emscripten

Building with Emscripten works with emsdk 3.0.0 and above.

You will also need autoconf, libharfbuzz-dev, and libicu-dev installed on your host OS.


```
  sudo apt-get install autoconf automake libtool libharfbuzz-dev libicu-dev

  ./emscripten-build.sh
```

And now you have in your build folder files like "openttd.html".

To run it locally, you would have to start a local webserver, like:

```
  cd build
  python3 -m http.server
````

Now you can play the game via http://127.0.0.1:8000/openttd.html .
