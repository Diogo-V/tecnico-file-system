# Description:
Program that simulates a file system with multiple threads that recieves an input file to processe and writes output tree in output file. <br />
Also receives a way os synching all the threads. <br />
Only uses a global mutex and so, is not very efficient

## How to run
Execute the following command:
```
make
./tecnicofs <inputfile> <outputfile> <numthreads> <synchstrategy>
```
