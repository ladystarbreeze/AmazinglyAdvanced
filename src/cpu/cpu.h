/*
 * AmazinglyAdvanced is a WIP GBA emulator.
 * Copyright (C) 2019  Lady Starbreeze (Michelle-Marie Schiller)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see http://www.gnu.org/licenses/.
 */

#pragma once
#ifndef AMAZINGLY_ADVANCED_CPU_H
#define AMAZINGLY_ADVANCED_CPU_H


#include "cpu_modes.h"
#include "cpu_registers.h"

#include "../utils/log.h"

#include <array>
#include <memory>

class MMU;

class CPU
{
private:
    std::shared_ptr<MMU> mmu;
    std::shared_ptr<spdlog::logger> console;

    CPU_Registers regs;

    std::array<void (CPU::*)(), 2>    state_table;
    std::array<void (CPU::*)(), 4096> arm_table;
    std::array<void (CPU::*)(), 256>  thumb_table;

    uint32_t arm_inst;
    uint32_t arm_op;
    uint16_t thumb_inst;
    uint16_t thumb_op;

    void fill_arm_table();
    void fill_thumb_table();

    [[nodiscard]] inline std::string get_cpsr_flags() const;
    [[nodiscard]] inline std::string get_cpu_mode(CPU_MODE mode) const;
    //[[nodiscard]] inline std::string get_c_code(uint8_t c_code) const;

    [[nodiscard]] inline uint8_t get_index(CPU_MODE mode) const;

    [[nodiscard]] inline uint32_t get_pc() const;
    [[nodiscard]] inline uint32_t get_pc_prefetch() const;
    [[nodiscard]] inline uint32_t get_cpsr() const;
    [[nodiscard]] inline uint32_t get_spsr() const;
    [[nodiscard]] inline uint32_t get_register(uint8_t index) const;
    inline void set_cpsr(uint32_t value, bool privileged);
    inline void set_spsr(uint32_t value);
    inline void set_cpu_state(bool thumb);
    inline void set_register(uint8_t index, uint32_t value);

    inline void set_nz(uint32_t value);
    inline void set_nz_long(uint64_t value);
    inline void set_nzcv_add(uint32_t a, uint32_t b, uint32_t result);
    inline void set_nzcv_sub(uint32_t a, uint32_t b, uint32_t result);
    inline bool is_condition(uint8_t c_code);

    inline void load_register(uint8_t rd, uint32_t address, bool byte);
    inline void store_register(uint8_t rd, uint32_t address, bool byte);

    [[nodiscard]] inline uint32_t read_word(uint32_t address) const;

    inline uint32_t fetch_arm();
    inline uint16_t fetch_thumb();
    inline void decode_arm();
    inline void decode_thumb();

    inline uint32_t barrel_shifter(bool immediate, bool set_c, uint16_t operand, bool dp = false);
    inline uint32_t logical_shift_left(uint32_t value, uint8_t amount, bool set_c, bool imm);
    inline uint32_t logical_shift_right(uint32_t value, uint8_t amount, bool set_c, bool imm);
    inline uint32_t arithmetic_shift_right(uint32_t value, uint8_t amount, bool set_c, bool imm);
    inline uint32_t rotate_right(uint32_t value, uint8_t amount, bool set_c, bool imm);
    [[nodiscard]] inline uint32_t rotate_immediate(uint16_t operand) const;

    inline uint32_t lsl(uint32_t value, uint8_t amount);
    inline uint32_t lsl_set(uint32_t value, uint8_t amount);
    inline uint32_t lsl_imm(uint32_t value, uint8_t amount);
    inline uint32_t lsl_reg(uint32_t value, uint8_t amount);
    inline uint32_t lsl_set_reg(uint32_t value, uint8_t amount);
    inline uint32_t lsl_set_imm(uint32_t value, uint8_t amount);

    inline uint32_t lsr(uint32_t value, uint8_t amount);
    inline uint32_t lsr_set(uint32_t value, uint8_t amount);
    inline uint32_t lsr_imm(uint32_t value, uint8_t amount);
    inline uint32_t lsr_reg(uint32_t value, uint8_t amount);
    inline uint32_t lsr_set_reg(uint32_t value, uint8_t amount);
    inline uint32_t lsr_set_imm(uint32_t value, uint8_t amount);

    inline uint32_t asr(uint32_t value, uint8_t amount);
    inline uint32_t asr_set(uint32_t value, uint8_t amount);
    inline uint32_t asr_imm(uint32_t value, uint8_t amount);
    inline uint32_t asr_reg(uint32_t value, uint8_t amount);
    inline uint32_t asr_set_reg(uint32_t value, uint8_t amount);
    inline uint32_t asr_set_imm(uint32_t value, uint8_t amount);

    inline uint32_t ror(uint32_t value, uint8_t amount);
    inline uint32_t ror_set(uint32_t value, uint8_t amount);
    inline uint32_t ror_imm(uint32_t value, uint8_t amount);
    inline uint32_t ror_reg(uint32_t value, uint8_t amount);
    inline uint32_t ror_set_imm(uint32_t value, uint8_t amount);
    inline uint32_t ror_set_reg(uint32_t value, uint8_t amount);
    inline uint32_t rrx(uint32_t value);
    inline uint32_t rrx_set(uint32_t value);

    inline void undefined_instruction();

    inline void hardware_interrupt();
    inline void software_interrupt();

    inline void arm_block_data_transfer();
    inline void arm_branch();
    inline void arm_branch_and_exchange();
    inline void arm_data_processing();
    inline void arm_halfword_data_transfer();
    inline void arm_multiply();
    inline void arm_multiply_long();
    inline void arm_single_data_transfer();
    inline void arm_single_data_swap();

    inline void adc(uint32_t a, uint32_t b, uint8_t rd, bool set_c);
    inline void add(uint32_t a, uint32_t b, uint8_t rd, bool set_c);
    inline void logical_and(uint32_t a, uint32_t b, uint8_t rd, bool set_c);
    inline void bic(uint32_t a, uint32_t b, uint8_t rd, bool set_c);
    inline void cmn(uint32_t a, uint32_t b);
    inline void cmp(uint32_t a, uint32_t b);
    inline void logical_eor(uint32_t a, uint32_t b, uint8_t rd, bool set_c);
    inline void mov(uint32_t a, uint8_t rd, bool set_c);
    inline void mrs();
    inline void msr();
    inline void logical_or(uint32_t a, uint32_t b, uint8_t rd, bool set_c);
    inline void sbc(uint32_t a, uint32_t b, uint8_t rd, bool set_c);
    inline void sub(uint32_t a, uint32_t b, uint8_t rd, bool set_c);
    inline void teq(uint32_t a, uint32_t b);
    inline void tst(uint32_t a, uint32_t b);

    inline void smlal(uint8_t rd_hi, uint8_t rd_lo, uint8_t rs, uint8_t rm, bool set_c);
    inline void smull(uint8_t rd_hi, uint8_t rd_lo, uint8_t rs, uint8_t rm, bool set_c);
    inline void umlal(uint8_t rd_hi, uint8_t rd_lo, uint8_t rs, uint8_t rm, bool set_c);
    inline void umull(uint8_t rd_hi, uint8_t rd_lo, uint8_t rs, uint8_t rm, bool set_c);

    inline void ldmd(uint8_t rn, uint16_t rlist, bool pre_index, bool user, bool write_back);
    inline void ldmi(uint8_t rn, uint16_t rlist, bool pre_index, bool user, bool write_back);
    inline void stmd(uint8_t rn, uint16_t rlist, bool pre_index, bool user, bool write_back);
    inline void stmi(uint8_t rn, uint16_t rlist, bool pre_index, bool user, bool write_back);

    inline void thumb_add();
    inline void thumb_add_offset_to_sp();
    inline void thumb_alu();
    inline void thumb_conditional_branch();
    inline void thumb_hi_register();
    inline void thumb_load_address();
    inline void thumb_load_halfword();
    inline void thumb_load_immediate_offset();
    inline void thumb_load_register_offset();
    inline void thumb_load_signed_halfword();
    inline void thumb_long_branch();
    inline void thumb_move_immediate();
    inline void thumb_move_shifted_register();
    inline void thumb_multiple_load();
    inline void thumb_pc_relative_load();
    inline void thumb_push_registers();
    inline void thumb_sp_relative_load();
    inline void thumb_unconditional_branch();

    inline void thumb_branch_and_exchange(uint8_t rs);
    inline void thumb_multiply(uint32_t a, uint32_t b, uint8_t rd, bool set_c);
    inline void thumb_ldmia(uint8_t rb, uint8_t rlist);
    inline void thumb_stmia(uint8_t rb, uint8_t rlist);
    inline void thumb_push(uint8_t rlist, bool lr);
    inline void thumb_pop(uint8_t rlist, bool pc);

    inline void dump_registers() const;
public:
    explicit CPU(const std::shared_ptr<MMU> &mmu);
    ~CPU();

    void run();
};


#endif //AMAZINGLY_ADVANCED_CPU_H
