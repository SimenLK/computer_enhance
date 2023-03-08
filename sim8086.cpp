#include <stdio.h>
#include <stdint.h>
#include <string.h>

#define REGISTER_COUNT 8

#define ArraySize(x) ((sizeof(x)) / (sizeof(x[0])))

char usage[] = {
    "Usage: sim8086 {asm file}"
};

size_t open_file(char *file_name, uint8_t *data)
{
    size_t result = 0;

    FILE *f = fopen(file_name, "r");

    result = fread(data, sizeof(uint8_t), 256, f);

    return result;
}

const uint8_t opcodes[] = {
    0b10001000,     // MOV: register/memory/from register
    0b11000110,     // Immediate to register/memory
    0b10110000,     // Immediate to register
    0b10100000,     // Memory to accumulator
    0b10100010,     // Accumulator to memory
    0b00000000,     // ADD
};

const uint8_t opcode_masks[] = {
    0b11111100,     // MOV: register/memory/from register
    0b11111110,     // Immediate to register/memory
    0b11110000,     // Immediate to register
    0b11111110,     // Memory to accumulator
    0b11111110,     // Accumulator to memory
    0b11111100,     // ADD: Reg/memory with register to either
};

char opcode_symbols[6][4] = {
    { "mov" },
    { "mov" },
    { "mov" },
    { "mov" },
    { "mov" },
    { "add" },
};

struct Register {
    bool is_word;
    char symbol[3];
    uint32_t index;
};

const uint8_t register_codes[REGISTER_COUNT] = {
    0b00000000,     // AX
    0b00000001,     // CX
    0b00000010,     // DX
    0b00000011,     // BX
    0b00000100,     // SP
    0b00000101,     // BP
    0b00000110,     // SI
    0b00000111,     // DI
};

const char byte_register_symbols[REGISTER_COUNT][3] = {
    { "al" },
    { "cl" },
    { "dl" },
    { "bl" },
    { "ah" },
    { "ch" },
    { "dh" },
    { "bh" },
};

const char word_register_symbols[REGISTER_COUNT][3] = {
    { "ax" },
    { "cx" },
    { "dx" },
    { "bx" },
    { "sp" },
    { "bp" },
    { "si" },
    { "di" },
};

uint32_t find_opcode(uint8_t byte)
{
    uint32_t result;
    int32_t opcodes_length = ArraySize(opcodes);

    for (int32_t op_idx = 0;
         op_idx < opcodes_length;
         ++op_idx)
    {
        uint8_t opcode_mask = opcode_masks[op_idx];
        uint8_t instruction_opcode = byte & opcode_mask;
        uint8_t opcode = opcodes[op_idx];
        if (opcode == instruction_opcode)
        {
            result = op_idx;
            break;
        }
    }

    return result;
}

bool read_dst_bit(uint8_t byte)
{
    bool result = (byte & 0b00000010) >> 1;

    return result;
}

bool read_is_word(uint8_t byte)
{
    bool result = (byte & 0b00000001);

    return result;
}

enum Mod {
    MMND = 0b00,
    MM8D = 0b01,
    M16D = 0b10,
    RRND = 0b11,
};

Mod find_mod(uint8_t byte)
{
    Mod result;

    uint8_t mod_bits = (byte & 0b11000000) >> 6;
    result = (Mod)mod_bits;

    return result;
}

Register find_reg(uint8_t byte, bool is_word, uint8_t mask, uint8_t pos)
{
    Register result = {};
    result.is_word = is_word;

    uint8_t operand_register = (byte & mask) >> pos;
    for (int32_t reg_idx = 0;
         reg_idx < REGISTER_COUNT;
         ++reg_idx)
    {
        uint8_t reg = register_codes[reg_idx];
        if (reg == operand_register)
        {
            result.index = reg_idx;
            if (is_word) {
                strcpy(result.symbol, word_register_symbols[reg_idx]);
            } else {
                strcpy(result.symbol, byte_register_symbols[reg_idx]);
            }
            break;
        }
    }

    return result;
}


int main(int argc, char **argv)
{
    if (argc != 2) {
        puts(usage);
        return 1;
    }

    uint8_t instructions[256] = {};
    size_t bytes_read = open_file(argv[1], instructions);
    // TODO(simkir): Find the exact amount of used per instruction
    uint32_t instructions_read = bytes_read / sizeof(uint16_t);

    uint8_t *instr_ptr = instructions;
    for (uint32_t instr_idx = 0;
         instr_idx < instructions_read;
         instr_idx++)
    {
        // Find opcode
        uint8_t opcode_byte = *instr_ptr++;
        uint32_t op_idx = find_opcode(opcode_byte);
        // TODO(simkir): Test based on which opcode

        bool reg_is_dst = read_dst_bit(opcode_byte);
        bool is_word = read_is_word(opcode_byte);

        uint8_t reg_byte = *instr_ptr++;
        // TODO: Find MOD
        Mod mod_field = find_mod(reg_byte);
        // Find REG
        uint8_t reg_mask = (uint8_t)0b00111000;
        Register reg_register = find_reg(reg_byte, is_word, reg_mask, 3);
        // Find R/M
        uint8_t rm_mask = (uint8_t)0b00000111;
        Register rm_register = find_reg(reg_byte, is_word, rm_mask, 0);

        Register dest_register = rm_register;
        Register source_register = reg_register;
        if (reg_is_dst) {
            dest_register = reg_register;
            source_register = rm_register;
        }

        char *opcode_symbol = opcode_symbols[op_idx];
        printf("%s %s, %s\n",
               opcode_symbol,
               dest_register.symbol,
               source_register.symbol);
    }

    return 0;
}
