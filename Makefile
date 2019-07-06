load.dll:
	gcc -shared load.c -o load.dll


.PHONY: clean
clean:
	rm load.dll
