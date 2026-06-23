1) Download the tensorflow repo :
https://github.com/tensorflow/tensorflow

2) Build the shared library : 

The following command will build the library : libtensorflowlite_c.so
bazel build -c opt --define=tflite_with_xnnpack=false //tensorflow/lite/c:libtensorflowlite_c.so