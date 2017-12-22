# SDC-compression-experiments
An improved simple dictionary compression algorithm implementation.

The original implementation was described in 
David Salomon
Data Compression. The Complete Reference. 4th ed. Chapter 3.2

This implementation doesn't use the slow code search during the last compression phase,
but uses a look-up table instead. Therefore the compression speed is imporved.

Also, this implementation makes use of the fact that we can store a 1-bit shorter code
and use the stored code length to calculate the original code during the decompression.
This helps to reduce a compressed size significantly.