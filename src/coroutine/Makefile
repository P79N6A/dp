SHELL = /bin/bash
DIR = st co2

all:
	for dir in $(DIR); \
    do\
        cd $$dir && make && cd -; \
    done

clean:
	for dir in $(DIR); \
	do \
        cd $$dir && make clean && cd -; \
    done
