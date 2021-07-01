#ifndef RISCV_SIMULATOR_HPP
#define RISCV_SIMULATOR_HPP

#include <cstring>
#include <iostream>
#include "memory.hpp"

using namespace std;

extern memoryManager mManager;

enum commandType
{
    LUI, AUIPC, JAL, JALR, BEQ, BNE, BLT, BGE, BLTU, BGEU, LB, LH, LW, LBU,
    LHU, SB, SH, SW, ADDI, SLTI, SLTIU, XORI, ORI, ANDI, SLLI, SRLI, SRAI,
    ADD, SUB, SLL, SLT, SLTU, XOR, SRL, SRA, OR, AND
};

class Simulator
{
private:
    int pc;
    unsigned reg[32];

    struct instruction
    {
        commandType cmdType;
        unsigned cmd;
        int opcode;
        int rs1, rs2, rd;
        int func3, func7;
        int imm;
        int shamt;
        int vd;
        int pc;
        bool busy;

        void clear()
        {
            cmd = opcode = rs1 = rs2 = rd = func3 = func7 = imm = shamt = vd = pc = 0;
            busy = false;
        }
    } id, ex, mem, wb;

    unsigned pow[32];
    unsigned bi[32];

public:
    Simulator()
    {
        pc = 0;
        memset(reg, 0, sizeof(reg));
        for (int i = 0; i < 32; ++i)
        {
            pow[i] = (1 << i) - 1;
            bi[i] = (-1) << i;
        }
    }

    unsigned run()
    {
        while (true)
        {
            IF();
            if (id.cmd == 0x0FF00513) break;
            ID();
            EX();
            MEM();
            WB();
        }
        return reg[10] & pow[8];
    }

    void IF()
    {
        id.cmd = mManager.load(pc, 4);
        id.pc = pc;
//        cout << pc << endl;
        pc += 4;
    }

    void ID()
    {
        unsigned inst = id.cmd;
        id.opcode = inst & pow[7];
        switch (id.opcode)
        {
            case 0x37:id.cmdType = LUI;
                id.rd = (inst >> 7) & pow[5];
                id.imm = inst & bi[12];
                break;
            case 0x17:id.cmdType = AUIPC;
                id.rd = (inst >> 7) & pow[5];
                id.imm = inst & bi[12];
                break;
            case 0x6F:id.cmdType = JAL;
                id.rd = (inst >> 7) & pow[5];
                id.imm = ((inst >> 31) & pow[1]) == 1 ? bi[20] : 0;
                id.imm |= inst & (bi[12] - bi[20]);
                id.imm |= ((inst >> 20) & pow[1]) << 11u;
                id.imm |= (inst & (bi[21] - bi[31])) >> 20u;
                break;
            case 0x67:
//                printReg();
                id.cmdType = JALR;
                id.rd = (inst >> 7) & pow[5];
                id.func3 = (inst >> 12) & pow[3];
                id.rs1 = (inst >> 15) & pow[5];
                id.imm = ((inst >> 31) & pow[1]) == 1 ? bi[11] : 0;
                id.imm |= (inst >> 20) & pow[11];
                break;
            case 0x63:id.func3 = (inst >> 12) & pow[3];
                id.rs1 = (inst >> 15) & pow[5];
                id.rs2 = (inst >> 20) & pow[5];
                id.imm = ((inst >> 31) & pow[1]) == 1 ? bi[12] : 0;
                id.imm |= ((inst >> 7) & pow[1]) << 11u;
                id.imm |= (inst & (bi[25] - bi[31])) >> 20u;
                id.imm |= (inst & (bi[8] - bi[12])) >> 7u;
                switch (id.func3)
                {
                    case 0:id.cmdType = BEQ;
                        break;
                    case 1:id.cmdType = BNE;
                        break;
                    case 4:id.cmdType = BLT;
                        break;
                    case 5:id.cmdType = BGE;
                        break;
                    case 6:id.cmdType = BLTU;
                        break;
                    case 7:id.cmdType = BGEU;
                }
                break;
            case 0x3:id.rd = (inst >> 7) & pow[5];
                id.func3 = (inst >> 12) & pow[3];
                id.rs1 = (inst >> 15) & pow[5];
                id.imm = ((inst >> 31) & pow[1]) == 1 ? bi[11] : 0;
                id.imm |= (inst >> 20) & pow[11];
                switch (id.func3)
                {
                    case 0:id.cmdType = LB;
                        break;
                    case 1:id.cmdType = LH;
                        break;
                    case 2:id.cmdType = LW;
                        break;
                    case 4:id.cmdType = LBU;
                        break;
                    case 5:id.cmdType = LHU;
                }
                break;
            case 0x23:id.func3 = (inst >> 12) & pow[3];
                id.rs1 = (inst >> 15) & pow[5];
                id.rs2 = (inst >> 20) & pow[5];
                id.imm = ((inst >> 31) & pow[1]) == 1 ? bi[11] : 0;
                id.imm |= (inst & (bi[25] - bi[31])) >> 20u;
                id.imm |= (inst & (bi[7] - bi[12])) >> 7u;
                switch (id.func3)
                {
                    case 0:id.cmdType = SB;
                        break;
                    case 1:id.cmdType = SH;
                        break;
                    case 2:id.cmdType = SW;
                }
                break;
            case 0x13:id.rd = (inst >> 7) & pow[5];
                id.func3 = (inst >> 12) & pow[3];
                id.rs1 = (inst >> 15) & pow[5];
                id.imm = ((inst >> 31) & pow[1]) == 1 ? bi[11] : 0;
                id.imm |= (inst >> 20) & pow[11];
                id.shamt = (inst >> 20) & pow[5];
                id.func7 = (inst >> 25) & pow[7];
                switch (id.func3)
                {
                    case 0:id.cmdType = ADDI;
                        break;
                    case 2:id.cmdType = SLTI;
                        break;
                    case 3:id.cmdType = SLTIU;
                        break;
                    case 4:id.cmdType = XORI;
                        break;
                    case 6:id.cmdType = ORI;
                        break;
                    case 7:id.cmdType = ANDI;
                        break;
                    case 1:id.cmdType = SLLI;
                        break;
                    case 5:id.cmdType = id.func7 == 0 ? SRLI : SRAI;
                }
                break;
            case 0x33:id.rd = (inst >> 7) & pow[5];
                id.func3 = (inst >> 12) & pow[3];
                id.rs1 = (inst >> 15) & pow[5];
                id.rs2 = (inst >> 20) & pow[5];
                id.func7 = (inst >> 25) & pow[7];
                switch (id.func3)
                {
                    case 0:id.cmdType = id.func7 == 0 ? ADD : SUB;
                        break;
                    case 1:id.cmdType = SLL;
                        break;
                    case 2:id.cmdType = SLT;
                        break;
                    case 3:id.cmdType = SLTU;
                        break;
                    case 4:id.cmdType = XOR;
                        break;
                    case 5:id.cmdType = id.func7 == 0 ? SRL : SRA;
                        break;
                    case 6:id.cmdType = OR;
                        break;
                    case 7:id.cmdType = AND;
                }
        }
    }

    void EX()
    {
        ex = id;
        switch (ex.cmdType)
        {
            case LUI:ex.vd = ex.imm;
                break;
            case AUIPC:ex.vd = ex.pc + ex.imm;
                break;
            case JAL:ex.vd = ex.pc + 4;
                pc = ex.pc + ex.imm;
                break;
            case JALR:ex.vd = ex.pc + 4;
                pc = (reg[ex.rs1] + ex.imm) & ~1;
                break;
            case BEQ:if (reg[ex.rs1] == reg[ex.rs2]) pc = ex.pc + ex.imm;
                break;
            case BNE:if (reg[ex.rs1] != reg[ex.rs2]) pc = ex.pc + ex.imm;
                break;
            case BLT:if ((int) reg[ex.rs1] < (int) reg[ex.rs2]) pc = ex.pc + ex.imm;
                break;
            case BGE:if ((int) reg[ex.rs1] >= (int) reg[ex.rs2]) pc = ex.pc + ex.imm;
                break;
            case BLTU:if (reg[ex.rs1] < reg[ex.rs2]) pc = ex.pc + ex.imm;
                break;
            case BGEU:if (reg[ex.rs1] >= reg[ex.rs2]) pc = ex.pc + ex.imm;
                break;
            case LB:
            case LH:
            case LW:
            case LBU:
            case LHU:ex.vd = reg[ex.rs1] + ex.imm;
                break;
            case SB:
            case SH:
            case SW:ex.vd = reg[ex.rs2];
                ex.rd = reg[ex.rs1] + ex.imm;
                break;
            case ADDI:ex.vd = reg[ex.rs1] + ex.imm;
                break;
            case SLTI:ex.vd = (int) reg[ex.rs1] < ex.imm;
                break;
            case SLTIU:ex.vd = reg[ex.rs1] < ex.imm;
                break;
            case XORI:ex.vd = reg[ex.rs1] ^ ex.imm;
                break;
            case ORI:ex.vd = reg[ex.rs1] | ex.imm;
                break;
            case ANDI:ex.vd = reg[ex.rs1] & ex.imm;
                break;
            case SLLI:ex.vd = reg[ex.rs1] << ex.shamt;
                break;
            case SRLI:ex.vd = reg[ex.rs1] >> ex.shamt;
                break;
            case SRAI:ex.vd = (int) reg[ex.rs1] >> ex.shamt;
                break;
            case ADD:ex.vd = reg[ex.rs1] + reg[ex.rs2];
                break;
            case SUB:ex.vd = reg[ex.rs1] - reg[ex.rs2];
                break;
            case SLL:ex.vd = reg[ex.rs1] << (reg[ex.rs2] & pow[5]);
                break;
            case SLT:ex.vd = (int) reg[ex.rs1] < (int) reg[ex.rs2];
                break;
            case SLTU:ex.vd = reg[ex.rs1] < reg[ex.rs2];
                break;
            case XOR:ex.vd = reg[ex.rs1] ^ reg[ex.rs2];
                break;
            case SRL:ex.vd = reg[ex.rs1] >> (reg[ex.rs2] & pow[5]);
                break;
            case SRA:ex.vd = (int) reg[ex.rs1] >> (reg[ex.rs2] & pow[5]);
                break;
            case OR:ex.vd = reg[ex.rs1] | reg[ex.rs2];
                break;
            case AND:ex.vd = reg[ex.rs1] & reg[ex.rs2];
        }
    }

    void MEM()
    {
        mem = ex;
        switch (mem.cmdType)
        {
            case LB:mem.vd = mManager.load(mem.vd, 1);
                mem.vd |= (((mem.vd >> 7) & pow[1]) == 1) ? bi[8] : 0;
                break;
            case LH:mem.vd = mManager.load(mem.vd, 2);
                mem.vd |= (((mem.vd >> 15) & pow[1]) == 1) ? bi[16] : 0;
                break;
            case LW:mem.vd = mManager.load(mem.vd, 4);
                break;
            case LBU:mem.vd = mManager.load(mem.vd, 1);
                break;
            case LHU:mem.vd = mManager.load(mem.vd, 2);
                break;
            case SB:mManager.store(mem.rd, mem.vd, 1);
                break;
            case SH:mManager.store(mem.rd, mem.vd, 2);
                break;
            case SW:mManager.store(mem.rd, mem.vd, 4);
                break;
            default:break;
        }
    }

    void WB()
    {
        wb = mem;
        switch (wb.cmdType)
        {
            case BEQ:
            case BNE:
            case BLT:
            case BGE:
            case BLTU:
            case BGEU:
            case SB:
            case SH:
            case SW:break;
            default:if (wb.rd != 0) reg[wb.rd] = wb.vd;
        }
    }

    void printReg()
    {
        cout << "--------printReg" << endl;
        for (int i = 0; i < 32; ++i)
            cout << i << " " << reg[i] << endl;
        cout << "----------------" << endl;
    }
};

#endif //RISCV_SIMULATOR_HPP
