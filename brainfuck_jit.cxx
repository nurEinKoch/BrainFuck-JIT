#include "brainfuck_jit.hxx"
#include <memory.h>
#include <stdio.h>
#include <byteswap.h>

//di = current cell value
//rsi = old stack pointer
//rdx, zero
#define _DEF_INSTR(_i, ...) \
unsigned char const _i ## _bin[] = __VA_ARGS__;\
const int _i ## _len = sizeof(_i ## _bin);\
void brainfuck_jit::emit_ ## _i() { \
memcpy(code+address, _i ## _bin, _i ## _len);\
address+=_i ## _len;\
}

#define _DEF_BIN(_i, ...) \
unsigned char const _i ## _bin[] = __VA_ARGS__;\
const int _i ## _len = sizeof(_i ## _bin);

#define _EMIT_BIN(_i)\
memcpy(code+address, _i ## _bin, _i ## _len);\
address+=_i ## _len;


#define __EMIT(...) {\
    unsigned char const _bin[] = __VA_ARGS__;\
    int const _len = sizeof(_bin);\
    memcpy(code+address, _bin, _len);\
    address+=_len;\
}

#define _DEF_INSTR2(_i,_pre , _post, ...) \
void brainfuck_jit::emit_ ## _i() { \
    _pre;\
    __EMIT(__VA_ARGS__)\
    _post;\
}

#define __EMIT_8(_i) {\
    *((uint8_t*)(code + address))=_i;\
    address++;\
}

#define __EMIT_16(_i) {\
    *((uint16_t*)(code + address))=bswap_16(_i);\
    address+=2;\
}

#define __EMIT_32(_i) {\
    *((uint32_t*)(code + address))=bswap_32(_i);\
    address+=4;\
}
#define __EMIT_16_REV(_i) {\
    *((uint16_t*)(code + address))=_i;\
    address+=2;\
}
#define __EMIT_32_REV(_i) {\
    *((uint32_t*)(code + address))=_i;\
    address+=4;\
}

#define __EMIT_64(_i) {\
    *((uint64_t*)(code + address))=bswap_64(_i);\
    address+=8;\
}




#define MAX_INSTR_LEN 10

unsigned char instructions[][MAX_INSTR_LEN] = {
    {0x48, 0x8b, 0x7c, 0x24, 0xf8}
};
int instr_lens[] = {
    sizeof(instructions[0])
};


void brainfuck_jit::finalize() {
    emit_putcell();
    emit_recoverSP();
    emit_return();
}

void brainfuck_jit::init() {
    emit_saveSP();
    emit_setSP();
    emit_adjustSP();
    emit_zerocell();
}

void brainfuck_jit::emit_putcell() {
    //mov [rsp-2], di
    __EMIT_32(0x66897c24)
    __EMIT_8(0xfe);
}

/*void brainfuck_jit::emit_return() {
    __EMIT({0xc3});
}*/
_DEF_INSTR(zerocell, {0x48, 0x31, 0xff}) //xor rdi, rdi

//_DEF_INSTR(zerocellcond, {0x48, 0x31, 0xd2})    // xor rdx, rdx

_DEF_INSTR(setSP, {0x48, 0x89, 0xfc})   // mov rsp, rdi

_DEF_INSTR(saveSP, {0x48, 0x89, 0xe6}) // mov rsi, rsp

_DEF_INSTR(recoverSP, {0x48, 0x89, 0xf4})   //mov rsp, rsi

_DEF_INSTR(return, {0xc3})  //ret

_DEF_INSTR(increase, {0x66, 0xff, 0xc7})    //inc di

_DEF_INSTR(decrease, {0x66, 0xff, 0xcf});      //dec di

_DEF_INSTR(adjustSP, {0x48, 0x83, 0xc4, 0x02})  // add rsp, $0x2

/*_DEF_INSTR(putchar, {

})*/

_DEF_INSTR(cmpzero, {
    0x66, 0x83, 0xff, 0x00  // cmp $0
})



_DEF_INSTR(catchnone, {
    0x66, 0x83, 0xff, 0xff,  // cmp $-1
    0x75, 0x04,         // jnz L1
    0x66, 0xbf, 0x00, 0x00, // mov di, $0
    //L1:
})


_DEF_INSTR2(nextcell, emit_putcell(), emit_cmpzero() ,{
    0x66, 0x5f      // pop di
})

_DEF_INSTR2(prevcell, , emit_cmpzero(), {
    0x66, 0x57,     // push di
    0x66, 0x8b, 0x7c, 0x24, 0xfe    //mov di, [rsp-2]
})

_DEF_INSTR(callsetup, {
    0x48, 0x87, 0xe6,   //  xchg rsp, rsi
    0x56,  //  push rsi
})

_DEF_INSTR(call_cleanup, {
    0x5e,               //pop rsi
    0x48, 0x87, 0xe6   //  xchg rsp, rsi
})

_DEF_INSTR2(putchar, emit_callsetup(), {
    int diff = ((char*)this->putc)  - ((char*)code) - address - 0x4;
    __EMIT_32_REV(diff)     // RIP - putc

    __EMIT({
        0x66, 0x89, 0xc7   //  mov di, ax
    })
    emit_call_cleanup();  
} , {
    0xe8    // call
})

_DEF_INSTR2(getchar, emit_callsetup(), {
    int diff = ((char*)this->getc)  - ((char*)code) - address - 0x4;
    __EMIT_32_REV(diff)     // RIP - putc

    __EMIT({
        0x66, 0x89, 0xc7   //  mov di, ax
    })
    emit_call_cleanup();
    emit_catchnone();
    emit_cmpzero();
} , {
    0xe8    // call
})

void brainfuck_jit::open_loop() {
    returns[++top_ret] = address;
    __EMIT({0x0f, 0x84})   // jz skelleton
    address += 4;
}

void brainfuck_jit::close_loop() {
    int ret_addr = returns[top_ret--];
    int diff = ret_addr - address;
    printf("jumpdiff: %i\n", diff);
    *(int32_t*)&code[ret_addr+2] = -diff;


    /*if(diff > -0x80 + 0x2) {    //is shortjump enough??
        __EMIT_8(0x75)      //shortjump skeletton 
        __EMIT_8(diff-0x2); 
    } else */{                    //else do nearjump
        __EMIT_16(0x0f85)
        //diff-=0x6;
        __EMIT_32_REV(diff);
        
    }

}

int brainfuck_jit::get_addr() {
    return address;
}

_DEF_BIN(di_to_ax, {
    0x66, 0x89, 0xf8   // mov ax, di
})


_DEF_BIN(ax_to_di, {
    0x66, 0x89, 0xc7   // mov di, ax
})
void brainfuck_jit::emit_add(uint16_t x) {
    _EMIT_BIN(di_to_ax)
    __EMIT({0x66, 0x05})
    __EMIT_16_REV(x);
    _EMIT_BIN(ax_to_di)
}

void brainfuck_jit::emit_sub(uint16_t x) {
    _EMIT_BIN(di_to_ax)
    __EMIT({0x66, 0x2d})
    __EMIT_16_REV(x);
    _EMIT_BIN(ax_to_di)
}