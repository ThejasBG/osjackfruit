#include <stdio.h>
#include <windows.h>

int main() {
    for(int i = 0; i < 5; i++) {
        printf("Running iteration %d\n", i);
        Sleep(1000);
    }
    return 0;
}