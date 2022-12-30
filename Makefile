TARGETS = int21.exe
all: $(TARGETS)

int21.exe: int21.c
	wcl -ml -we -fe=$@ -fm=$@.map -bcl=dos -I$(WATCOM)/h int21.c

clean:
	rm $(TARGETS)
