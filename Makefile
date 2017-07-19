boot: tinyboot.cpp
	g++ -ggdb -std=c++11 -Wall -Wextra -pedantic tinyboot.cpp -o boot
	./boot tinyboot1.tbf1 < tinyboot1.tbf1 > tinyboot-gen
	chmod +x tinyboot-gen
