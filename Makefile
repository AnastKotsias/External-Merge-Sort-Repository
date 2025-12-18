sort:
	@echo " Compile sort_main ...";
	gcc -g -I ./include/ -L ./lib/ -Wl,-rpath,./lib/ ./examples/sort_main.c ./src/record.c ./src/sort.c ./src/merge.c ./src/chunk.c -lbf -lhp_file -o ./build/sort_main -O2

experiment:
	@echo " Compile experiment_main ...";
	gcc -g -I ./include/ -L ./lib/ -Wl,-rpath,./lib/ ./examples/experiment_main.c ./src/record.c ./src/sort.c ./src/merge.c ./src/chunk.c -lbf -lhp_file -o ./build/experiment_main

# New target for memory leak checking
valgrind: sort
	@echo " Running Valgrind on sort_main ...";
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./build/sort_main

clean:
	@echo " Cleaning build files...";
	rm -f ./build/*
