## How to build with Emscripten

Building with Emscripten works with emsdk 2.0.31 and above.

```
  ./emscripten-build.sh
```

And now you have in your build folder files like "openttd.html".

To run it locally, you would have to start a local webserver, like:

```
  cd build
  python3 -m http.server
````

Now you can play the game via http://127.0.0.1:8000/openttd.html .
