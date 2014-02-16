echo "###Begin building Simple Logger###"
echo "Creating build directory..."
mkdir -p ./build
echo "Entering build directory..."
cd ./build
echo "Cleaning build directory..."
rm -rfv *
echo "Executing CMake..."
cmake ..
echo "Executing Makefile created by CMake..."
make
echo "Copying 'libsimplog.a' to root of project..."
cp -fv libsimplog.a ..
echo "###Simple Logger build complete###"
