#include "CPU.h"
#include "opcodes.h"
#include <cstdint>
#include "../bus/bus.h" // IWYU pragma: keep

static struct {
    uint16_t nmi;
    uint16_t reset;
    uint16_t irq;
} vectors = {0xfffa, 0xfffc, 0xfffe};

CPU::CPU(Bus& mem) : pendingNMI(false), bus(mem) {};

CPURegisters CPU::get_regs() const
{
    CPURegisters state{};
    state.pc = regs.pc;
    state.a = regs.a;
    state.x = regs.x;
    state.y = regs.y;
    state.sp = regs.sp;
    state.status = flags.all;
    return state;
}

void CPU::reset()
{
    reset(read_address(vectors.reset));
}

void CPU::reset(uint16_t start_addr)
{
    skipCycles = 0;
    cycles = 0;

    regs.a = 0;
    regs.x = 0;
    regs.y = 0;
    regs.pc = start_addr;
    regs.sp = 0xfd;

    flags.I = 1;

    flags.C = 0;
    flags.D = 0;
    flags.N = 0;
    flags.V = 0;
    flags.Z = 0;

}

void CPU::nmiInterrupt()
{
    this->pendingNMI = true;
}

uint16_t CPU::read_address(uint16_t addr)
{
    return this->bus.read(addr) | this->bus.read(addr + 1) << 8;
}

IRQ& CPU::createIRQHandler()
{
    int bit = 1 << m_irqHandlers.size();
    IRQ irq(bit, *this);
    m_irqHandlers.emplace_back(irq);
    return m_irqHandlers.back();
}

void IRQ::release()
{
    cpu.setIRQPulldown(bit, false);
}

void IRQ::pull()
{
    cpu.setIRQPulldown(bit, true);
}

void CPU::setIRQPulldown(int bit, bool state)
{
    int mask       = ~(1 << bit);
    m_irqPulldowns = (m_irqPulldowns & mask) | state;
};

void CPU::interruptSequence(InterruptType type)
{
    if (flags.I && type != INTR_NMI && type != INTR_BRK)
        return;

    // BRK pushes pc by 1.
    if (type == INTR_BRK)
        ++regs.pc;

    stack_push(regs.pc >> 8);
    stack_push(regs.pc);

    flags.B = (type == INTR_BRK);
    stack_push(flags.all);

    flags.I = true;

    switch (type) {
        case INTR_IRQ:
        case INTR_BRK:
            regs.pc = read_address(vectors.irq);
            break;
        case INTR_NMI:
            regs.pc = read_address(vectors.nmi);
            break;
    }

    // Interrupts take 7 cycles
    skipCycles += 7;
}

void CPU::stack_push(uint8_t value)
{
    this->bus.write(0x100 | regs.sp, value);
    --regs.sp;
}

uint8_t CPU::stack_pop()
{
    return this->bus.read(0x100 | ++regs.sp);
}

void CPU::set_zn(uint8_t value)
{
    flags.Z = !value;
    flags.N = (value & 0x80) != 0;
}

void CPU::skipPageCrossCycle(uint16_t a, uint16_t b)
{
    if ((a & 0xff00) != (b & 0xff00))
        skipCycles += 1;
}

void CPU::skipOAMDMACycles()
{
    // 256 + 256 (read write) + 1 dummy
    skipCycles += (cycles & 1); // odd cycles add 1
    skipCycles += 513;
}

void CPU::skipDMCDMACycles()
{
    // TODO: DMCDMA is not cycle accurate here! It depends on alignment. (does this matter?)
    skipCycles += 3;
}

void CPU::clock()
{
    ++cycles;

    if (skipCycles-- > 1)
        return;

    skipCycles = 0;

    // NMI has higher priority, check for it first
    if (this->pendingNMI)
    {
        interruptSequence(INTR_NMI);
        this->pendingNMI = false;
        return;
    }
    else if (isPendingIRQ())
    {
        interruptSequence(INTR_IRQ);
        return;
    }

    uint8_t opcode = this->bus.read(regs.pc++);

    auto CycleLength = OperationCycles[opcode];

    if (CycleLength && (executeImplied(opcode) || executeBranch(opcode) || executeType1(opcode) || executeType2(opcode) || executeType0(opcode))) {
        skipCycles += CycleLength;
    } else {
        printf("Unrecognized opcode: %02X\n", opcode);
    }
}

bool CPU::executeImplied(uint8_t opcode)
{
    switch (static_cast<OperationImplied>(opcode)) {
    case NOP:
        break;
    case BRK:
        interruptSequence(INTR_BRK);
        break;
    case JSR:
        // Push address of next instruction - 1, thus r_PC + 1 instead of r_PC + 2
        // since r_PC and r_PC + 1 are address of subroutine
        stack_push((regs.pc + 1) >> 8);
        stack_push(regs.pc + 1);
        regs.pc = read_address(regs.pc);
        break;
    case RTS:
        regs.pc = stack_pop();
        regs.pc |= stack_pop() << 8;
        ++regs.pc;
        break;
    case RTI:
        flags.all = stack_pop();
        regs.pc = stack_pop();
        regs.pc |= stack_pop() << 8;
        break;
    case JMP:
        regs.pc = read_address(regs.pc);
        break;
    case JMPI: {
        uint16_t location = read_address(regs.pc);
        // this emulates a 6502 bug where when the vector of an indirect address begins at the last byte of a page,
        // the second byte is fetched from the beginning of that page rather than the beginning of the next
        uint16_t page = location & 0xff00;
        regs.pc = this->bus.read(location) | this->bus.read(page | ((location + 1) & 0xff)) << 8;
    }
    break;
    case PHP:
        stack_push(flags.all);
        break;
    case PLP:
        flags.all = stack_pop();
        break;
    case PHA:
        stack_push(regs.a);
        break;
    case PLA:
        regs.a = stack_pop();
        set_zn(regs.a);
        break;
    case DEY:
        --regs.y;
        set_zn(regs.y);
        break;
    case DEX:
        --regs.x;
        set_zn(regs.x);
        break;
    case TAY:
        regs.y = regs.a;
        set_zn(regs.y);
        break;
    case INY:
        ++regs.y;
        set_zn(regs.y);
        break;
    case INX:
        ++regs.x;
        set_zn(regs.x);
        break;
    case CLC:
        flags.C = 0;
        break;
    case SEC:
        flags.C = 1;
        break;
    case CLI:
        flags.I = 0;
        break;
    case SEI:
        flags.I = 1;
        break;
    case CLD:
        flags.D = 0;
        break;
    case SED:
        flags.D = 1;
        break;
    case TYA:
        regs.a = regs.y;
        set_zn(regs.a);
        break;
    case CLV:
        flags.V = false;
        break;
    case TXA:
        regs.a = regs.x;
        set_zn(regs.a);
        break;
    case TXS:
        regs.sp = regs.x;
        break;
    case TAX:
        regs.x = regs.a;
        set_zn(regs.x);
        break;
    case TSX:
        regs.x = regs.sp;
        set_zn(regs.x);
        break;
    default:
        return false;
    };
    return true;
}

bool CPU::executeBranch(uint8_t opcode)
{
    if ((opcode & BranchInstructionMask) == BranchInstructionMaskResult) {
        // branch is initialized to the condition required (for the flag specified later)
        bool branch = opcode & BranchConditionMask;

        // set branch to true if the given condition is met by the given flag
        // We use xnor here, it is true if either both operands are true or false
        switch (opcode >> BranchOnFlagShift) {
            case Negative:
                branch = !(branch ^ flags.N);
                break;
            case Overflow:
                branch = !(branch ^ flags.V);
                break;
            case Carry:
                branch = !(branch ^ flags.C);
                break;
            case Zero:
                branch = !(branch ^ flags.Z);
                break;
            default:
                return false;
        }

        if (branch) {
            int8_t offset = this->bus.read(regs.pc++);
            // skip 1 cycle since branch is taken
            ++skipCycles;
            uint16_t newPC = (regs.pc + offset);
            // skip 1 additional cycle if page is crossed
            skipPageCrossCycle(regs.pc, newPC);
            regs.pc = newPC;
        } else
            ++regs.pc;
        return true;
    }
    return false;
}

bool CPU::executeType1(uint8_t opcode)
{
    if ((opcode & InstructionModeMask) == 0x1) {
        uint16_t location = 0; // operand location
        auto op = static_cast<Operation1>((opcode & OperationMask) >> OperationShift);
        switch (static_cast<AddrMode1>((opcode & AddrModeMask) >> AddrModeShift)) {
            case IndexedIndirectX: {
                uint8_t zero_addr = regs.x + this->bus.read(regs.pc++);
                // addresses wrap in zero page mode
                location = this->bus.read(zero_addr & 0xff) | this->bus.read((zero_addr + 1) & 0xff) << 8;
                break;
            }
            case ZeroPage:
                location = this->bus.read(regs.pc++);
                break;
            case Immediate:
                location = regs.pc++;
                break;
            case Absolute:
                location = read_address(regs.pc);
                regs.pc += 2;
                break;
            case IndirectY: {
                uint8_t zero_addr = this->bus.read(regs.pc++);
                location = this->bus.read(zero_addr & 0xff) | this->bus.read((zero_addr + 1) & 0xff) << 8;
                if (op != STA)
                    skipPageCrossCycle(location, location + regs.y);
                location += regs.y;
                break;
            }

            case IndexedX:
                // Address wraps around in the zero page
                location = (this->bus.read(regs.pc++) + regs.x) & 0xff;
                break;
            case AbsoluteY:
                location  = read_address(regs.pc);
                regs.pc += 2;
                if (op != STA)
                    skipPageCrossCycle(location, location + regs.y);
                location += regs.y;
                break;
            case AbsoluteX:
                location  = read_address(regs.pc);
                regs.pc += 2;
                if (op != STA)
                    skipPageCrossCycle(location, location + regs.x);
                location += regs.x;
                break;
            default:
                return false;
        }

        switch (op) {
            case ORA:
                regs.a |= this->bus.read(location);
                set_zn(regs.a);
                break;
            case EOR:
                regs.a ^= this->bus.read(location);
                set_zn(regs.a);
                break;
            case AND:
                regs.a &= this->bus.read(location);
                set_zn(regs.a);
                break;
            case ADC: {
                uint8_t operand = this->bus.read(location);
                std::uint16_t sum = regs.a + operand + flags.C;
                // Carry forward or UNSIGNED overflow
                flags.C = (sum & 0x100) != 0;
                // SIGNED overflow, would only happen if the sign of sum is
                // different from BOTH the operands
                flags.V = ((regs.a ^ sum) & (operand ^ sum) & 0x80) != 0;
                regs.a = static_cast<uint8_t>(sum);
                set_zn(regs.a);
                break;
            }
            case STA:
                this->bus.write(location, regs.a);
                break;
            case LDA:
                regs.a = this->bus.read(location);
                set_zn(regs.a);
                break;
            case SBC: {
                // High carry means "no borrow", thus negate and subtract
                std::uint16_t subtrahend = this->bus.read(location), diff = regs.a - subtrahend - !flags.C;
                // if the ninth bit is 1, the resulting number is negative => borrow => low carry
                flags.C = !(diff & 0x100);
                // Same as ADC, except instead of the subtrahend,
                // substitute with it's one complement
                flags.V = ((regs.a ^ diff) & (~subtrahend ^ diff) & 0x80) != 0;
                regs.a = diff;
                set_zn(diff);
                break;
            }
            case CMP: {
                std::uint16_t diff = regs.a - this->bus.read(location);
                flags.C = !(diff & 0x100);
                set_zn(diff);
                break;
            }

            default:
                return false;
            }
            return true;
        }
    return false;
}

bool CPU::executeType2(uint8_t opcode)
{
    if ((opcode & InstructionModeMask) == 2)
    {
        uint16_t location  = 0;
        auto     op        = static_cast<Operation2>((opcode & OperationMask) >> OperationShift);
        auto     addr_mode = static_cast<AddrMode2>((opcode & AddrModeMask) >> AddrModeShift);
        switch (addr_mode) {
            case Addr_Immediate:
                location = regs.pc++;
                break;
            case Addr_ZeroPage:
                location = this->bus.read(regs.pc++);
                break;
            case Addr_Accumulator:
                break;
            case Addr_Absolute:
                location  = read_address(regs.pc);
                regs.pc += 2;
                break;
            case Addr_Indexed: {
                location = this->bus.read(regs.pc++);
                uint8_t index;
                if (op == LDX || op == STX)
                    index = regs.y;
                else
                    index = regs.x;
                // zp wrapping
                location = (location + index) & 0xff;
                break;
            }
            case Addr_AbsoluteIndexed: {
                location  = read_address(regs.pc);
                regs.pc += 2;
                uint8_t index;
                if (op == LDX || op == STX)
                    index = regs.y;
                else
                    index = regs.x;
                skipPageCrossCycle(location, location + index);
                location += index;
                break;
            }

            default:
                return false;
        }

        std::uint16_t operand = 0;
        switch (op) {
        case ASL:
        case ROL: {
            if (addr_mode == Addr_Accumulator) {
                auto prev_C = flags.C;
                flags.C = (regs.a & 0x80) != 0;
                regs.a <<= 1;
                // If Rotating, set the bit-0 to the the previous carry
                regs.a |= prev_C && (op == ROL);
                set_zn(regs.a);
            } else {
                auto prev_C = flags.C;
                operand = this->bus.read(location);
                flags.C = (operand & 0x80) != 0;
                operand = operand << 1 | (prev_C && (op == ROL));
                set_zn(operand);
                this->bus.write(location, operand);
            }
            break;
        }
        case LSR:
        case ROR: {
            if (addr_mode == Addr_Accumulator) {
                auto prev_C = flags.C;
                flags.C = (regs.a & 1) != 0;
                regs.a >>= 1;
                // If Rotating, set the bit-7 to the previous carry
                regs.a = regs.a | (prev_C && (op == ROR)) << 7;
                set_zn(regs.a);
            } else {
                auto prev_C = flags.C;
                operand = this->bus.read(location);
                flags.C = (operand & 1) != 0;
                operand = operand >> 1 | (prev_C && (op == ROR)) << 7;
                set_zn(operand);
                this->bus.write(location, operand);
            }
            break;
        }
        case STX:
            this->bus.write(location, regs.x);
            break;
        case LDX:
            regs.x = this->bus.read(location);
            set_zn(regs.x);
            break;
        case DEC: {
            auto loc = this->bus.read(location) - 1;
            set_zn(loc);
            this->bus.write(location, loc);
            break;
        }
        case INC: {
            auto loc = this->bus.read(location) + 1;
            set_zn(loc);
            this->bus.write(location, loc);
            break;
        }
        default:
            return false;
        }
        return true;
    }
    return false;
}

bool CPU::executeType0(uint8_t opcode)
{
    if ((opcode & InstructionModeMask) == 0x0) {
        uint16_t location = 0;
        switch (static_cast<AddrMode2>((opcode & AddrModeMask) >> AddrModeShift)) {
            case Addr_Immediate:
                location = regs.pc++;
                break;
            case Addr_ZeroPage:
                location = this->bus.read(regs.pc++);
                break;
            case Addr_Absolute:
                location = read_address(regs.pc);
                regs.pc += 2;
                break;
            case Addr_Indexed:
                // more zp wrapping
                location = (this->bus.read(regs.pc++) + regs.x) & 0xff;
                break;
            case Addr_AbsoluteIndexed:
                location = read_address(regs.pc);
                regs.pc += 2;
                skipPageCrossCycle(location, location + regs.x);
                location += regs.x;
                break;
            default:
                return false;
        }
        std::uint16_t operand = 0;
        switch (((opcode & OperationMask) >> OperationShift)) {
            case BIT:
                operand = this->bus.read(location);
                flags.Z = !(regs.a & operand);
                flags.V = (operand & 0x40) != 0;
                flags.N = (operand & 0x80) != 0;
                break;
            case STY:
                this->bus.write(location, regs.y);
                break;
            case LDY:
                regs.y = this->bus.read(location);
                set_zn(regs.y);
                break;
            case CPX: {
                std::uint16_t diff = regs.x - this->bus.read(location);
                flags.C = !(diff & 0x100);
                set_zn(diff);
                break;
            }
            case CPY: {
                std::uint16_t diff = regs.y - this->bus.read(location);
                flags.C = !(diff & 0x100);
                set_zn(diff);
                break;
            }
            default:
                return false;
        }

        return true;
    }
    return false;
}
