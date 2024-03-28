FLAGS   = -lsfml-system -lsfml-window -lsfml-graphics -flto
SRC_DIR = source
EXE_DIR = executables
OBJ_DIR = obj

all: $(OBJ_DIR) $(EXE_DIR) SIMD NoSIMD NoSIMD2 mandelbrot mandelbrot_high_resolution



$(OBJ_DIR):
	@mkdir $(OBJ_DIR)

$(EXE_DIR):
	@mkdir $(EXE_DIR)



SIMD: $(OBJ_DIR)/SIMD-O0.o $(OBJ_DIR)/SIMD-O3.o
	@g++ $(OBJ_DIR)/SIMD-O0.o $(FLAGS) -o $(EXE_DIR)/SIMD-O0.out
	@g++ $(OBJ_DIR)/SIMD-O3.o $(FLAGS) -o $(EXE_DIR)/SIMD-O3.out

$(OBJ_DIR)/SIMD-O0.o: $(SRC_DIR)/SIMD.cpp
	@g++ -c -mavx2 $< -O0 -o $@

$(OBJ_DIR)/SIMD-O3.o: $(SRC_DIR)/SIMD.cpp
	@g++ -c -mavx2 $< -O3 -o $@



NoSIMD: $(OBJ_DIR)/NoSIMD-O0.o $(OBJ_DIR)/NoSIMD-O3.o
	@g++ $(OBJ_DIR)/NoSIMD-O0.o $(FLAGS) -o $(EXE_DIR)/NoSIMD-O0.out
	@g++ $(OBJ_DIR)/NoSIMD-O3.o $(FLAGS) -o $(EXE_DIR)/NoSIMD-O3.out

$(OBJ_DIR)/NoSIMD-O0.o: $(SRC_DIR)/NoSIMD.cpp
	@g++ -c -mavx2 $< -O0 -o $@

$(OBJ_DIR)/NoSIMD-O3.o: $(SRC_DIR)/NoSIMD.cpp
	@g++ -c -mavx2 $< -O3 -o $@



NoSIMD2: $(OBJ_DIR)/NoSIMD2-O0.o $(OBJ_DIR)/NoSIMD2-O3.o
	@g++ $(OBJ_DIR)/NoSIMD2-O0.o $(FLAGS) -o $(EXE_DIR)/NoSIMD2-O0.out
	@g++ $(OBJ_DIR)/NoSIMD2-O3.o $(FLAGS) -o $(EXE_DIR)/NoSIMD2-O3.out

$(OBJ_DIR)/NoSIMD2-O0.o: $(SRC_DIR)/NoSIMD2.cpp
	@g++ -c -mavx2 $< -O0 -o $@

$(OBJ_DIR)/NoSIMD2-O3.o: $(SRC_DIR)/NoSIMD2.cpp
	@g++ -c -mavx2 $< -O3 -o $@



mandelbrot: $(OBJ_DIR)/mandelbrot.o
	@g++ $< $(FLAGS) -o $(EXE_DIR)/mandelbrot.out

$(OBJ_DIR)/mandelbrot.o: $(SRC_DIR)/SIMD.cpp
	@g++ -D RENDER -c -mavx2 $< -O3 -o $@



mandelbrot_high_resolution: $(OBJ_DIR)/mandelbrot-mandelbrot_high_resolution.o
	@g++ $< $(FLAGS) -o $(EXE_DIR)/mandelbrot-mandelbrot_high_resolution.out

$(OBJ_DIR)/mandelbrot-mandelbrot_high_resolution.o: $(SRC_DIR)/SIMD-high.cpp
	@g++ -D RENDER -c -mavx2 $< -O3 -o $@