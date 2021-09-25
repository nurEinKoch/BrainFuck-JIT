#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdint.h>

#include "brainfuck_jit.hxx"

#define SIZE_CODE_BUFFER (1 << 20)
#define SIZE_STACK_BUFFER (1 << 24)
#define STACK_PRINT_DEPTH 40


void (*jit_code)(unsigned char* stack);

int main(int argc,  const char** argv) {
    struct stat file_prop;
    int source_file = open(argv[1], O_RDONLY);
    fstat(source_file, &file_prop);
    int file_len = file_prop.st_size;
    printf("filesize: %i\n", file_len);

    char* source = (char*)mmap(nullptr, file_len, PROT_READ, MAP_PRIVATE, source_file, 0);
    close(source_file);

    write(STDOUT_FILENO, source, file_prop.st_size);
    write(STDOUT_FILENO, "\n", 1);

    

    unsigned char* code_buf = (unsigned char*)mmap(0, SIZE_CODE_BUFFER, PROT_READ|PROT_WRITE|PROT_EXEC, MAP_ANONYMOUS|MAP_PRIVATE, -1, 0);
    jit_code = (void (*)(unsigned char*)) code_buf;
    unsigned char* stack_buf = (unsigned char*)malloc(SIZE_STACK_BUFFER);
    //void* sp;
    brainfuck_jit jit(code_buf, putchar, getchar);
    {
        jit.init();
        
        for( int i = 0; i < file_len; i++) {
            switch(source[i]) {
                case '+':
                    //jit.emit_increase();
                    {
                        int old = i;
                        while(source[++i]=='+');
                        printf("add %i\n", i-old);
                        jit.emit_add(i-old);
                        i--;
                    }

                break;
                case '-':
                    //jit.emit_decrease();
                    {
                        int old = i;
                        while(source[++i]=='-');
                        printf("sub %i\n", i-old);
                        jit.emit_sub(i-old);
                        i--;
                    }
                break;
                case '[':
                    jit.open_loop();
                break;
                case ']':
                    jit.close_loop();
                break;
                case '>':
                    jit.emit_nextcell();
                break;
                case '<':
                    jit.emit_prevcell();
                break;
                case '.':
                    jit.emit_putchar();
                break;
                case ',':
                    jit.emit_getchar();
                break;
                default:
                    
                break;
            }
        }
        munmap(source, file_len);
        jit.finalize();
        
    }
    for(int i = 0; i < jit.get_addr();i+=10) {
        for( int k = i; k < i+10; k++) {
            printf("%x, ", ((unsigned char*)jit_code)[k]);
        }
        putchar('\n');
    }
    printf("codesize: %i\n", jit.get_addr());
    jit_code(stack_buf);
    //asm("mov %%rsp, %0;": "=r"(sp): : "%rax");
    puts("\n\n");
    for(int i = 0; i < STACK_PRINT_DEPTH;i+=10) {
        for( int k = i; k < i+10; k++) {
            printf("%i, ", ((unsigned char*)stack_buf)[k]);
        }
        putchar('\n');
    }
    free(stack_buf);
    munmap(code_buf, SIZE_CODE_BUFFER);
}