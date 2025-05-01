###setup
if false
then
    sudo apt update
    sudo apt install libglm-dev
    sudo apt install libglfw3-dev
    sudo apt install freeglut3 freeglut3-dev libglew-dev
fi

###compile & run
rm main
g++  *.cpp *.c -o main -lglfw
echo "Compiled"
./main
