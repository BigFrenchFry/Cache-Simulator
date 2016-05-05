FILES := src/cache.c src/trace.c src/queue.c

cacheSim: $(FILES)
	gcc -o $@ $^ -O3 -lm

cacheSimDebug: $(FILES)
	gcc -o $@ $^ -g -lm

clean:
	rm -rf cacheSimDebug.dSYM
	rm -f cacheSimDebug
	rm -f cacheSim

.PHONY : clean
