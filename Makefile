all: go c

c: c/
	make -C c

go: go/
	make -C go
