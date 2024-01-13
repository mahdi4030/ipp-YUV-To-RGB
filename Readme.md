# IPP YUV to RGB conversion

Using Intel Performance Primitives for converting colorspaces

## Environement
In order to build with IPP acceleration, download `l_ipp_oneapi_p_2021.9.0.49454_offline.sh` from [Intel website](https://www.intel.com/content/www/us/en/developer/tools/oneapi/base-toolkit-download.html).<br>
The test requires libjpeg: `apt install libjpeg-dev`<br>

## Building & Testing
### Using IPP
```
mkdir -p build && cd build
cmake .. && make
```
### Using CPU conversion
In order to test the functionality of the test program, and for comparison:
```
mkdir -p build && cd build
cmake .. -DCMAKE_CXX_FLAGS="-DNAIVE_CONVERSION" && make
```
### Running the test
You might need to add the library to the LD PATH:
```
export LD_LIBRARY_PATH=`pwd`/build/
```
Execution:
```
./build/tests/conversion_test examples/rv-logo.jpg
```
