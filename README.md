# server-coursework-2014

Usage: 
```
cmake .
make
./bin/CourseWork 7273
```
Then set proxy in your browser to localhost/7273. That's it.
Tested it on nightly_35.0. Proxy supports chunked, HTTP/1.1, keep-alive, stuff. Perfectly works for me on 
almost all sites I need (though it can be slow for sites with big number of images on different hosts, I suppose).
