all:
	echo "need action"

boot: tinyboot.cpp
	g++ -std=c++11 -Wall -Wextra -pedantic tinyboot.cpp -o boot
	./boot tinyboot1.tbf1 > tinyboot-gen

tiny:
	nasm -f elf64 tiny.asm
	gcc tiny.o -o tiny
	./tiny
