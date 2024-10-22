pushd %~dp0

mkdir bin
cd bin
cmake ../src/
cmake --build . --config RelWithDebInfo

popd
