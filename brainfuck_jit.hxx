#ifndef BRAINFUCK_JIT_H
#define BRAINFUCL_JIT_H


#define JIT_RETURN_ADDRESSES 2048

#include <stdint.h>

class brainfuck_jit {
    uint64_t returns[JIT_RETURN_ADDRESSES];
    signed top_ret ;
    uint64_t address;
    unsigned char* code;
    int (*putc)(int c);
    int (*getc)();

    public:

    enum instruction {
        save_cell = 0,

    };

    brainfuck_jit(unsigned char* code_buf, int(*putchar_)(int c), int (*getchar)()):code(code_buf), putc(putchar_), getc(getchar), address(0), top_ret(-1){};
    void finalize();
    void emit_putcell();
    void emit_return();
    void emit_increase();
    void emit_decrease();
    void emit_recoverSP();
    void emit_saveSP();
    void emit_zerocell();
    void emit_setSP();
    void emit_adjustSP();
    void emit_nextcell();
    void emit_prevcell();
    void emit_putchar();
    void emit_callsetup();
    void emit_call_cleanup();
    void emit_getchar();
    void emit_cmpzero();
    void emit_add(uint16_t x);
    void emit_sub(uint16_t x);
    void emit_catchnone();
    void init();
    void open_loop();
    void close_loop();

    int get_addr();
};

#endif