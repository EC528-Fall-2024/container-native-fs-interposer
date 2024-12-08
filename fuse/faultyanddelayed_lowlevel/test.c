#include <stdio.h>
#include <unistd.h>

void main(){
    printf("Hello World\n");
    sleep(10);
    int i = 0;
    while(i != 100){
        sleep(100);
        i++;
    }
    printf("Hello World\n");
    return;
}