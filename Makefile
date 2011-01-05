all: fetch co build run-static

fetch:
	echo Fetching the Mongrel2...
	mkdir repo
	fossil clone http://mongrel2.org:44445 repo/mongrel2.fossil
co:
	echo Checking out Mongrel2 from repository
	mkdir source
	(cd source; git init; fossil export --git ../repo/mongrel2.fossil | git fast-import; git checkout trunk)
co2:
	echo Checking out Mongrel2 from repository
	mkdir source
	(cd source; fossil open ../repo/mongrel2.fossil)
build:
	echo Building Mongrel2
	(cd source; make)
build-dev:
	echo Building Mongrel2
	(cd source; make dev)
clean:
	(cd source; make clean)
	(cd live-tests; make clean)

run-static:
	(PATH=`pwd`/source/tools/m2sh/build:`pwd`/source/bin:$PATH; cd site-static; m2sh load; m2sh start --name test)

run-ssl:
	(PATH=`pwd`/source/tools/m2sh/build:`pwd`/source/bin:$PATH; cd site-static-ssl; m2sh load; m2sh start --name test)

livetest:
	echo Please wait, running tests...
	(cd live-tests; make; ./vivosector ../source/bin/mongrel2 ../site-static "f400bf85-4538-4f7a-8908-67e313d515c2" 5 tests 127.0.0.1 6767 2>stderr >stdout)
	echo Done.

distclean:
	rm -rf source repo



