# flowtest
If you want to run the app giving its two arguments to the command line. Do the following:

    gcc -std=c++17 Server.cpp BootUpApp.cpp -o mainProgram -lstdc++
    ./mainProgram <arg1> <arg2>
    
    
If you want to see the unit tests then do the following:

    gcc -std=c++17 Server.cpp Client.cpp unitTests.cpp -o unitTestsWithGcc -lstdc++
    ./unitTestsWithGcc
    
   
   


