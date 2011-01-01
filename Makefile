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
clean:
	(cd source; make clean)

run-static:
	(PATH=`pwd`/source/tools/m2sh/build:`pwd`/source/bin:$PATH; cd site-static; m2sh load; m2sh start --name test)

run-ssl:
	(PATH=`pwd`/source/tools/m2sh/build:`pwd`/source/bin:$PATH; cd site-static-ssl; m2sh load; m2sh start --name test)

distclean:
	rm -rf source repo



