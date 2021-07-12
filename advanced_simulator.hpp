#ifndef RISCV_ADVANCED_SIMULATOR_HPP
#define RISCV_ADVANCED_SIMULATOR_HPP

#include <cstring>
#include <iostream>
#include "memory.hpp"
#include <iomanip>

using namespace std;

//#define Debug
//#define print
#define BHT

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
    int pc = 0;
    unsigned reg[32];

    struct instruction //pipeline register
    {
        int pc = 0;
        commandType cmdType;
        unsigned cmd = 0;
        int opcode = 0;
        int rs1 = 0, rs2 = 0, rd = 0;
        unsigned vs1 = 0, vs2 = 0, vd = 0;
        int func3 = 0, func7 = 0;
        int imm = 0;
        int shamt = 0;
    } IF_ID, ID_EX, EX_MEM, MEM_WB;
    bool busy[5]; //IF ID EX MEM WB

    //forwarding(data hazards)
    int MEM_WB_rd = 0;
    unsigned MEM_WB_vd = 0;

    //prediction
    int pd_table[128];
    int pd_right = 0;
    int pd_tot = 0;

    unsigned pow[32]; //0...01...1
    unsigned bi[32]; //1...10...0

public:
    Simulator()
    {
        memset(reg, 0, sizeof(reg));
        memset(pd_table, 0, sizeof(pd_table));
        memset(busy, 0, sizeof(busy));
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
            if (busy[4])
            {
                WB();
#ifdef print
                printInst(MEM_WB);
                printReg();
#endif
            }
            if (busy[3] && (!busy[4])) MEM();
            if (busy[2] && (!busy[3])) EX();
            if (busy[1] && (!busy[2]) && IF_ID.cmd != 0x0FF00513) ID();
            if (!busy[1] && IF_ID.cmd != 0x0FF00513) IF();
            if (IF_ID.cmd == 0x0FF00513 && busy[1] && !busy[2] && !busy[3] && !busy[4]) break;
        }
#ifdef BHT
        cout << "Correct predictions: " << pd_right << endl;
        cout << "Total predictions: " << pd_tot << endl;
        cout << "Accuracy of prediction: ";
        if (pd_tot == 0) cout << "-" << endl;
        else cout << fixed << setprecision(2) << (double) pd_right / pd_tot * 100 << "%" << endl;
#endif
        return reg[10] & pow[8];
    }

    void IF()
    {
        IF_ID = instruction();
        IF_ID.cmd = mManager.load(pc, 4);
        IF_ID.pc = pc;
#ifdef Debug
        cout << std::hex << pc << endl;
#endif
        pc += 4;
        busy[1] = true;
    }

    void ID()
    {
        ID_EX = IF_ID;
        unsigned inst = ID_EX.cmd;
        ID_EX.opcode = inst & pow[7];
        switch (ID_EX.opcode)
        {
            case 0x37:ID_EX.cmdType = LUI;
                ID_EX.rd = (inst >> 7) & pow[5];
                ID_EX.imm = inst & bi[12];
                break;
            case 0x17:ID_EX.cmdType = AUIPC;
                ID_EX.rd = (inst >> 7) & pow[5];
                ID_EX.imm = inst & bi[12];
                break;
            case 0x6F:ID_EX.cmdType = JAL;
                ID_EX.rd = (inst >> 7) & pow[5];
                ID_EX.imm = ((inst >> 31) & pow[1]) == 1 ? bi[20] : 0;
                ID_EX.imm |= inst & (bi[12] - bi[20]);
                ID_EX.imm |= ((inst >> 20) & pow[1]) << 11u;
                ID_EX.imm |= (inst & (bi[21] - bi[31])) >> 20u;
                pc = ID_EX.pc + ID_EX.imm;
                break;
            case 0x67:ID_EX.cmdType = JALR;
                ID_EX.rd = (inst >> 7) & pow[5];
                ID_EX.func3 = (inst >> 12) & pow[3];
                ID_EX.rs1 = (inst >> 15) & pow[5];
                ID_EX.imm = ((inst >> 31) & pow[1]) == 1 ? bi[11] : 0;
                ID_EX.imm |= (inst >> 20) & pow[11];
                pc = (reg[ID_EX.rs1] + ID_EX.imm) & ~1;
                break;
            case 0x63:ID_EX.func3 = (inst >> 12) & pow[3];
                ID_EX.rs1 = (inst >> 15) & pow[5];
                ID_EX.rs2 = (inst >> 20) & pow[5];
                ID_EX.imm = ((inst >> 31) & pow[1]) == 1 ? bi[12] : 0;
                ID_EX.imm |= ((inst >> 7) & pow[1]) << 11u;
                ID_EX.imm |= (inst & (bi[25] - bi[31])) >> 20u;
                ID_EX.imm |= (inst & (bi[8] - bi[12])) >> 7u;
                switch (ID_EX.func3)
                {
                    case 0:ID_EX.cmdType = BEQ;
                        break;
                    case 1:ID_EX.cmdType = BNE;
                        break;
                    case 4:ID_EX.cmdType = BLT;
                        break;
                    case 5:ID_EX.cmdType = BGE;
                        break;
                    case 6:ID_EX.cmdType = BLTU;
                        break;
                    case 7:ID_EX.cmdType = BGEU;
                }
                //todo
                if (((pd_table[(ID_EX.pc >> 2u) & 31u] & 2) >> 1) == 1) pc = ID_EX.pc + ID_EX.imm;
                break;
            case 0x3:ID_EX.rd = (inst >> 7) & pow[5];
                ID_EX.func3 = (inst >> 12) & pow[3];
                ID_EX.rs1 = (inst >> 15) & pow[5];
                ID_EX.imm = ((inst >> 31) & pow[1]) == 1 ? bi[11] : 0;
                ID_EX.imm |= (inst >> 20) & pow[11];
                switch (ID_EX.func3)
                {
                    case 0:ID_EX.cmdType = LB;
                        break;
                    case 1:ID_EX.cmdType = LH;
                        break;
                    case 2:ID_EX.cmdType = LW;
                        break;
                    case 4:ID_EX.cmdType = LBU;
                        break;
                    case 5:ID_EX.cmdType = LHU;
                }
                break;
            case 0x23:ID_EX.func3 = (inst >> 12) & pow[3];
                ID_EX.rs1 = (inst >> 15) & pow[5];
                ID_EX.rs2 = (inst >> 20) & pow[5];
                ID_EX.imm = ((inst >> 31) & pow[1]) == 1 ? bi[11] : 0;
                ID_EX.imm |= (inst & (bi[25] - bi[31])) >> 20u;
                ID_EX.imm |= (inst & (bi[7] - bi[12])) >> 7u;
                switch (ID_EX.func3)
                {
                    case 0:ID_EX.cmdType = SB;
                        break;
                    case 1:ID_EX.cmdType = SH;
                        break;
                    case 2:ID_EX.cmdType = SW;
                }
                break;
            case 0x13:ID_EX.rd = (inst >> 7) & pow[5];
                ID_EX.func3 = (inst >> 12) & pow[3];
                ID_EX.rs1 = (inst >> 15) & pow[5];
                ID_EX.imm = ((inst >> 31) & pow[1]) == 1 ? bi[11] : 0;
                ID_EX.imm |= (inst >> 20) & pow[11];
                ID_EX.shamt = (inst >> 20) & pow[5];
                ID_EX.func7 = (inst >> 25) & pow[7];
                switch (ID_EX.func3)
                {
                    case 0:ID_EX.cmdType = ADDI;
                        break;
                    case 2:ID_EX.cmdType = SLTI;
                        break;
                    case 3:ID_EX.cmdType = SLTIU;
                        break;
                    case 4:ID_EX.cmdType = XORI;
                        break;
                    case 6:ID_EX.cmdType = ORI;
                        break;
                    case 7:ID_EX.cmdType = ANDI;
                        break;
                    case 1:ID_EX.cmdType = SLLI;
                        break;
                    case 5:ID_EX.cmdType = ID_EX.func7 == 0 ? SRLI : SRAI;
                }
                break;
            case 0x33:ID_EX.rd = (inst >> 7) & pow[5];
                ID_EX.func3 = (inst >> 12) & pow[3];
                ID_EX.rs1 = (inst >> 15) & pow[5];
                ID_EX.rs2 = (inst >> 20) & pow[5];
                ID_EX.func7 = (inst >> 25) & pow[7];
                switch (ID_EX.func3)
                {
                    case 0:ID_EX.cmdType = ID_EX.func7 == 0 ? ADD : SUB;
                        break;
                    case 1:ID_EX.cmdType = SLL;
                        break;
                    case 2:ID_EX.cmdType = SLT;
                        break;
                    case 3:ID_EX.cmdType = SLTU;
                        break;
                    case 4:ID_EX.cmdType = XOR;
                        break;
                    case 5:ID_EX.cmdType = ID_EX.func7 == 0 ? SRL : SRA;
                        break;
                    case 6:ID_EX.cmdType = OR;
                        break;
                    case 7:ID_EX.cmdType = AND;
                }
        }

        //bubble(control hazards)
        if (busy[3] && EX_MEM.rd && (EX_MEM.rd == ID_EX.rs1 || EX_MEM.rd == ID_EX.rs2)) return;

        //forwarding
        ID_EX.vs1 = reg[ID_EX.rs1];
        ID_EX.vs2 = reg[ID_EX.rs2];
        if (ID_EX.rs1 != 0 && ID_EX.rs1 == MEM_WB_rd) ID_EX.vs1 = MEM_WB_vd;
        if (ID_EX.rs2 != 0 && ID_EX.rs2 == MEM_WB_rd) ID_EX.vs2 = MEM_WB_vd;
        MEM_WB_rd = 0;

        busy[1] = false;
        busy[2] = true;
    }

    void EX()
    {
        EX_MEM = ID_EX;
        bool typeB = false, jump = false;
        switch (EX_MEM.cmdType)
        {
            case LUI:EX_MEM.vd = EX_MEM.imm;
                break;
            case AUIPC:EX_MEM.vd = EX_MEM.pc + EX_MEM.imm;
                break;
            case JAL:EX_MEM.vd = EX_MEM.pc + 4;
                break;
            case JALR:EX_MEM.vd = EX_MEM.pc + 4;
                break;
            case BEQ:typeB = true;
                if (EX_MEM.vs1 == EX_MEM.vs2) jump = true;
                break;
            case BNE:typeB = true;
                if (EX_MEM.vs1 != EX_MEM.vs2) jump = true;
                break;
            case BLT:typeB = true;
                if ((int) EX_MEM.vs1 < (int) EX_MEM.vs2) jump = true;
                break;
            case BGE:typeB = true;
                if ((int) EX_MEM.vs1 >= (int) EX_MEM.vs2) jump = true;
                break;
            case BLTU:typeB = true;
                if (EX_MEM.vs1 < EX_MEM.vs2) jump = true;
                break;
            case BGEU:typeB = true;
                if (EX_MEM.vs1 >= EX_MEM.vs2) jump = true;
                break;
            case LB:
            case LH:
            case LW:
            case LBU:
            case LHU:EX_MEM.vd = EX_MEM.vs1 + EX_MEM.imm;
                break;
            case SB:
            case SH:
            case SW:EX_MEM.vd = EX_MEM.vs2;
                EX_MEM.rd = EX_MEM.vs1 + EX_MEM.imm;
                break;
            case ADDI:EX_MEM.vd = EX_MEM.vs1 + EX_MEM.imm;
                break;
            case SLTI:EX_MEM.vd = (int) EX_MEM.vs1 < EX_MEM.imm;
                break;
            case SLTIU:EX_MEM.vd = EX_MEM.vs1 < EX_MEM.imm;
                break;
            case XORI:EX_MEM.vd = EX_MEM.vs1 ^ EX_MEM.imm;
                break;
            case ORI:EX_MEM.vd = EX_MEM.vs1 | EX_MEM.imm;
                break;
            case ANDI:EX_MEM.vd = EX_MEM.vs1 & EX_MEM.imm;
                break;
            case SLLI:EX_MEM.vd = EX_MEM.vs1 << EX_MEM.shamt;
                break;
            case SRLI:EX_MEM.vd = EX_MEM.vs1 >> EX_MEM.shamt;
                break;
            case SRAI:EX_MEM.vd = (int) EX_MEM.vs1 >> EX_MEM.shamt;
                break;
            case ADD:EX_MEM.vd = EX_MEM.vs1 + EX_MEM.vs2;
                break;
            case SUB:EX_MEM.vd = EX_MEM.vs1 - EX_MEM.vs2;
                break;
            case SLL:EX_MEM.vd = EX_MEM.vs1 << (EX_MEM.vs2 & pow[5]);
                break;
            case SLT:EX_MEM.vd = (int) EX_MEM.vs1 < (int) EX_MEM.vs2;
                break;
            case SLTU:EX_MEM.vd = EX_MEM.vs1 < EX_MEM.vs2;
                break;
            case XOR:EX_MEM.vd = EX_MEM.vs1 ^ EX_MEM.vs2;
                break;
            case SRL:EX_MEM.vd = EX_MEM.vs1 >> (EX_MEM.vs2 & pow[5]);
                break;
            case SRA:EX_MEM.vd = (int) EX_MEM.vs1 >> (EX_MEM.vs2 & pow[5]);
                break;
            case OR:EX_MEM.vd = EX_MEM.vs1 | EX_MEM.vs2;
                break;
            case AND:EX_MEM.vd = EX_MEM.vs1 & EX_MEM.vs2;
        }
        if (typeB)
        {
            pd_tot++;
            int idx = (EX_MEM.pc >> 2) & 31u;
            if (jump)
            {
                if (((pd_table[idx] & 2) >> 1) == 1) pd_right++;
                else
                {
                    pc = EX_MEM.pc + EX_MEM.imm;
                    busy[1] = false;
                }
                pd_table[idx] = std::min(pd_table[idx] + 1, 3);
            } else
            {
                if (((pd_table[idx] & 2) >> 1) == 0) pd_right++;
                else
                {
                    pc = EX_MEM.pc + 4;
                    busy[1] = false;
                }
                pd_table[idx] = std::max(pd_table[idx] - 1, 0);
            }
        }
        busy[2] = false;
        busy[3] = true;
    }

    void MEM()
    {
        MEM_WB = EX_MEM;
        switch (MEM_WB.cmdType)
        {
            case LB:MEM_WB.vd = mManager.load(MEM_WB.vd, 1);
                MEM_WB.vd |= (((MEM_WB.vd >> 7) & pow[1]) == 1) ? bi[8] : 0;
                break;
            case LH:MEM_WB.vd = mManager.load(MEM_WB.vd, 2);
                MEM_WB.vd |= (((MEM_WB.vd >> 15) & pow[1]) == 1) ? bi[16] : 0;
                break;
            case LW:MEM_WB.vd = mManager.load(MEM_WB.vd, 4);
                break;
            case LBU:MEM_WB.vd = mManager.load(MEM_WB.vd, 1);
                break;
            case LHU:MEM_WB.vd = mManager.load(MEM_WB.vd, 2);
                break;
            case SB:mManager.store(MEM_WB.rd, MEM_WB.vd, 1);
                break;
            case SH:mManager.store(MEM_WB.rd, MEM_WB.vd, 2);
                break;
            case SW:mManager.store(MEM_WB.rd, MEM_WB.vd, 4);
                break;
            default:break;
        }

        //forwarding
        MEM_WB_rd = MEM_WB.rd;
        MEM_WB_vd = MEM_WB.vd;

        busy[3] = false;
        busy[4] = true;
    }

    void WB()
    {
        switch (MEM_WB.cmdType)
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
            default:if (MEM_WB.rd != 0) reg[MEM_WB.rd] = MEM_WB.vd;
        }
        busy[4] = false;
    }

    void printReg()
    {
        cout << "--------printReg" << endl;
        for (int i = 0; i < 32; ++i)
            cout << std::dec << i << " " << reg[i] << endl;
        cout << "----------------" << endl;
    }

    void printInst(instruction &i)
    {
        cout << "--------printInst" << endl;
        cout << "pc: " << i.pc << endl;
        cout << "cmdType: " << i.cmdType << endl;
        cout << "imm: " << i.imm << " " << "shamt: " << i.shamt << endl;
        cout << "rs1: " << i.rs1 << " " << "vs1: " << i.vs1 << endl;
        cout << "rs2: " << i.rs2 << " " << "vs2: " << i.vs2 << endl;
        cout << "rd: " << i.rd << " " << "vd: " << i.vd << endl;
    }
};

#endif //RISCV_ADVANCED_SIMULATOR_HPP
