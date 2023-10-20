# Plugin++


# Plugin Lifecycle

1) Plugin is loaded (handle::load), load() called
2) First step:
   * Go thread starts, go() called
   * start() called
   * step() called
3) Every subsequent step, step() called
4) A stop is request, stop() called