boot: l00interpreter.cpp l00interpreter.py
	g++ -ggdb -std=c++11 -Wall -Wextra -pedantic l00interpreter.cpp -o l00interpreter
	./l00interpreter l01compiler.tbf1 < l01compiler.tbf1 > l01compiler
	python l00interpreter.py l01compiler.tbf1 < l01compiler.tbf1 > l01compiler-py
	@diff l01compiler l01compiler-py || ( echo "l01compiler differs between c++/py" && exit 1 )
	@chmod +x l01compiler && rm l01compiler-py
