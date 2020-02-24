PJ_BASE=~/project/pjproject
include $(PJ_BASE)/build.mak

all: ../netchart.cgi ../NetTest.class ../netsrv.cgi

jj: ../NetTest.class

../netchart.cgi: netchart.c
	gcc -o $@ $< -Igd gd/gdc.o \
	    gd/gdchart.o gd/price_conv.o \
	    -Lgd/gd1.3 -lgd -lm

../NetTest.class: NetTest.java
	javac -target 1.1 -deprecation NetTest.java
	mv *.class ..

../netsrv.cgi: netsrv.c
	gcc -o $@ $< $(APP_CFLAGS) $(APP_LDFLAGS) $(APP_LDLIBS)

clean:
	rm -f *.o output.gif ../*.cgi ../*.class

