#include <cstdio>
#include <unistd.h>
#include <iostream>

int main(){
    std::cout << "Hello World\n" << std::endl;
    sleep(10);
    int i = 0;
    while(i != 100){
        sleep(100);
        i++;
    }
    std::cout << "Hello World\n" << std::endl;
    return 0;
}