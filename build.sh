mkdir -p ./build/bin
pushd ./build/bin
g++ -g -Wall -o handmade ../../handmade/code/macos_main.mm
popd