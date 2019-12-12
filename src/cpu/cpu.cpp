#include "cpu.h"

#include "../mmu/mmu.h"

constexpr uint32_t count_bits_set(const uint16_t value)
{
    uint32_t bits_set = 0;

    for (uint16_t i = 0; i < 16; i++)
    {
        if ((value & (1u << i)) != 0)
        {
            ++bits_set;
        }
    }

    return bits_set;
}

constexpr uint32_t first_bit_set(const uint16_t value)
{
    for (uint16_t i = 0; i < 16; i++)
    {
        if ((value & (1u << i)) != 0)
        {
            return i;
        }
    }

    return 16;
}

CPU::CPU(const std::shared_ptr<MMU> &mmu) :
regs(), arm_inst(0), arm_op(0), thumb_inst(0), thumb_op(0)
{
    this->mmu = mmu;
    console = spdlog::stdout_color_mt("ARM7TDMI");

    regs.pc = 0x8000000;
    regs.cpsr.cpu_mode = CPU_MODE::System;
    regs.sp_banked[get_index(CPU_MODE::Supervisor)] = 0x3007FE0;
    regs.sp_banked[get_index(CPU_MODE::IRQ)]    = 0x3007FA0;
    regs.sp_banked[get_index(CPU_MODE::System)] = 0x3007F00;

    state_table[0] = &CPU::decode_arm;
    state_table[1] = &CPU::decode_thumb;

    fill_arm_table();
    fill_thumb_table();

    /*console->info("  r0: {:08X}h   r1: {:08X}h   r2: {:08X}h   r3: {:08X}h",
            get_register(0), get_register(1), get_register(2), get_register(3));
    console->info("  r4: {:08X}h   r5: {:08X}h   r6: {:08X}h   r7: {:08X}h",
            get_register(4), get_register(5), get_register(6), get_register(7));
    console->info("  r8: {:08X}h   r9: {:08X}h  r10: {:08X}h  r11: {:08X}h",
            get_register(8), get_register(9), get_register(10), get_register(11));
    console->info(" r12: {:08X}h  r13: {:08X}h  r14: {:08X}h  r15: {:08X}h",
            get_register(12), get_register(13), get_register(14), get_register(15));
    console->info("cpsr: {:08X}h [{}]",
            get_cpsr(), get_cpsr_flags());*/
}

CPU::~CPU()
= default;

void CPU::fill_arm_table()
{
    for (uint32_t i = 0; i < 4096; i++)
    {
        arm_table[i] = &CPU::undefined_instruction;
    }

    for (uint32_t i = 0; i <= 0x3FF; i++)
    {
        arm_table[i] = &CPU::arm_data_processing;
    }

    arm_table[0x121] = &CPU::arm_branch_and_exchange;

    {
        uint32_t pu = 0;
        uint32_t wl = 0;
        uint32_t sh = 0;

        while (pu < 4)
        {
            uint32_t i = 0b000000001001u | pu << 7u | wl << 4u | sh << 1u;

            arm_table[i] = &CPU::arm_halfword_data_transfer;
            arm_table[i | 0x40u] = &CPU::arm_halfword_data_transfer;

            ++sh;

            if (sh == 4)
            {
                ++wl;
                sh = 0;
            }

            if (wl == 4)
            {
                ++pu;
                wl = 0;
            }
        }
    }

    for (uint16_t i = 0x9; i <= 0x39; i += 0x10)
    {
        arm_table[i] = &CPU::arm_multiply;
    }

    for (uint16_t i = 0x89; i <= 0xF9; i += 0x10)
    {
        arm_table[i] = &CPU::arm_multiply_long;
    }

    arm_table[0x109] = &CPU::arm_single_data_swap;
    arm_table[0x149] = &CPU::arm_single_data_swap;

    for (uint32_t i = 0x400; i <= 0x7FF; i++)
    {
        arm_table[i] = &CPU::arm_single_data_transfer;
    }

    for (uint32_t i = 0x800; i <= 0x9FF; i++)
    {
        arm_table[i] = &CPU::arm_block_data_transfer;
    }

    for (uint32_t i = 0xA00; i <= 0xBFF; i++)
    {
        arm_table[i] = &CPU::arm_branch;
    }

    for (uint32_t i = 0xF00; i <= 0xFFF; i++)
    {
        arm_table[i] = &CPU::software_interrupt;
    }
}

void CPU::fill_thumb_table()
{
    for (uint32_t i = 0; i < 256; i++)
    {
        thumb_table[i] = &CPU::undefined_instruction;
    }

    for (uint32_t i = 0; i <= 0x1F; i++)
    {
        thumb_table[i] = &CPU::thumb_move_shifted_register;
    }

    for (uint32_t i = 0x18; i <= 0x1F; i++)
    {
        thumb_table[i] = &CPU::thumb_add;
    }

    for (uint32_t i = 0x20; i <= 0x3F; i++)
    {
        thumb_table[i] = &CPU::thumb_move_immediate;
    }

    for (uint32_t i = 0x40; i <= 0x43; i++)
    {
        thumb_table[i] = &CPU::thumb_alu;
    }

    for (uint32_t i = 0x44; i <= 0x47; i++)
    {
        thumb_table[i] = &CPU::thumb_hi_register;
    }

    for (uint32_t i = 0x48; i <= 0x4F; i++)
    {
        thumb_table[i] = &CPU::thumb_pc_relative_load;
    }

    {
        uint32_t lb = 0;
        uint32_t a  = 0;

        while (lb < 4)
        {
            uint32_t i = 0b01010000u | (lb << 2u) | a;

            thumb_table[i] = &CPU::thumb_load_register_offset;
            thumb_table[i | 2u] = &CPU::thumb_load_signed_halfword;
            ++a;

            if (a == 2)
            {
                ++lb;
                a = 0;
            }
        }
    }

    for (uint32_t i = 0x60; i <= 0x7F; i++)
    {
        thumb_table[i] = &CPU::thumb_load_immediate_offset;
    }

    for (uint32_t i = 0x80; i <= 0x8F; i++)
    {
        thumb_table[i] = &CPU::thumb_load_halfword;
    }

    for (uint32_t i = 0x90; i <= 0x9F; i++)
    {
        thumb_table[i] = &CPU::thumb_sp_relative_load;
    }

    for (uint32_t i = 0xA0; i <= 0xAF; i++)
    {
        thumb_table[i] = &CPU::thumb_load_address;
    }

    thumb_table[0b10110000] = &CPU::thumb_add_offset_to_sp;

    thumb_table[0b10110100] = &CPU::thumb_push_registers;
    thumb_table[0b10110101] = &CPU::thumb_push_registers;
    thumb_table[0b10111100] = &CPU::thumb_push_registers;
    thumb_table[0b10111101] = &CPU::thumb_push_registers;

    for (uint32_t i = 0xC0; i <= 0xCF; i++)
    {
        thumb_table[i] = &CPU::thumb_multiple_load;
    }

    for (uint32_t i = 0xD0; i <= 0xDF; i++)
    {
        thumb_table[i] = &CPU::thumb_conditional_branch;
    }

    thumb_table[0b11011111] = &CPU::software_interrupt;

    for (uint32_t i = 0xE0; i <= 0xE7; i++)
    {
        thumb_table[i] = &CPU::thumb_unconditional_branch;
    }

    for (uint32_t i = 0xF0; i <= 0xFF; i++)
    {
        thumb_table[i] = &CPU::thumb_long_branch;
    }
}

std::string CPU::get_cpsr_flags() const
{
    std::string flags = "       ";

    flags[0] = (regs.cpsr.negative)    ? 'N' : '-';
    flags[1] = (regs.cpsr.zero)        ? 'Z' : '-';
    flags[2] = (regs.cpsr.carry)       ? 'C' : '-';
    flags[3] = (regs.cpsr.overflow)    ? 'V' : '-';
    flags[4] = (regs.cpsr.irq_disable) ? 'I' : '-';
    flags[5] = (regs.cpsr.fiq_disable) ? 'F' : '-';
    flags[6] = (regs.cpsr.thumb_state) ? 'T' : '-';

    return flags;
}

std::string CPU::get_cpu_mode(const CPU_MODE mode) const
{
    switch (mode)
    {
        case CPU_MODE::User:
            return "User";
        case CPU_MODE::FIQ:
            return "FIQ";
        case CPU_MODE::IRQ:
            return "IRQ";
        case CPU_MODE::Supervisor:
            return "Supervisor";
        case CPU_MODE::Abort:
            return "Abort";
        case CPU_MODE::Undefined:
            return "Undefined";
        case CPU_MODE::System:
            return "System";
    }

    console->error("Invalid CPU mode!");

    throw std::runtime_error("Invalid CPU mode!");
}

/*
std::string CPU::get_c_code(const uint8_t c_code) const
{
    switch (c_code)
    {
        case 0b0000:
            return "eq";
        case 0b0001:
            return "ne";
        case 0b0010:
            return "cs";
        case 0b0011:
            return "cc";
        case 0b0100:
            return "mi";
        case 0b0101:
            return "pl";
        case 0b0110:
            return "vs";
        case 0b0111:
            return "vc";
        case 0b1000:
            return "hi";
        case 0b1001:
            return "ls";
        case 0b1010:
            return "ge";
        case 0b1011:
            return "lt";
        case 0b1100:
            return "gt";
        case 0b1101:
            return "le";
        case 0b1110:
            return "";
    }

    console->error("Invalid/reserved condition!");

    throw std::runtime_error("Invalid/reserved condition!");
}
*/

uint8_t CPU::get_index(const CPU_MODE mode) const
{
    switch (mode)
    {
        case CPU_MODE::User:
        case CPU_MODE::System:
            return 0;
        case CPU_MODE::FIQ:
            return 1;
        case CPU_MODE::IRQ:
            return 2;
        case CPU_MODE::Supervisor:
            return 3;
        case CPU_MODE::Abort:
            return 4;
        case CPU_MODE::Undefined:
            return 5;
    }

    console->error("Invalid register index!");

    throw std::runtime_error("Invalid register index!");
}

uint32_t CPU::get_pc() const
{
    static uint32_t align_table[] = { 0xFFFFFFFD, 0xFFFFFFFE };

    return regs.pc & align_table[regs.cpsr.thumb_state];
}

uint32_t CPU::get_pc_prefetch() const
{
    static uint32_t align_table[] = { 0xFFFFFFFD, 0xFFFFFFFE };

    return (regs.pc + ((regs.cpsr.thumb_state) ? 2u : 4u)) & align_table[regs.cpsr.thumb_state];
}

uint32_t CPU::get_cpsr() const
{
    return regs.cpsr.cpsr;
}

uint32_t CPU::get_spsr() const
{
    if ((CPU_MODE)regs.cpsr.cpu_mode != CPU_MODE::User && (CPU_MODE)regs.cpsr.cpu_mode != CPU_MODE::System)
    {
        return regs.spsr_banked[get_index((CPU_MODE)regs.cpsr.cpu_mode) - 1u].cpsr;
    }

    console->error("SPSR doesn't exist!");

    throw std::runtime_error("SPSR doesn't exist!");
}

uint32_t CPU::get_register(const uint8_t index) const
{
    if (index < 8)
    {
        return regs.r_unbanked[index];
    }
    else if (index < 13)
    {
        return regs.r_banked_fiq[index - 8u][(regs.cpsr.cpu_mode == CPU_MODE::FIQ) ? 1 : 0];
    }
    else if (index == 13)
    {
        return regs.sp_banked[get_index((CPU_MODE)regs.cpsr.cpu_mode)];
    }
    else if (index == 14)
    {
        return regs.lr_banked[get_index((CPU_MODE)regs.cpsr.cpu_mode)];
    }
    else if (index == 15)
    {
        return get_pc_prefetch();
    }

    console->error("Invalid register index: {}", index);

    throw std::runtime_error("Invalid register index!");
}

void CPU::set_cpsr(const uint32_t value, const bool privileged)
{
    uint32_t old_cpsr = regs.cpsr.cpsr;

    if (privileged)
    {
        //console->warn("Privileged CPSR access!");

        regs.cpsr.cpsr = value;

        /*
        if (((old_cpsr & 0x20u) != 0) != regs.cpsr.thumb_state)
        {
            console->error("T bit changed outside of BX instruction!");

            throw std::runtime_error("T bit changed outside of BX instruction!");
        }
        */

        if ((old_cpsr & 0x1Fu) != regs.cpsr.cpu_mode)
        {
            //console->critical("Mode change! New CPU mode: {}", get_cpu_mode((CPU_MODE)regs.cpsr.cpu_mode));
        }
    }
    else
    {
        //console->info("Unprivileged CPSR access");

        regs.cpsr.cpsr = (regs.cpsr.cpsr & 0xFFFFFFFu) | (value & 0xF0000000u);
    }
}

void CPU::set_spsr(const uint32_t value)
{
    //console->warn("SPSR access!");

    if ((CPU_MODE)regs.cpsr.cpu_mode == CPU_MODE::User || (CPU_MODE)regs.cpsr.cpu_mode == CPU_MODE::System)
    {
        console->error("User/System mode don't have SPSRs!");

        return;
    }

    regs.spsr_banked[get_index((CPU_MODE)regs.cpsr.cpu_mode) - 1u].cpsr = value;
}

void CPU::set_cpu_state(const bool thumb)
{
    if (regs.cpsr.thumb_state != thumb)
    {
        //console->critical("State change!");

        regs.cpsr.thumb_state = thumb;

        //console->info("New CPU state: {}", ((thumb) ? "Thumb" : "ARM"));
    }
}

void CPU::set_register(const uint8_t index, const uint32_t value)
{
    if (index < 8)
    {
        regs.r_unbanked[index] = value;
        return;
    }
    else if (index < 13)
    {
        regs.r_banked_fiq[index - 8u][(regs.cpsr.cpu_mode == CPU_MODE::FIQ) ? 1 : 0] = value;
        return;
    }
    else if (index == 13)
    {
        regs.sp_banked[get_index((CPU_MODE)regs.cpsr.cpu_mode)] = value;
        return;
    }
    else if (index == 14)
    {
        regs.lr_banked[get_index((CPU_MODE)regs.cpsr.cpu_mode)] = value;
        return;
    }
    else if (index == 15)
    {
        regs.pc = value;
        return;
    }

    console->error("Invalid register index: {}", index);

    throw std::runtime_error("Invalid register index!");
}

void CPU::set_nz(const uint32_t value)
{
    regs.cpsr.negative = (value & 0x80000000u) != 0;
    regs.cpsr.zero = value == 0;
}

void CPU::set_nz_long(const uint64_t value)
{
    regs.cpsr.negative = (value & 0x8000000000000000u) != 0;
    regs.cpsr.zero = value == 0;
}

void CPU::set_nzcv_add(const uint32_t a, const uint32_t b, const uint32_t result)
{
    set_nz(result);

    regs.cpsr.carry = (0xFFFFFFFFu - a) < b;
    regs.cpsr.overflow = ((a ^ b) & 0x80000000u) == 0 && ((a ^ result) & 0x80000000u) != 0;
}

void CPU::set_nzcv_sub(const uint32_t a, const uint32_t b, const uint32_t result)
{
    set_nz(result);

    regs.cpsr.carry = a >= b;
    regs.cpsr.overflow = ((a ^ b) & 0x80000000u) != 0 && ((a ^ result) & 0x80000000u) != 0;
}

bool CPU::is_condition(const uint8_t c_code)
{
    switch (c_code)
    {
        case 0b0000:
            return regs.cpsr.zero;
        case 0b0001:
            return !regs.cpsr.zero;
        case 0b0010:
            return regs.cpsr.carry;
        case 0b0011:
            return !regs.cpsr.carry;
        case 0b0100:
            return regs.cpsr.negative;
        case 0b0101:
            return !regs.cpsr.negative;
        case 0b0110:
            return regs.cpsr.overflow;
        case 0b0111:
            return !regs.cpsr.overflow;
        case 0b1000:
            return regs.cpsr.carry && !regs.cpsr.zero;
        case 0b1001:
            return !regs.cpsr.carry || regs.cpsr.zero;
        case 0b1010:
            return (regs.cpsr.negative && regs.cpsr.overflow) || (!regs.cpsr.negative && !regs.cpsr.overflow);
        case 0b1011:
            return (regs.cpsr.negative && !regs.cpsr.overflow) || (!regs.cpsr.negative && regs.cpsr.overflow);
        case 0b1100:
            return !regs.cpsr.zero && ((regs.cpsr.negative && regs.cpsr.overflow) || (!regs.cpsr.negative && !regs.cpsr.overflow));
        case 0b1101:
            return regs.cpsr.zero || (regs.cpsr.negative && !regs.cpsr.overflow) || (!regs.cpsr.negative && regs.cpsr.overflow);
        case 0b1110:
            return true;
        case 0b1111:
        default:
            console->error("Invalid/reserved condition!");

            throw std::runtime_error("Invalid/reserved condition!");
    }
}

void CPU::load_register(const uint8_t rd, const uint32_t address, const bool byte)
{
    if (byte)
    {
        set_register(rd, mmu->read8(address));

        //console->info("ldrb r{}, ${:08X}", rd, address);
    }
    else
    {
        set_register(rd, read_word(address));

        //console->info("ldr r{}, ${:08X}", rd, address);
    }
}

void CPU::store_register(const uint8_t rd, const uint32_t address, const bool byte)
{
    if (byte)
    {
        //console->info("strb r{}, ${:08X}", rd, address);

        mmu->write8(get_register(rd) + ((rd == 15) ? 4u : 0), address);
    }
    else
    {
        //console->info("str r{}, ${:08X}", rd, address);

        mmu->write32(get_register(rd) + ((rd == 15) ? 4u : 0), address);
    }
}

uint32_t CPU::read_word(const uint32_t address) const
{
    uint8_t offset = (address % 4u) * 8u;
    uint32_t word  = mmu->read32(address & 0xFFFFFFFCu);

    return (word >> offset) | (word << (32u - offset));
}

uint32_t CPU::fetch_arm()
{
    uint32_t instruction = mmu->read32(get_pc());

    regs.pc += 4u;

    return instruction;
}

uint16_t CPU::fetch_thumb()
{
    uint16_t instruction = mmu->read16(get_pc());

    regs.pc += 2u;

    return instruction;
}

void CPU::decode_arm()
{
    arm_inst = fetch_arm();
    arm_op   = ((arm_inst >> 4u) & 0xFu) | ((arm_inst >> 16u) & 0xFF0u);

    if (!is_condition(arm_inst >> 28u))
    {
        return;
    }

    (this->*arm_table[arm_op])();
}

void CPU::decode_thumb()
{
    thumb_inst = fetch_thumb();
    thumb_op   = thumb_inst >> 8u;

    (this->*thumb_table[thumb_op])();
}

uint32_t CPU::barrel_shifter(const bool immediate, const bool set_c, const uint16_t operand, const bool dp)
{
    uint8_t rm     = operand & 0xFu;
    uint32_t value = get_register(rm);
    uint8_t amount = ((immediate) ? (operand >> 7u) : get_register(operand >> 8u));
    auto mode      = (uint8_t)((operand >> 5u) & 3u);

    if (dp && (rm == 15 && !immediate))
    {
        value += 4u;
    }

    switch (mode)
    {
        case 0b00:
            return logical_shift_left(value, amount, set_c, immediate);
        case 0b01:
            return logical_shift_right(value, amount, set_c, immediate);
        case 0b10:
            return arithmetic_shift_right(value, amount, set_c, immediate);
        case 0b11:
            return rotate_right(value, amount, set_c, immediate);
        default:
            return 0xC0FFEE;
    }
}

uint32_t CPU::logical_shift_left(const uint32_t value, const uint8_t amount, const bool set_c, const bool imm)
{
    uint8_t is_set_imm = (uint8_t)(set_c) << 1u | (uint8_t)imm;

    switch (is_set_imm)
    {
        case 0b00:
            return lsl_reg(value, amount);
        case 0b01:
            return lsl_imm(value, amount);
        case 0b10:
            return lsl_set_reg(value, amount);
        case 0b11:
            return lsl_set_imm(value, amount);
        default:
            return 0xC0FFEE;
    }
}

uint32_t CPU::logical_shift_right(const uint32_t value, const uint8_t amount, const bool set_c, const bool imm)
{
    uint8_t is_set_imm = (uint8_t)(set_c) << 1u | (uint8_t)imm;

    switch (is_set_imm)
    {
        case 0b00:
            return lsr_reg(value, amount);
        case 0b01:
            return lsr_imm(value, amount);
        case 0b10:
            return lsr_set_reg(value, amount);
        case 0b11:
            return lsr_set_imm(value, amount);
        default:
            return 0xC0FFEE;
    }
}

uint32_t CPU::arithmetic_shift_right(const uint32_t value, const uint8_t amount, const bool set_c, const bool imm)
{
    uint8_t is_set_imm = (uint8_t)(set_c) << 1u | (uint8_t)imm;

    switch (is_set_imm)
    {
        case 0b00:
            return asr_reg(value, amount);
        case 0b01:
            return asr_imm(value, amount);
        case 0b10:
            return asr_set_reg(value, amount);
        case 0b11:
            return asr_set_imm(value, amount);
        default:
            return 0xC0FFEE;
    }
}

uint32_t CPU::rotate_right(const uint32_t value, const uint8_t amount, const bool set_c,const bool imm)
{
    uint8_t is_set_imm = (uint8_t)(set_c) << 1u | (uint8_t)imm;

    switch (is_set_imm)
    {
        case 0b00:
            return ror_reg(value, amount);
        case 0b01:
            return ror_imm(value, amount);
        case 0b10:
            return ror_set_reg(value, amount);
        case 0b11:
            return ror_set_imm(value, amount);
        default:
            return 0xC0FFEE;
    }
}

uint32_t CPU::rotate_immediate(const uint16_t operand) const
{
    uint32_t imm = (operand & 0xFFu);
    uint8_t amount = (((uint8_t)(operand >> 8u) & 0xFu) << 1u) & 0x1Fu;

    if (amount == 0)
    {
        return imm;
    }

    return (imm >> amount) | (imm << (32u - amount));
}

uint32_t CPU::lsl(const uint32_t value, const uint8_t amount)
{
    return value << amount;
}

uint32_t CPU::lsl_set(const uint32_t value, const uint8_t amount)
{
    regs.cpsr.carry = (value & (1u << (32u - amount))) != 0;

    return lsl(value, amount);
}

uint32_t CPU::lsl_imm(const uint32_t value, const uint8_t amount)
{
    return lsl(value, amount);
}

uint32_t CPU::lsl_reg(const uint32_t value, const uint8_t amount)
{
    if (amount < 32)
    {
        return lsl(value, amount);
    }

    return 0;
}

uint32_t CPU::lsl_set_imm(const uint32_t value, const uint8_t amount)
{
    if (amount == 0)
    {
        return value;
    }

    return lsl_set(value, amount);
}

uint32_t CPU::lsl_set_reg(const uint32_t value, const uint8_t amount)
{
    if (amount == 0)
    {
        return value;
    }
    else if (amount < 32)
    {
        return lsl_set(value, amount);
    }

    regs.cpsr.carry = amount == 32 && (value & 1u) != 0;

    return 0;
}

uint32_t CPU::lsr(const uint32_t value, const uint8_t amount)
{
    return value >> amount;
}

uint32_t CPU::lsr_set(const uint32_t value, const uint8_t amount)
{
    regs.cpsr.carry = (value & (1u << (amount - 1u))) != 0;

    return lsr(value, amount);
}

uint32_t CPU::lsr_imm(const uint32_t value, const uint8_t amount)
{
    if (amount == 0)
    {
        return 0;
    }

    return lsr(value, amount);
}

uint32_t CPU::lsr_reg(const uint32_t value, const uint8_t amount)
{
    if (amount < 32)
    {
        return lsr(value, amount);
    }

    return 0;
}

uint32_t CPU::lsr_set_imm(const uint32_t value, const uint8_t amount)
{
    if (amount == 0)
    {
        regs.cpsr.carry = ((value >> 31u) & 1u) != 0;

        return 0;
    }

    return lsr_set(value, amount);
}

uint32_t CPU::lsr_set_reg(const uint32_t value, const uint8_t amount)
{
    if (amount == 0)
    {
        return value;
    }
    else if (amount < 32)
    {
        return lsr_set(value, amount);
    }

    regs.cpsr.carry = amount == 32 && ((value >> 31u) & 1u) != 0;

    return 0;
}

uint32_t CPU::asr(const uint32_t value, const uint8_t amount)
{
    return (int32_t)value >> (int8_t)amount;
}

uint32_t CPU::asr_set(const uint32_t value, const uint8_t amount)
{
    regs.cpsr.carry = (value & (1u << (amount - 1u))) != 0;

    return asr(value, amount);
}

uint32_t CPU::asr_imm(const uint32_t value, const uint8_t amount)
{
    if (amount == 0)
    {
        return asr(value, 31);
    }

    return asr(value, amount);
}

uint32_t CPU::asr_reg(const uint32_t value, const uint8_t amount)
{
    if (amount < 32)
    {
        return asr(value, amount);
    }

    return asr(value, 31);
}

uint32_t CPU::asr_set_imm(const uint32_t value, const uint8_t amount)
{
    if (amount == 0)
    {
        regs.cpsr.carry = ((value >> 31u) & 1u) != 0;

        return asr(value, 31);
    }

    return asr_set(value, amount);
}

uint32_t CPU::asr_set_reg(const uint32_t value, const uint8_t amount)
{
    if (amount == 0)
    {
        return value;
    }
    else if (amount < 32)
    {
        return asr_set(value, amount);
    }

    regs.cpsr.carry = ((value >> 31u) & 1u) != 0;

    return asr(value, 31);
}

uint32_t CPU::ror(const uint32_t value, const uint8_t amount)
{
    return (value >> amount) | (value << (32u - amount));
}

uint32_t CPU::ror_set(const uint32_t value, const uint8_t amount)
{
    regs.cpsr.carry = (value & (1u << (amount - 1u))) != 0;

    return ror(value, amount);
}

uint32_t CPU::ror_imm(const uint32_t value, const uint8_t amount)
{
    if (amount == 0)
    {
        return rrx(value);
    }

    return ror(value, amount);
}

uint32_t CPU::ror_reg(const uint32_t value, const uint8_t amount)
{
    return ror(value, amount & 0x1Fu);
}

uint32_t CPU::ror_set_imm(const uint32_t value, const uint8_t amount)
{
    if (amount == 0)
    {
        return rrx_set(value);
    }

    return ror_set(value, amount);
}

uint32_t CPU::ror_set_reg(const uint32_t value, const uint8_t amount)
{
    if (amount == 0)
    {
        return value;
    }

    regs.cpsr.carry = (value & (1u << ((amount - 1u) & 0x1Fu))) != 0;

    return ror(value, amount & 0x1Fu);
}

uint32_t CPU::rrx(const uint32_t value)
{
    return (value >> 1u) | ((uint32_t)(regs.cpsr.carry) << 31u);
}

uint32_t CPU::rrx_set(const uint32_t value)
{
    uint32_t result = rrx(value);

    regs.cpsr.carry = (value & 1u) != 0;

    return result;
}

void CPU::undefined_instruction()
{
    console->critical("Undefined instruction!");
    console->error("ARM instruction: {:08X}h, ARM opcode: {:03X}h", arm_inst, arm_op);
    console->error("Thumb instruction: {:04X}h, Thumb opcode: {:02X}h", thumb_inst, thumb_op);

    regs.lr_banked[get_index(CPU_MODE::Undefined)] = get_pc();
    regs.spsr_banked[get_index(CPU_MODE::Undefined) - 1u] = regs.cpsr;
    set_cpsr((regs.cpsr.cpsr & 0xFFFFFF00u) | 0b10011011u, true);
    regs.pc = 4;

    //dump_registers();

    //throw std::runtime_error("Undefined instruction!");
}

void CPU::hardware_interrupt()
{
    console->info("Hardware interrupt");

    regs.lr_banked[get_index(CPU_MODE::IRQ)] = get_pc();
    regs.spsr_banked[get_index(CPU_MODE::IRQ) - 1u] = regs.cpsr;
    set_cpsr((regs.cpsr.cpsr & 0xFFFFFF00u) | 0b10010010u, true);
    regs.pc = 0x18;
}

void CPU::software_interrupt()
{
    console->info("Software interrupt SWI ${:0X}",
            ((regs.cpsr.thumb_state) ? thumb_inst & 0xFFu : arm_inst & 0xFFFFFFu));

    regs.lr_banked[get_index(CPU_MODE::Supervisor)] = get_pc();
    regs.spsr_banked[get_index(CPU_MODE::Supervisor) - 1u].cpsr = get_cpsr();
    set_cpsr((regs.cpsr.cpsr & 0xFFFFFF00u) | 0b11010011u, true);
    regs.pc = 8;
}

void CPU::arm_block_data_transfer()
{
    bool pre_index = ((arm_inst >> 24u) & 1u) != 0;
    bool up    = ((arm_inst >> 23u) & 1u) != 0;
    bool user  = ((arm_inst >> 22u) & 1u) != 0;
    bool write_back = ((arm_inst >> 21u) & 1u) != 0;
    bool load  = ((arm_inst >> 20u) & 1u) != 0;
    uint8_t rn = (arm_inst >> 16u) & 0xFu;
    uint16_t rlist = arm_inst & 0xFFFFu;

    switch ((uint8_t)up << 1u | (uint8_t)load)
    {
        case 0b00:
            stmd(rn, rlist, !pre_index, user, write_back);
            break;
        case 0b01:
            ldmd(rn, rlist, !pre_index, user, write_back);
            break;
        case 0b10:
            stmi(rn, rlist, pre_index, user, write_back);
            break;
        case 0b11:
            ldmi(rn, rlist, pre_index, user, write_back);
            break;
    }
}

void CPU::arm_branch()
{
    bool link = ((arm_inst >> 24u) & 1u) != 0;
    uint32_t offset = (arm_inst & 0xFFFFFFu) << 2u;

    if ((offset & 0x3000000u) != 0)
    {
        offset |= 0xFC000000u;
    }

    if (link)
    {
        set_register(14, get_pc());
    }

    set_register(15, get_pc_prefetch() + offset);

    //console->info("b{} ${:08X}", (link) ? "l" : "", get_pc());
}

void CPU::arm_branch_and_exchange()
{
    uint8_t rs = arm_inst & 0xFu;
    uint32_t target_addr = get_register(rs);

    set_register(15, target_addr);
    set_cpu_state((target_addr & 1u) != 0);

    //console->info("bx r{}", rs);
}

void CPU::arm_data_processing()
{
    bool imm_op   = ((arm_inst >> 25u) & 1u) != 0;
    uint8_t op    = (arm_inst >> 21u) & 0xFu;
    bool set_c    = ((arm_inst >> 20u) & 1u) != 0;
    uint8_t rn    = (arm_inst >> 16u) & 0xFu;
    uint8_t rd    = (arm_inst >> 12u) & 0xFu;
    uint32_t op_1 = get_register(rn);
    uint32_t op_2;

    if (imm_op)
    {
        op_2 = rotate_immediate(arm_inst & 0xFFFu);
    }
    else
    {
        op_2 = barrel_shifter((arm_inst & 0x10u) == 0, set_c, arm_inst & 0xFFFu, true);
    }

    switch (op)
    {
        case 0b0000:
            logical_and(op_1, op_2, rd, set_c);
            break;
        case 0b0001:
            logical_eor(op_1, op_2, rd, set_c);
            break;
        case 0b0010:
            sub(op_1, op_2, rd, set_c);
            break;
        case 0b0011:
            sub(op_2, op_1, rd, set_c);
            break;
        case 0b0100:
            add(op_1, op_2, rd, set_c);
            break;
        case 0b0101:
            adc(op_1, op_2, rd, set_c);
            break;
        case 0b0110:
            sbc(op_1, op_2, rd, set_c);
            break;
        case 0b0111:
            sbc(op_2, op_1, rd, set_c);
            break;
        case 0b1000:
            if (set_c)
            {
                tst(op_1, op_2);
            }
            else
            {
                mrs();
            }
            break;
        case 0b1001:
            if (set_c)
            {
                teq(op_1, op_2);
            }
            else
            {
                msr();
            }
            break;
        case 0b1010:
            if (set_c)
            {
                cmp(op_1, op_2);
            }
            else
            {
                mrs();
            }
            break;
        case 0b1011:
            if (set_c)
            {
                cmn(op_1, op_2);
            }
            else
            {
                msr();
            }
            break;
        case 0b1100:
            logical_or(op_1, op_2, rd, set_c);
            break;
        case 0b1101:
            mov(op_2, rd, set_c);
            break;
        case 0b1110:
            bic(op_1, op_2, rd, set_c);
            break;
        case 0b1111:
            mov(~op_2, rd, set_c);
            break;
        default:
            break;
    }

    if (set_c && rd == 15)
    {
        console->critical("ALU mode change");

        set_cpsr(get_spsr(), true);
    }
}

void CPU::arm_halfword_data_transfer()
{
    bool pre_index = ((arm_inst >> 24u) & 1u) != 0;
    bool up   = ((arm_inst >> 23u) & 1u) != 0;
    bool imm  = ((arm_inst >> 22u) & 1u) != 0;
    bool write_back = ((arm_inst >> 21u) & 1u) != 0;
    bool load = ((arm_inst >> 20u) & 1u) != 0;
    uint8_t rn = (arm_inst >> 16u) & 0xFu;
    uint8_t rd = (arm_inst >> 12u) & 0xFu;
    uint32_t offset = ((imm) ? ((arm_inst >> 4u) & 0xF0u) | (arm_inst & 0xFu) : get_register(arm_inst & 0xFu));
    uint8_t is_signed_halfword = (arm_inst >> 5u) & 3u;
    uint32_t base = get_register(rn);

    if (pre_index)
    {
        (up) ? base += offset : base -= offset;
    }

    switch (is_signed_halfword)
    {
        case 0b00:
            console->error("Single data swap!");

            throw std::runtime_error("Single data swap!");
        case 0b01:
            if (load)
            {
                set_register(rd, mmu->read16(base));
            }
            else
            {
                mmu->write16(get_register(rd), base);
            }
            break;
        case 0b10:
            if (load)
            {
                set_register(rd, (int32_t)(int8_t)mmu->read8(base));
            }
            else
            {
                undefined_instruction();
            }
            break;
        case 0b11:
            if (load)
            {
                set_register(rd, (int32_t)(int16_t)mmu->read16(base));
            }
            else
            {
                undefined_instruction();
            }
            break;
        default:
            break;
    }

    if (rn != rd && (write_back || !pre_index))
    {
        if (!pre_index)
        {
            set_register(rn, (up) ? base + offset : base - offset);
        }
        else
        {
            set_register(rn, base);
        }
    }
}

void CPU::arm_multiply()
{
    bool acc   = ((arm_inst >> 21u) & 1u);
    bool set_c = ((arm_inst >> 20u) & 1u);
    uint8_t rd = ((arm_inst >> 16u) & 0xfu);
    uint8_t rn = ((arm_inst >> 12u) & 0xfu);
    uint8_t rs = ((arm_inst >> 8u) & 0xfu);
    uint8_t rm = (arm_inst & 0xfu);
    uint32_t result = (acc) ? get_register(rm) * get_register(rs) + get_register(rn) :
                      get_register(rm) * get_register(rs);

    set_register(rd, result);

    if (set_c)
    {
        set_nz(result);
    }

    // console->info("mul{} r{}, r{}, r{}, {}{}", ((acc) ? "a" : ""), rd, rm, rd, ((acc) ? "r" : ""), ((acc) ? rn : ""));
}

void CPU::arm_multiply_long()
{
    uint8_t is_signed_accumulate = ((arm_inst >> 22u) & 1u) << 1u | ((arm_inst >> 21u) & 1u);
    bool set_c = ((arm_inst >> 20u) & 1u) != 0;
    uint8_t rd_hi = (arm_inst >> 16u) & 0xFu;
    uint8_t rd_lo = (arm_inst >> 12u) & 0xFu;
    uint8_t rs = (arm_inst >> 8u) & 0xFu;
    uint8_t rm = arm_inst & 0xFu;

    switch (is_signed_accumulate)
    {
        case 0b00:
            umull(rd_hi, rd_lo, rs, rm, set_c);
            break;
        case 0b01:
            umlal(rd_hi, rd_lo, rs, rm, set_c);
            break;
        case 0b10:
            smull(rd_hi, rd_lo, rs, rm, set_c);
            break;
        case 0b11:
            smlal(rd_hi, rd_lo, rs, rm, set_c);
            break;
        default:
            break;
    }
}

void CPU::arm_single_data_transfer()
{
    bool reg_offset = ((arm_inst >> 25u) & 1u) != 0;
    bool pre_index  = ((arm_inst >> 24u) & 1u) != 0;
    bool up   = ((arm_inst >> 23u) & 1u) != 0;
    bool byte = ((arm_inst >> 22u) & 1u) != 0;
    bool write_back = ((arm_inst >> 21u) & 1u) != 0;
    bool load = ((arm_inst >> 20u) & 1u) != 0;
    uint8_t rn = (arm_inst >> 16u) & 0xFu;
    uint8_t rd = (arm_inst >> 12u) & 0xFu;
    uint32_t offset;
    uint32_t base = get_register(rn);

    if (reg_offset)
    {
        offset = barrel_shifter(true, false, arm_inst & 0xfffu);
    }
    else
    {
        offset = arm_inst & 0xFFFu;
    }

    if (pre_index)
    {
        (up) ? base += offset : base -= offset;
    }

    (load) ? load_register(rd, base, byte) : store_register(rd, base, byte);

    if (rn != rd && (write_back || !pre_index))
    {
        if (!pre_index)
        {
            set_register(rn, (up) ? base + offset : base - offset);
        }
        else
        {
            set_register(rn, base);
        }
    }
}

void CPU::arm_single_data_swap()
{
    bool byte  = ((arm_inst >> 22u) & 1u) != 0;
    uint8_t rn = (arm_inst >> 16u) & 0xFu;
    uint8_t rd = (arm_inst >> 12u) & 0xFu;
    uint8_t rm = arm_inst & 0xFu;
    uint32_t source = get_register(rm);
    uint32_t base   = get_register(rn);

    if (byte)
    {
        set_register(rd, mmu->read8(base));
        mmu->write8(source, base);
    }
    else
    {
        set_register(rd, mmu->read32(base));
        mmu->write32(source, base);
    }

    //console->info("swp{} r{}, r{}, ${:08X}", ((byte) ? "byte" : ""), rd, rm, base);
}

void CPU::adc(const uint32_t a, const uint32_t b, const uint8_t rd, const bool set_c)
{
    uint32_t result = a + b + (uint32_t)regs.cpsr.carry;

    set_register(rd, result);

    if (set_c)
    {
        set_nzcv_add(a, b + (uint32_t)regs.cpsr.carry, result);
    }

    //console->info("adc r{}, #{}, #{}", rd, a, b);
}

void CPU::add(const uint32_t a, const uint32_t b, const uint8_t rd, const bool set_c)
{
    uint32_t result = a + b;

    set_register(rd, result);

    if (set_c)
    {
        set_nzcv_add(a, b, result);
    }

    //console->info("add r{}, #{}, #{}", rd, a, b);
}

void CPU::logical_and(const uint32_t a, const uint32_t b, const uint8_t rd, const bool set_c)
{
    uint32_t result = a & b;

    set_register(rd, result);

    if (set_c)
    {
        set_nz(result);
    }

    //console->info("and r{}, #{}, #{}", rd, a, b);
}

void CPU::bic(const uint32_t a, const uint32_t b, const uint8_t rd, const bool set_c)
{
    uint32_t result = a & ~b;

    set_register(rd, result);

    if (set_c)
    {
        set_nz(result);
    }

    //console->info("bic r{}, #{}, #{}", rd, a, b);
}

void CPU::cmn(const uint32_t a, const uint32_t b)
{
    uint32_t result = a + b;

    set_nzcv_add(a, b, result);

    //console->info("cmn #{}, #{}", a, b);
}

void CPU::cmp(const uint32_t a, const uint32_t b)
{
    uint32_t result = a - b;

    set_nzcv_sub(a, b, result);

    //console->info("cmp #{}, #{}", a, b);
}

void CPU::logical_eor(const uint32_t a, const uint32_t b, const uint8_t rd, const bool set_c)
{
    uint32_t result = a ^ b;

    set_register(rd, result);

    if (set_c)
    {
        set_nz(result);
    }

    //console->info("eor r{}, #{}, #{}", rd, a, b);
}

void CPU::mov(const uint32_t a, const uint8_t rd, const bool set_c)
{
    set_register(rd, a);

    if (set_c)
    {
        set_nz(a);
    }

    //console->info("mov r{}, #{}", rd, a);
}

void CPU::mrs()
{
    bool spsr  = ((arm_inst >> 22u) & 1u) != 0;
    uint8_t rd = (arm_inst >> 12u) & 0xFu;

    if (spsr)
    {
        set_register(rd, get_spsr());
    }
    else
    {
        set_register(rd, get_cpsr());
    }
}

void CPU::msr()
{
    bool imm  = ((arm_inst >> 25u) & 1u) != 0;
    bool spsr = ((arm_inst >> 22u) & 1u) != 0;
    bool privileged = ((arm_inst >> 16u) & 1u) != 0;
    uint32_t operand = ((imm) ? rotate_immediate(arm_inst & 0xFFFu) : get_register(arm_inst & 0xFu));

    (spsr) ? set_spsr(operand) : set_cpsr(operand, privileged && ((CPU_MODE)regs.cpsr.cpu_mode != CPU_MODE::User));

    //console->info("msr {}{}, #{}", ((spsr) ? "spsr" : "cpsr"), ((privileged) ? "" : "_cf"), operand);
}

void CPU::logical_or(const uint32_t a, const uint32_t b, const uint8_t rd, const bool set_c)
{
    uint32_t result = a | b;

    set_register(rd, result);

    if (set_c)
    {
        set_nz(result);
    }

    //console->info("orr r{}, #{}, #{}", rd, a, b);
}

void CPU::sbc(const uint32_t a, const uint32_t b, const uint8_t rd, const bool change_flags)
{
    uint32_t result = a - b + (uint32_t)regs.cpsr.carry - 1u;

    set_register(rd, result);

    if (change_flags)
    {
        set_nzcv_sub(a, b + (uint32_t)regs.cpsr.carry - 1u, result);
    }

    //console->info("sbc r{}, #{}, #{}", rd, a, b);
}

void CPU::sub(const uint32_t a, const uint32_t b, const uint8_t rd, const bool set_c)
{
    uint32_t result = a - b;

    set_register(rd, result);

    if (set_c)
    {
        set_nzcv_sub(a, b, result);
    }

    //console->info("sub r{}, #{}, #{}", rd, a, b);
}

void CPU::teq(const uint32_t a, const uint32_t b)
{
    uint32_t result = a ^ b;

    set_nz(result);

    //console->info("teq #{}, #{}", a, b);
}

void CPU::tst(const uint32_t a, const uint32_t b)
{
    uint32_t result = a & b;

    set_nz(result);

    //console->info("tst #{}, #{}", a, b);
}

void CPU::smlal(const uint8_t rd_hi, const uint8_t rd_lo, const uint8_t rs, const uint8_t rm, const bool set_c)
{
    int64_t a = (int32_t)get_register(rs);
    int64_t b = (int32_t)get_register(rm);
    int64_t c = (int64_t)get_register(rd_hi) << 32;
    int64_t d = (int64_t)get_register(rd_lo);
    int64_t result = a * b + (c | d);

    set_register(rd_hi, result >> 32);
    set_register(rd_lo, result);

    if (set_c)
    {
        set_nz_long(result);
    }

    //console->info("smlal");
}

void CPU::smull(const uint8_t rd_hi, const uint8_t rd_lo, const uint8_t rs, const uint8_t rm, const bool set_c)
{
    int64_t a = (int32_t)get_register(rs);
    int64_t b = (int32_t)get_register(rm);
    int64_t result = a * b;

    set_register(rd_hi, result >> 32);
    set_register(rd_lo, result);

    if (set_c)
    {
        set_nz_long(result);
    }

    //console->info("smull");
}

void CPU::umlal(const uint8_t rd_hi, const uint8_t rd_lo, const uint8_t rs, const uint8_t rm, const bool set_c)
{
    uint64_t a = get_register(rs);
    uint64_t b = get_register(rm);
    uint64_t c = get_register(rd_hi);
    uint64_t d = get_register(rd_lo);
    uint64_t result = a * b + (c << 32u | d);

    set_register(rd_hi, result >> 32u);
    set_register(rd_lo, result);

    if (set_c)
    {
        set_nz_long(result);
    }

    //console->info("umlal");
}

void CPU::umull(const uint8_t rd_hi, const uint8_t rd_lo, const uint8_t rs, const uint8_t rm, const bool set_c)
{
    uint64_t a = get_register(rs);
    uint64_t b = get_register(rm);
    uint64_t result = a * b;

    set_register(rd_hi, result >> 32u);
    set_register(rd_lo, result);

    if (set_c)
    {
        set_nz_long(result);
    }

    //console->info("umull");
}

void CPU::ldmd(const uint8_t rn, const uint16_t rlist, const bool pre_index, const bool user, const bool write_back)
{
    bool r15_in_list  = (rlist & 0x8000u) != 0;
    uint32_t base = get_register(rn) - count_bits_set(rlist) * 4u;
    uint32_t new_base = base;
    CPU_MODE old_mode = (CPU_MODE)regs.cpsr.cpu_mode;

    if (rlist == 0)
    {
        if (pre_index)
        {
            base -= 0x40u;
        }

        set_register(15, mmu->read32(base));

        new_base -= 0x40u;
    }
    else
    {
        if (user && !r15_in_list)
        {
            regs.cpsr.cpu_mode = CPU_MODE::User;
        }

        for (uint16_t i = 0; i < 16; i++)
        {
            if ((rlist & (1u << i)) != 0)
            {
                if (pre_index)
                {
                    base += 4u;
                }

                set_register(i, mmu->read32(base));

                if (!pre_index)
                {
                    base += 4u;
                }
            }
        }

        if (user && r15_in_list)
        {
            set_cpsr(get_spsr(), true);
        }

        if (user && !r15_in_list)
        {
            regs.cpsr.cpu_mode = old_mode;
        }
    }

    if (write_back && (rlist & (1u << rn)) == 0)
    {
        set_register(rn, new_base);
    }

    //console->info("ldmd{} r{}{}, {:016b}{}", ((!pre_index) ? "b" : "a"), rn, ((write_back) ? "!" : ""),
    //        rlist, ((user) ? "^" : ""));
}

void CPU::ldmi(const uint8_t rn, const uint16_t rlist, const bool pre_index, const bool user, const bool write_back)
{
    bool r15_in_list  = (rlist & 0x8000u) != 0;
    uint32_t base     = get_register(rn);
    uint32_t new_base = base + count_bits_set(rlist) * 4u;
    CPU_MODE old_mode = (CPU_MODE)regs.cpsr.cpu_mode;

    static uint64_t c;

    if (rlist == 0)
    {
        if (pre_index)
        {
            base += 0x40u;
        }

        set_register(15, mmu->read32(base));

        new_base += 0x40u;
    }
    else
    {
        if (user && !r15_in_list)
        {
            regs.cpsr.cpu_mode = CPU_MODE::User;
        }

        for (uint16_t i = 0; i < 16; i++)
        {
            if ((rlist & (1u << i)) != 0)
            {
                if (pre_index)
                {
                    base += 4u;
                }

                set_register(i, mmu->read32(base));

                if (!pre_index)
                {
                    base += 4u;
                }
            }
        }

        if (user && r15_in_list)
        {
            set_cpsr(get_spsr(), true);
        }

        if (user && !r15_in_list)
        {
            regs.cpsr.cpu_mode = old_mode;
        }
    }

    if (write_back && (rlist & (1u << rn)) == 0)
    {
        set_register(rn, new_base);
    }

    ++c;

    //console->info("ldmi{} r{}{}, {:016b}{}", ((pre_index) ? "b" : "a"), rn, ((write_back) ? "!" : ""),
    //        rlist, ((user) ? "^" : ""));
}

void CPU::stmd(const uint8_t rn, const uint16_t rlist, const bool pre_index, const bool user, const bool write_back)
{
    uint32_t base = get_register(rn) - count_bits_set(rlist) * 4u;
    uint32_t new_base = base;
    CPU_MODE old_mode = (CPU_MODE)regs.cpsr.cpu_mode;

    if (rlist == 0)
    {
        if (pre_index)
        {
            base -= 0x40u;
        }

        mmu->write32(get_pc_prefetch(), base);

        new_base -= base;
    }
    else
    {
        if (user)
        {
            regs.cpsr.cpu_mode = CPU_MODE::User;
        }

        uint32_t first_in_list = first_bit_set(rlist);

        for (uint16_t i = 0; i < 16; i++)
        {
            if ((rlist & (1u << i)) != 0)
            {
                if (pre_index)
                {
                    base += 4u;
                }

                if (i == rn && rn != first_in_list)
                {
                    mmu->write32(new_base, base);
                }
                else
                {
                    if (i == 15)
                    {
                        mmu->write32(get_pc_prefetch() + 4u, base);
                    }
                    else
                    {
                        mmu->write32(get_register(i), base);
                    }
                }

                if (!pre_index)
                {
                    base += 4u;
                }
            }
        }

        if (user)
        {
            regs.cpsr.cpu_mode = old_mode;
        }
    }

    if (write_back)
    {
        set_register(rn, new_base);
    }

    //console->info("stmd{} r{}{}, {:016b}{}", ((!pre_index) ? "b" : "a"), rn, ((write_back) ? "!" : ""),
    //        rlist, ((user) ? "^" : ""));
}

void CPU::stmi(const uint8_t rn, const uint16_t rlist, const bool pre_index, const bool user, const bool write_back)
{
    uint32_t base     = get_register(rn);
    uint32_t new_base = base + count_bits_set(rlist) * 4u;
    CPU_MODE old_mode = (CPU_MODE)regs.cpsr.cpu_mode;

    if (rlist == 0)
    {
        if (pre_index)
        {
            base += 0x40u;
        }

        mmu->write32(get_pc_prefetch(), base);

        new_base += base;
    }
    else
    {
        if (user)
        {
            regs.cpsr.cpu_mode = CPU_MODE::User;
        }

        uint32_t first_in_list = first_bit_set(rlist);

        for (uint16_t i = 0; i < 16; i++)
        {
            if ((rlist & (1u << i)) != 0)
            {
                if (pre_index)
                {
                    base += 4u;
                }

                if (i == rn && rn != first_in_list)
                {
                    mmu->write32(new_base, base);
                }
                else
                {
                    if (i == 15)
                    {
                        mmu->write32(get_pc_prefetch() + 4u, base);
                    }
                    else
                    {
                        mmu->write32(get_register(i), base);
                    }
                }

                if (!pre_index)
                {
                    base += 4u;
                }
            }
        }

        if (user)
        {
            regs.cpsr.cpu_mode = old_mode;
        }
    }

    if (write_back)
    {
        set_register(rn, new_base);
    }

    //console->info("stmd{} r{}{}, {:016b}{}", ((!pre_index) ? "b" : "a"), rn, ((write_back) ? "!" : ""),
    //        rlist, ((user) ? "^" : ""));
}

void CPU::thumb_add()
{
    bool imm = ((thumb_inst >> 10u) & 1u) != 0;
    bool op  = ((thumb_inst >> 9u) & 1u) != 0;
    uint32_t value = (imm) ? (thumb_inst >> 6u) & 7u : get_register((thumb_inst >> 6u) & 7u);
    uint8_t rs = (thumb_inst >> 3u) & 7u;
    uint8_t rd = thumb_inst & 7u;

    if (op)
    {
        sub(get_register(rs), value, rd, true);
    }
    else
    {
        add(get_register(rs), value, rd, true);
    }
}

void CPU::thumb_add_offset_to_sp()
{
    uint16_t sword8 = (thumb_inst & 0x7Fu) << 2u;

    if ((thumb_inst & 0x80u) != 0)
    {
        set_register(13, get_register(13) - sword8);
    }
    else
    {
        set_register(13, get_register(13) + sword8);
    }
}

void CPU::thumb_alu()
{
    uint8_t op = (thumb_inst >> 6u) & 0xFu;
    uint8_t rs = (thumb_inst >> 3u) & 7u;
    uint8_t rd = thumb_inst & 7u;

    switch (op)
    {
        case 0b0000:
            logical_and(get_register(rd), get_register(rs), rd, true);
            break;
        case 0b0001:
            logical_eor(get_register(rd), get_register(rs), rd, true);
            break;
        case 0b0010:
            set_register(rd, logical_shift_left(get_register(rd), get_register(rs) & 0xFFu, true, false));
            set_nz(get_register(rd));
            break;
        case 0b0011:
            set_register(rd, logical_shift_right(get_register(rd), get_register(rs) & 0xFFu, true, false));
            set_nz(get_register(rd));
            break;
        case 0b0100:
            set_register(rd, arithmetic_shift_right(get_register(rd), get_register(rs) & 0xFFu, true, false));
            set_nz(get_register(rd));
            break;
        case 0b0101:
            adc(get_register(rd), get_register(rs), rd, true);
            break;
        case 0b0110:
            sbc(get_register(rd), get_register(rs), rd, true);
            break;
        case 0b0111:
            set_register(rd, rotate_right(get_register(rd), get_register(rs) & 0xFFu, true, false));
            set_nz(get_register(rd));
            break;
        case 0b1001:
            sub(0, get_register(rs), rd, true);
            break;
        case 0b1010:
            cmp(get_register(rd), get_register(rs));
            break;
        case 0b1011:
            cmn(get_register(rd), get_register(rs));
            break;
        case 0b1100:
            logical_or(get_register(rd), get_register(rs), rd, true);
            break;
        case 0b1101:
            thumb_multiply(get_register(rd), get_register(rs), rd, true);
            break;
        case 0b1110:
            bic(get_register(rd), get_register(rs), rd, true);
            break;
        case 0b1111:
            mov(~get_register(rs), rd, true);
            break;
        default:
            break;
    }
}

void CPU::thumb_conditional_branch()
{
    uint8_t c_code = thumb_op & 0xFu;
    int16_t s_offset8 = (int16_t)(int8_t)(thumb_inst & 0xFFu) << 1;
    uint32_t target_addr = get_pc_prefetch() + s_offset8;

    if (is_condition(c_code))
    {
        set_register(15, target_addr);
    }

    //console->info("b{} ${:08X}", get_c_code(c_code), target_addr);
}

void CPU::thumb_hi_register()
{
    uint8_t op = thumb_op & 3u;
    bool h1 = ((thumb_inst >> 7u) & 1u) != 0;
    bool h2 = ((thumb_inst >> 6u) & 1u) != 0;
    uint8_t rs = ((thumb_inst >> 3u) & 7u) + ((h2) ? 8u : 0u);
    uint8_t rd = (thumb_inst & 7u) + ((h1) ? 8u : 0u);

    switch (op)
    {
        case 0b00:
            add(get_register(rd), get_register(rs), rd, false);
            break;
        case 0b01:
            cmp(get_register(rd), get_register(rs));
            break;
        case 0b10:
            mov(get_register(rs), rd, false);
            break;
        case 0b11:
            thumb_branch_and_exchange(rs);
            break;
        default:
            break;
    }
}

void CPU::thumb_load_address()
{
    bool sp    = ((thumb_inst >> 11u) & 1u) != 0;
    uint8_t rd = (thumb_inst >> 8u) & 7u;
    uint32_t word8  = (thumb_inst & 0xFFu) << 2u;
    uint32_t source = ((sp) ? get_register(13) : get_pc_prefetch() & 0xFFFFFFFDu);

    set_register(rd, source + word8);
}

void CPU::thumb_load_halfword()
{
    bool load  = ((thumb_inst >> 11u) & 1u) != 0;
    uint16_t offset5 = ((thumb_inst >> 6u) & 0x1Fu) << 1u;
    uint8_t rb = (thumb_inst >> 3u) & 7u;
    uint8_t rd = thumb_inst & 7u;
    uint32_t base = get_register(rb) + offset5;

    if (load)
    {
        set_register(rd, mmu->read16(base));
    }
    else
    {
        mmu->write16(get_register(rd), base);
    }
}

void CPU::thumb_load_immediate_offset()
{
    bool byte = ((thumb_inst >> 12u) & 1u) != 0;
    bool load = ((thumb_inst >> 11u) & 1u) != 0;
    uint16_t offset5 = (thumb_inst >> 6u) & 0x1Fu;
    uint8_t rb = (thumb_inst >> 3u) & 7u;
    uint8_t rd = thumb_inst & 7u;
    uint32_t offset_addr = get_register(rb) + ((byte) ? offset5 : offset5 << 2u);

    if (load)
    {
        load_register(rd, offset_addr, byte);
    }
    else
    {
        store_register(rd, offset_addr, byte);
    }
}

void CPU::thumb_load_register_offset()
{
    bool load = ((thumb_inst >> 11u) & 1u) != 0;
    bool byte = ((thumb_inst >> 10u) & 1u) != 0;
    uint8_t ro = (thumb_inst >> 6u) & 7u;
    uint8_t rb = (thumb_inst >> 3u) & 7u;
    uint8_t rd = thumb_inst & 7u;
    uint32_t offset_addr = get_register(rb) + get_register(ro);

    if (load)
    {
        load_register(rd, offset_addr, byte);
    }
    else
    {
        store_register(rd, offset_addr, byte);
    }
}

void CPU::thumb_load_signed_halfword()
{
    uint8_t sh = ((thumb_inst >> 11u) & 1u) | ((thumb_inst >> 9u) & 2u);
    uint8_t ro = (thumb_inst >> 6u) & 7u;
    uint8_t rb = (thumb_inst >> 3u) & 7u;
    uint8_t rd = thumb_inst & 7u;
    uint32_t base = get_register(rb) + get_register(ro);

    switch (sh)
    {
        case 0b00:
            mmu->write16(get_register(rd), base);
            break;
        case 0b01:
            set_register(rd, mmu->read16(base));
            break;
        case 0b10:
            set_register(rd, (int32_t)(int8_t)mmu->read8(base));
            break;
        case 0b11:
            set_register(rd, (int32_t)(int16_t)mmu->read16(base));
            break;
        default:
            break;
    }
}

void CPU::thumb_long_branch()
{
    uint32_t offset = thumb_inst & 0x7FFu;
    uint32_t temp;

    if ((thumb_inst & 0x800u) == 0)
    {
        offset <<= 12u;

        if ((offset & 0x400000u) != 0)
        {
            offset |= 0xFF800000u;
        }

        set_register(14, get_pc_prefetch() + offset);

        //console->info("bl");
    }
    else
    {
        temp = get_pc();

        set_register(15, get_register(14) + (offset << 1u));
        set_register(14, temp | 1u);

        //console->info("bl ${:08X}", get_pc() - 2u);
    }
}

void CPU::thumb_move_immediate()
{
    uint8_t op = (thumb_inst >> 11u) & 3u;
    uint8_t rd = (thumb_inst >> 8u) & 7u;
    uint8_t offset8 = thumb_inst & 0xFFu;

    switch (op)
    {
        case 0b00:
            mov(offset8, rd, true);
            break;
        case 0b01:
            cmp(get_register(rd), offset8);
            break;
        case 0b10:
            add(get_register(rd), offset8, rd, true);
            break;
        case 0b11:
            sub(get_register(rd), offset8, rd, true);
            break;
        default:
            break;
    }
}

void CPU::thumb_move_shifted_register()
{
    uint8_t op = (thumb_inst >> 11u) & 3u;
    uint8_t offset5 = (thumb_inst >> 6u) & 0x1Fu;
    uint8_t rs = (thumb_inst >> 3u) & 7u;
    uint8_t rd = thumb_inst & 7u;

    switch (op)
    {
        case 0b00:
            //console->info("lsl r{}, r{}, #{}", rd, rs, offset5);

            set_register(rd, logical_shift_left(get_register(rs), offset5, true, true));
            break;
        case 0b01:
            //console->info("lsr r{}, r{}, #{}", rd, rs, offset5);

            set_register(rd, logical_shift_right(get_register(rs), offset5, true, true));
            break;
        case 0b10:
            //console->info("asr r{}, r{}, #{}", rd, rs, offset5);

            set_register(rd, arithmetic_shift_right(get_register(rs), offset5, true, true));
            break;
        default:
            console->error("Invalid shift, ROR not available!");

            throw std::runtime_error("Invalid shift!");
    }

    set_nz(get_register(rd));
}

void CPU::thumb_multiple_load()
{
    bool load  = (thumb_op & 8u) != 0;
    uint8_t rb = thumb_op & 7u;
    uint8_t rlist = thumb_inst & 0xFFu;

    if (load)
    {
        thumb_ldmia(rb, rlist);
    }
    else
    {
        thumb_stmia(rb, rlist);
    }
}

void CPU::thumb_pc_relative_load()
{
    uint8_t rd = (thumb_inst >> 8u) & 7u;
    uint16_t word8 = (thumb_inst & 0xFFu) << 2u;

    load_register(rd, (get_pc_prefetch() & 0xFFFFFFFDu) + word8, false);
}

void CPU::thumb_push_registers()
{
    bool load = ((thumb_inst >> 11u) & 1u) != 0;
    bool load_pc_lr = ((thumb_inst >> 8u) & 1u) != 0;
    uint8_t rlist = thumb_inst & 0xFFu;

    if (load)
    {
        thumb_pop(rlist, load_pc_lr);
    }
    else
    {
        thumb_push(rlist, load_pc_lr);
    }
}

void CPU::thumb_sp_relative_load()
{
    bool load  = ((thumb_inst >> 11u) & 1u) != 0;
    uint8_t rd = (thumb_inst >> 8u) & 7u;
    uint16_t word8 = (thumb_inst & 0xFFu) << 2u;

    if (load)
    {
        load_register(rd, get_register(13) + word8, false);
    }
    else
    {
        mmu->write32(get_register(rd), get_register(13) + word8);
    }
}

void CPU::thumb_unconditional_branch()
{
    uint32_t offset11 = (thumb_inst & 0x7FFu) << 1u;

    if ((offset11 & 0x800u) != 0)
    {
        offset11 |= 0xFFFFF000u;
    }

    set_register(15, get_pc_prefetch() + offset11);
}

void CPU::thumb_branch_and_exchange(const uint8_t rs)
{
    uint32_t target_addr = get_register(rs);

    set_register(15, target_addr);
    set_cpu_state((target_addr & 1u) != 0);

    //console->info("bx r{}", rs);
}

void CPU::thumb_multiply(const uint32_t a, const uint32_t b, const uint8_t rd, const bool set_c)
{
    uint32_t result = a * b;

    set_register(rd, result);

    if (set_c)
    {
        set_nz(result);
    }
}

void CPU::thumb_ldmia(const uint8_t rb, const uint8_t rlist)
{
    uint32_t base = get_register(rb);
    uint32_t new_base;

    if (rlist == 0)
    {
        set_register(15, mmu->read32(base));

        new_base = base + 40u;
    }
    else
    {
        new_base = base + (count_bits_set(rlist) * 4u);

        for (uint16_t i = 0; i < 16; i++)
        {
            if ((rlist & (1u << i)) != 0)
            {
                set_register(i, mmu->read32(base));

                base += 4u;
            }
        }
    }

    if ((rlist & (1u << rb)) == 0)
    {
        set_register(rb, new_base);
    }

    //console->info("ldmia r{}!, {:08b}", rb, rlist);
}

void CPU::thumb_stmia(const uint8_t rb, const uint8_t rlist)
{
    uint32_t base = get_register(rb);
    uint32_t new_base;

    if (rlist == 0)
    {
        mmu->write32(get_pc_prefetch(), base);

        new_base = base + 0x40u;
    }
    else
    {
        uint32_t first_in_list = first_bit_set(rlist);
        new_base = base + (count_bits_set(rlist) * 4u);

        for (uint16_t i = 0; i < 8; i++)
        {
            if ((rlist & (1u << i)) != 0)
            {
                if (i == rb && rb != first_in_list)
                {
                    mmu->write32(new_base, base);
                }
                else
                {
                    mmu->write32(get_register(i), base);
                }

                base += 4u;
            }
        }
    }

    set_register(rb, new_base);

    //console->info("stmia r{}!, {:08b}", rb, rlist);
}

void CPU::thumb_push(const uint8_t rlist, const bool lr)
{
    uint32_t old_base = get_register(13);
    uint32_t base = old_base - (count_bits_set(rlist) * 4u + ((lr) ? 4u : 0u));
    uint32_t new_base = base;

    for (uint16_t i = 0; i < 8; i++)
    {
        if ((rlist & (1u << i)) != 0)
        {
            mmu->write32(get_register(i), base);

            base += 4u;
        }
    }

    if (lr)
    {
        mmu->write32(get_register(14), base);

        base += 4u;
    }

    if (base != old_base)
    {
        console->critical("stmdb: new base doesn't match old base!");

        throw std::runtime_error("New base doesn't match old base!");
    }

    set_register(13, new_base);

    //console->info("stmdb sp!, {:08b}{}", rlist, ((lr) ? ", r14" : ""));
}

void CPU::thumb_pop(const uint8_t rlist, const bool pc)
{
    uint32_t base = get_register(13);
    uint32_t new_base = base + (count_bits_set(rlist) * 4u);

    for (uint16_t i = 0; i < 8; i++)
    {
        if ((rlist & (1u << i)) != 0)
        {
            set_register(i, mmu->read32(base));

            base += 4u;
        }
    }

    if (pc)
    {
        set_register(15, mmu->read32(base));

        new_base += 4u;
    }

    set_register(13, new_base);

    //console->info("ldmia sp!, {:08b}{}", rlist, ((pc) ? ", r15" : ""));
}

void CPU::dump_registers() const
{
    console->info("{:08X}h {:08X}h {:08X}h {:08X}h {:08X}h {:08X}h {:08X}h {:08X}h {:08X}h",
            get_register(0), get_register(1), get_register(2), get_register(3),
            get_register(4), get_register(5), get_register(6), get_register(7),
            get_register(8));
    console->info("{:08X}h {:08X}h {:08X}h {:08X}h {:08X}h {:08X}h {:08X}h cpsr: {:08X}h [{}]",
            get_register(9), get_register(10), get_register(11), get_register(12),
            get_register(13), get_register(14), get_register(15), get_cpsr(), get_cpsr_flags());
}

void CPU::run()
{
    if ((mmu->interrupt_master_enable & 1u) == 1 && !regs.cpsr.irq_disable)
    {
        if ((mmu->interrupt_enable & mmu->interrupt_request_flags) != 0)
        {
            hardware_interrupt();
            return;
        }
    }

    (this->*state_table[regs.cpsr.thumb_state])();
    //dump_registers();
}
