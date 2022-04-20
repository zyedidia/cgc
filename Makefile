cgc.o: cgc.c cgc.h
	$(CC) -Wno-return-stack-address -O2 -c -o $@ $<

clean:
	rm cgc.o
