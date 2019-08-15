example.dll:
	gcc -shared example.c -o example.dll


.PHONY: clean
clean:
	rm example.dll
