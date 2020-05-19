#define _CRT_SECURE_NO_WARNINGS
#include<stdio.h>
#include<stdlib.h>
#include<string.h>

unsigned char SMEM[0x200000];
unsigned char IMEM[0x100000];
unsigned char DMEM[0x100000];
unsigned char stackMEM[0x100000];
unsigned int SP = 0x80000000;
unsigned int PC = 0x00400000;
unsigned int BreakPoint;
unsigned int Reg[32] = { 0, };
unsigned int HI[1] = { 0 };
unsigned int LO[1] = { 0 };

union instructionRegister {
	unsigned int I;
	struct RFormat {
		unsigned int fn : 6;
		unsigned int sh : 5;
		unsigned int rd : 5;
		unsigned int rt : 5;
		unsigned int rs : 5;
		unsigned int op : 6;
	}RI;
	struct IFormat {
		unsigned int off : 16;
		unsigned int rt : 5;
		unsigned int rs : 5;
		unsigned int op : 6;	// offset or operand
	}II;
	struct JFormat {
		unsigned int adr : 26;
		unsigned int op : 6;
	}JI;
}IR;

int load(char filename[]) { //������ �ҷ��´�.
	//FILE* pFile = fopen(filename, "rb");
	PC = 0x400000;
	FILE* pFile = NULL;
	errno_t err;
	unsigned int len = 0;
	//err = fopen_s(&pFile, "as_ex01_arith.bin", "rb");
	//err = fopen_s(&pFile, "as_ex02_logic.bin", "rb");
	//err = fopen_s(&pFile, "as_ex03_ifelse.bin", "rb");

	err = fopen_s(&pFile, filename, "rb");
	if (err) { //������ �ҷ����� �� ������ ��� ������ ����ϰ� 1�� �����Ѵ�.
		printf("Cannot open file\n");
		return 1;
	}
	fseek(pFile, 0, SEEK_END);
	len = ftell(pFile);
	fseek(pFile, 0, SEEK_SET);
	fread(SMEM, sizeof(unsigned char), len, pFile); //���̳ʸ� ������ �о� SMEM�� ����
	int NoI = SMEM[0] << 24 | SMEM[1] << 16 | SMEM[2] << 8 | SMEM[3]; // ��ɾ��� ���� (���� �� �� 4����Ʈ)
	int NoD = SMEM[4] << 24 | SMEM[5] << 16 | SMEM[6] << 8 | SMEM[7]; // �������� ���� (�� ���� 4����Ʈ)

	int i = 8;
	int j = 0;
	while (i < 8 + NoI * 4) { // �ľ��� ��ɾ��� ������ŭ SMEM�� �о� IMEM�� �����Ѵ�.
		IMEM[j] = SMEM[i];
		i += 1;
		j += 1;
	}

	i = 8 + NoI * 4;
	j = 0;
	while (i < 8 + NoI * 4 + NoD * 4) { // �ľ��� �������� ������ŭ SMEM�� �о� DMEM�� �����Ѵ�.
		DMEM[j] = SMEM[i];
		i += 1;
		j += 1;
	}
	fclose(pFile);
	printf("\n");
	printf("File Load is success!\n");
	printf("PC >> 0x400000\n");
	printf("Data >> 0x10000000\n");
	printf("SP >> 0x80000000\n");
}

unsigned int getOp(unsigned int Instruction) {
	IR.I = Instruction;
	IR.RI.op = IR.I >> 26;
	return IR.RI.op;
}
unsigned int getRs(unsigned int Instruction) {
	IR.I = Instruction;
	IR.RI.rs = IR.I >> 21;
	return IR.RI.rs;
}
unsigned int getRt(unsigned int Instruction) {
	IR.I = Instruction;
	IR.RI.rt = IR.I >> 16;
	return IR.RI.rt;
}
unsigned int getRd(unsigned int Instruction) {
	IR.I = Instruction;
	IR.RI.rd = IR.I >> 11;
	return IR.RI.rd;
}
unsigned int getSh(unsigned int Instruction) {
	IR.I = Instruction;
	IR.RI.sh = IR.I >> 6;
	return IR.RI.sh;
}

unsigned int getFn(unsigned int Instruction) {
	IR.I = Instruction;
	IR.RI.fn = IR.I;
	return IR.RI.fn;
}
unsigned int getOff(unsigned int Instruction) {
	IR.I = Instruction;
	IR.II.off = IR.I;
	return IR.II.off;
}
unsigned int getAdr(unsigned int Instruction) {
	IR.I = Instruction;
	IR.JI.adr = IR.I;
	return IR.JI.adr;
}


unsigned int MEM(unsigned int A, int V, int nRW, int S) { //�޸� ���� �Լ�
	unsigned int sel, offset;
	int addr;
	unsigned char* pM;
	sel = A >> 20; // �ּҸ� �о �����Ͱ� ��� �޸𸮸� ����ų�� �����Ѵ�.
	offset = A & 0xFFFFF; // �о�� �ּҿ� ��ƮAND�����Ͽ� �޸��� �ε����� �����Ѵ�.

	if (sel == 0x004) pM = IMEM;

	else if (sel == 0x100) pM = DMEM;

	else if (sel == 0x7FF) {
		pM = stackMEM;
		offset = ((~A) << 16) >> 16; // ���ø޸𸮴� �Ųٷ� �����ϹǷ� ��ƮXOR�����Ͽ� �޸��� �ε����� �����Ѵ�.
	}

	else {
		printf("No memory\n");
		exit(1);
	}

	if (S == 0) {
		if (nRW == 0) {
			return *(pM + offset + 3);
		}
		else if (nRW == 1) {
			*(pM + offset + 3) = (V << 24) >> 24; //4����Ʈ value�� �� �ڿ� �ִ� ������ �д´�.
		}
	}
	else if (S == 1) {
		if (nRW == 0) {
			return *(pM + offset + 2) << 8 | *(pM + offset + 3);
		}
		else if (nRW == 1) {
			*(pM + offset + 2) = (V << 16) >> 24;
			*(pM + offset + 3) = (V << 24) >> 24;
		}
	}
	else if (S == 2) {
		if (nRW == 0) {
			return *(pM + offset) << 24 | *(pM + offset + 1) << 16 | *(pM + offset + 2) << 8 | *(pM + offset + 3);
		}
		else if (nRW == 1) {
			*(pM + offset) = V >> 24;
			*(pM + offset + 1) = (V << 8) >> 24;
			*(pM + offset + 2) = (V << 16) >> 24;
			*(pM + offset + 3) = (V << 24) >> 24;
		}
	}
}

unsigned int REG(unsigned int A, unsigned int V, unsigned int nRW) {
	if (nRW == 0) return Reg[A];		//�б� ���: �ش� ��������($A)�� �����Ͽ� ���� �д´�.
	if (nRW == 1) Reg[A] = V;		//���� ���: �ش� ��������($A)�� ���ο� ���� ����.
}

int ALU(unsigned int type, unsigned int op, unsigned int rs, unsigned int rt, unsigned int rd, unsigned int shamt, unsigned int fn, unsigned int offset, unsigned int address) {
	//�������� ���� ������ �����ε� ����. ex) R[15] == R[0xf] == R[0b1111]
	if (type == 0) {

		if (fn == 0b000000)//sll
			REG(rd, (REG(rt, 0, 0) << shamt), 1);
		if (fn == 0b000010)//srl
			REG(rd, (REG(rt, 0, 0) >> shamt), 1);
		if (fn == 0b000011) {//sra
			if (REG(rt, 0, 0) >> 31 == 0x00000001)//��ȣ��Ʈ�� 1�� ��쿡
				REG(rd, (REG(rt, 0, 0) >> shamt | (0b11111111111111111111111111111111 << (32 - shamt))), 1);//���� 1�� ä��� ����Ʈ�ϱ� ����
			else//��ȣ��Ʈ�� 0�� ��쿡
				REG(rd, (REG(rt, 0, 0) >> shamt), 1);//��ȣ �Ű澲���ʰ� �̵�
		}
		if (fn == 0b001000)//jr
			PC = REG(rs, 0, 0);
		if (fn == 0b001100)//syscall
			return 0;  // print("syscall"), return 0;
		if (fn == 0b010000)//mfhi
			REG(rd, HI[0], 1);
		if (fn == 0b010010)//mflo
			REG(rd, LO[0], 1);
		if (fn == 0b011000)//mul, ���x���� ������ �س���
			REG(rd, (REG(rs, 0, 0) * REG(rt, 0, 0)), 1);
		if (fn == 0b100000)//add
			REG(rd, (REG(rs, 0, 0) + REG(rt, 0, 0)), 1);
		if (fn == 0b100001)//addu
			REG(rd, (REG(rs, 0, 0) + REG(rt, 0, 0)), 1);
		if (fn == 0b100010)//sub
			REG(rd, (REG(rs, 0, 0) - REG(rt, 0, 0)), 1);
		if (fn == 0b100100)//and
			REG(rd, (REG(rs, 0, 0) & REG(rt, 0, 0)), 1);
		if (fn == 0b100101)//or
			REG(rd, (REG(rs, 0, 0) | REG(rt, 0, 0)), 1);
		if (fn == 0b100110)//xor
			REG(rd, (REG(rs, 0, 0) ^ REG(rt, 0, 0)), 1);
		if (fn == 0b100111)//nor
			REG(rd, ~(REG(rs, 0, 0) | REG(rt, 0, 0)), 1);
		if (fn == 0b101010) {//slt rs<rt�� ���̸� rd�� 1 ����, �����̸� 0 ����
			int a, b;//unsigned���� signed�� �ٲ㼭 �����Ŵ
			a = REG(rs, 0, 0);
			b = REG(rt, 0, 0);
			if (a < b)
				REG(rd, 1, 1);
			else
				REG(rd, 0, 1);
		}
	}//RŸ��

	else if (type == 1) {

		if (op == 0b000001) {//bltz 
			if (REG(rs, 0, 0) >> 31 == 0x1) // (REG(rs, 0, 0) < 0 )���� �������� �ʾ� �ڵ� ������
				PC = PC + (offset * 4) - 4;//-4 �߰�
		}
		if (op == 0b000100) {//beq 
			if (REG(rs, 0, 0) == REG(rt, 0, 0))
				PC = PC + (offset * 4) - 4;//-4 �߰�
		}
		if (op == 0b000101) {//bne
			if (REG(rs, 0, 0) != REG(rt, 0, 0))
				PC = PC + (offset * 4) - 4;//-4 �߰�
		}
		if (op == 0b001000) {//addi
			if (offset >> 15 == 0x1)//��ģ�������� tc1�� ori�� qtspim�� �ٸ�
				offset = (0xffff0000 | offset);
			REG(rt, (REG(rs, 0, 0) + offset), 1);
		}
		if (op == 0b001010) {//slti
			if (offset >> 15 == 0x1)//��ģ�������� tc1�� ori�� qtspim�� �ٸ�
				offset = (0xffff0000 | offset);
			int a, b;//unsigned���� signed�� �ٲ㼭 �����Ŵ
			a = (REG(rs, 0, 0));
			b = offset;
			if (a < b)
				REG(rt, 1, 1);
			else
				REG(rt, 0, 1);
		}
		if (op == 0b001100)//andi
			REG(rt, (REG(rs, 0, 0) & offset), 1);
		if (op == 0b001101)//ori
			REG(rt, (REG(rs, 0, 0) | offset), 1);
		if (op == 0b001110)//xori
			REG(rt, (REG(rs, 0, 0) ^ offset), 1);
		if (op == 0b001111)//lui
			REG(rt, (offset << 16), 1);
		if (op == 0b100000) {//lb
			if ((MEM((REG(rs, 0, 0) + offset), 0, 0, 2) >> 31) == 0b00000001)//��ȣ��Ʈ�� 1�̸�
				REG(rt, (0b11111111111111111111111100000000 | MEM(REG(rs, 0, 0) + offset, 0, 0, 0)), 1);
			else//��ȣ��Ʈ�� 0�̸�
				REG(rt, MEM(REG(rs, 0, 0) + offset, 0, 0, 0), 1);
		}
		if (op == 0b100011)//lw
			REG(rt, MEM(REG(rs, 0, 0) + offset, 0, 0, 2), 1);
		if (op == 0b100100)//lbu
			REG(rt, MEM(REG(rs, 0, 0) + offset, 0, 0, 0), 1);
		if (op == 0b101000)//sb
			MEM(REG(rs, 0, 0) + offset, REG(rt, 0, 0), 1, 0);
		if (op == 0b101011)//sw
			MEM(REG(rs, 0, 0) + offset, REG(rt, 0, 0), 1, 2);

	}//IŸ��

	else if (type == 2) {
		if (op == 0b000010)//j
			PC = address * 4;
		if (op == 0b000011) {//jal
			REG(31, PC, 1);//R[31] == $ra
			PC = address * 4;
		}
	}//JŸ��
}


void Rtype(unsigned int IR) {
	unsigned int fn;
	unsigned int rs;
	unsigned int rt;
	unsigned int rd;
	unsigned int sh;
	fn = getFn(IR); rs = getRs(IR); rt = getRt(IR); rd = getRd(IR); sh = getSh(IR);
	ALU(0, 0, rs, rt, rd, sh, fn, 0, 0);
}
void Itype(unsigned int IR) {
	unsigned int op;
	unsigned int rs;
	unsigned int rt;
	unsigned int off;
	op = getOp(IR); rs = getRs(IR); rt = getRt(IR); off = getOff(IR);
	ALU(1, op, rs, rt, 0, 0, 0, off, 0);
}
void Jtype(unsigned int IR) {
	unsigned int op;
	unsigned int adr;
	op = getOp(IR); adr = getAdr(IR);
	ALU(2, op, 0, 0, 0, 0, 0, 0, adr);
}


void jump() {

}

int step() { //PC�� �ּҿ� �ִ� �޸𸮿� �����Ͽ� ��ɾ �ؼ��� �� �����Ѵ�.
	IR.I = MEM(PC, 0, 0, 2); PC += 4;
	if (PC - 4 == BreakPoint) {
		printf("Break Point!! >> PC: %x\n", BreakPoint);
		return 0;
	}
	unsigned int op = getOp(IR.I);
	unsigned int fn = getFn(IR.I);
	const char* ins;
	if (IR.I == 0x00000000) {
		printf("No Instruction!\n");
		return 0;
	}
	if (op == 0x0) {
		//�ӽ� �ڵ�
		printf("PC : %x\n", PC - 4);
		switch (fn) {
		case 0x0:
			ins = "sll";
			Rtype(IR.I);
			break;
		case 0x2:
			ins = "srl";
			Rtype(IR.I);
			break;
		case 0x3:
			ins = "sra";
			Rtype(IR.I);
			break;
		case 0x8:
			ins = "jr";
			Rtype(IR.I);
			break;
		case 0xc: //syscall
			ins = "syscall";
			Rtype(IR.I);
			printf("opcode: %2x, rs: %2d, rt: %2d, rd:%2d, sht:%2d, function: %2x, Inst: %s\n", op, IR.RI.rs, IR.RI.rt, IR.RI.rd, IR.RI.sh, fn, ins);
			return 0;
		case 0x10:
			ins = "mfhi";
			Rtype(IR.I);
			break;
		case 0x12:
			ins = "mflo";
			Rtype(IR.I);
			break;
		case 0x18:
			ins = "mul";
			Rtype(IR.I);
			break;
		case 0x20:
			ins = "add";
			Rtype(IR.I);
			break;
		case 0x21:
			ins = "addu";
			Rtype(IR.I);
			break;
		case 0x22:
			ins = "sub";
			Rtype(IR.I);
			break;
		case 0x24:
			ins = "and";
			Rtype(IR.I);
			break;
		case 0x25:
			ins = "or";
			Rtype(IR.I);
			break;
		case 0x26:
			ins = "xor";
			Rtype(IR.I);
			break;
		case 0x27:
			ins = "nor";
			Rtype(IR.I);
			break;
		case 0x2a:
			ins = "slt";
			Rtype(IR.I);
			break;
		default:
			ins = "Undefined Instruction!";
			break;
		}
		printf("opcode: %2x, rs: %2d, rt: %2d, rd:%2d, sht:%2d, function: %2x, Inst: %s\n", op, IR.RI.rs, IR.RI.rt, IR.RI.rd, IR.RI.sh, fn, ins);
	}
	else {
		//�ӽ� �ڵ�
		printf("PC : %x\n", PC - 4);
		switch (op) {
		case 0x1:
			ins = "bltz";
			Itype(IR.I);
			printf("opcode: %2x, rs: %2d, rt: %2d, operand or offset: %2x, Inst: %s\n", op, IR.RI.rs, IR.RI.rt, IR.II.off, ins);
			break;
		case 0x2:
			ins = "j";
			Jtype(IR.I);
			printf("opcode: %2x, address: %2x, Inst: %s\n", op, IR.JI.adr, ins);
			break;
		case 0x3:
			ins = "jal";
			Jtype(IR.I);
			printf("opcode: %2x, address: %2x, Inst: %s\n", op, IR.JI.adr, ins);
			break;
		case 0x4:
			ins = "beq";
			Itype(IR.I);
			printf("opcode: %2x, rs: %2d, rt: %2d, operand or offset: %2x, Inst: %s\n", op, IR.RI.rs, IR.RI.rt, IR.II.off, ins);
			break;
		case 0x5:
			ins = "bne";
			Itype(IR.I);
			printf("opcode: %2x, rs: %2d, rt: %2d, operand or offset: %2x, Inst: %s\n", op, IR.RI.rs, IR.RI.rt, IR.II.off, ins);
			break;
		case 0x8:
			ins = "addi";
			Itype(IR.I);
			printf("opcode: %2x, rs: %2d, rt: %2d, operand or offset: %2x, Inst: %s\n", op, IR.RI.rs, IR.RI.rt, IR.II.off, ins);
			break;
		case 0xa:
			ins = "slti";
			Itype(IR.I);
			printf("opcode: %2x, rs: %2d, rt: %2d, operand or offset: %2x, Inst: %s\n", op, IR.RI.rs, IR.RI.rt, IR.II.off, ins);
			break;
		case 0xc:
			ins = "andi";
			Itype(IR.I);
			printf("opcode: %2x, rs: %2d, rt: %2d, operand or offset: %2x, Inst: %s\n", op, IR.RI.rs, IR.RI.rt, IR.II.off, ins);
			break;
		case 0xd:
			ins = "ori";
			Itype(IR.I);
			printf("opcode: %2x, rs: %2d, rt: %2d, operand or offset: %2x, Inst: %s\n", op, IR.RI.rs, IR.RI.rt, IR.II.off, ins);
			break;
		case 0xe:
			ins = "xori";
			Itype(IR.I);
			printf("opcode: %2x, rs: %2d, rt: %2d, operand or offset: %2x, Inst: %s\n", op, IR.RI.rs, IR.RI.rt, IR.II.off, ins);
			break;
		case 0xf:
			ins = "lui";
			Itype(IR.I);
			printf("opcode: %2x, rs: %2d, rt: %2d, operand or offset: %2x, Inst: %s\n", op, IR.RI.rs, IR.RI.rt, IR.II.off, ins);
			break;
		case 0x20:
			ins = "lb";
			Itype(IR.I);
			printf("opcode: %2x, rs: %2d, rt: %2d, operand or offset: %2x, Inst: %s\n", op, IR.RI.rs, IR.RI.rt, IR.II.off, ins);
			break;
		case 0x23:
			ins = "lw";
			Itype(IR.I);
			printf("opcode: %2x, rs: %2d, rt: %2d, operand or offset: %2x, Inst: %s\n", op, IR.RI.rs, IR.RI.rt, IR.II.off, ins);
			break;
		case 0x24:
			ins = "lbu";
			Itype(IR.I);
			printf("opcode: %2x, rs: %2d, rt: %2d, operand or offset: %2x, Inst: %s\n", op, IR.RI.rs, IR.RI.rt, IR.II.off, ins);
			break;
		case 0x28:
			ins = "sb";
			Itype(IR.I);
			printf("opcode: %2x, rs: %2d, rt: %2d, operand or offset: %2x, Inst: %s\n", op, IR.RI.rs, IR.RI.rt, IR.II.off, ins);
			break;
		case 0x2b:
			ins = "sw";
			Itype(IR.I);
			printf("opcode: %2x, rs: %2d, rt: %2d, operand or offset: %2x, Inst: %s\n", op, IR.RI.rs, IR.RI.rt, IR.II.off, ins);
			break;
		default:
			ins = "Undefined Instruction!";
			break;
		}
	}
}

void go() { //���α׷��� ������ �����Ѵ�.
	while (1) {
		if (step() == 0) break;
	}
}

void setPC(unsigned int val) {
	PC = val;
	return;
}

void showRegister(void) {
	int i;
	printf("[REGISTER]\n");
	for (i = 0; i < 32; i++) {
		printf("REG[%d] = %x", i, Reg[i]);
		printf("\n");
	}
	printf("PC = %x\n", PC);
}


void showMEM(unsigned int start, unsigned int end) { //�޸� ���� �Լ�
	unsigned int sel, offset, limit;
	unsigned char* pM;
	int showAddr;
	sel = start >> 20; // �ּҸ� �о �����Ͱ� ��� �޸𸮸� ����ų�� �����Ѵ�.
	offset = start & 0xFFFFF;
	limit = end & 0xFFFFF;
	showAddr = start;

	if (sel == 0x004) pM = IMEM;

	else if (sel == 0x100) pM = DMEM;

	else if (sel == 0x7FF) {
		pM = stackMEM;
		offset = ((~start) << 16) >> 16; // ���ø޸𸮴� �Ųٷ� �����ϹǷ� ��ƮXOR�����Ͽ� �޸��� �ε����� �����Ѵ�.
		limit = ((~end) << 16) >> 16;
	}

	else {
		printf("No memory\n");
		return;
	}
	
		for (offset; offset <= limit; offset += 1) {
			printf("%x = %x\n", showAddr, *pM);
			showAddr += 1;
			pM += 1;
		}
}

int main() {
	char input = 0;
	char base_input[3], filename[40];
	int num, adr;
	unsigned int start, end;
	unsigned int tempPC, tempBP;
	Reg[29] = SP;
	while (1) {
		printf("\n");
		printf("����� ��ɾ �����Ͽ� �ֽʽÿ�.\n1. ���Ϸε� =>[l]\t2. PC�������� =>[j]\t3. PC�ʱ�ȭ =>[i]\n4. Break���� =>[b]\t5. �����ϰ����� =>[g]\t6. ���ϴܰ������� =>[s]\n7. �޸�Ȯ�� =>[m]\t8. ��������Ȯ�� =>[r]\t9. ������������ =>[sr]\n10. �޸����� =>[sm]\t11. ���α׷����� =>[x]\n");
		printf(">> ");
		scanf_s("%s", base_input, 3); // ���� �� ���� ����
		input = base_input[0]; // input�� �� �� ����

		if ( base_input[1] != 0 ) {  // �� ���ڰ� �ƴ� �� ���ڸ� ���� ���

			if ((strcmp(base_input, "sr") == 0) || (strcmp(base_input, "SR") == 0)) { 
				printf("������ ���������� �ּҸ� �Է����ֽʽÿ�. (0~31)\n");
				printf(">> ");
				scanf_s("%d", &adr);
				printf("���� �Է����ֽʽÿ�. ��) 0x1234abcd\n");
				printf(">> ");
				scanf_s("%x", &num);
				REG(adr, num, 1);
				printf("Reg[%d] = %x\n", adr, REG(adr, num, 0));
				continue;
			}
			else if ((strcmp(base_input, "sm") == 0) || (strcmp(base_input, "SM") == 0)) { 
				printf("������ �޸��� �ּҸ� �Է����ֽʽÿ�. ��) 0x10000000\n");
				printf(">> ");
				scanf_s("%x", &adr);
				printf("���� �Է����ֽʽÿ�. ��) 0xab\n");
				printf(">> ");
				scanf_s("%x", &num);
				MEM(adr-3, num, 1, 0);
				showMEM(adr, adr);
				continue;
			}
			else {
				printf("��ɾ �߸� �Է��ϼ̽��ϴ�.\n");
			}
		}

		else {
			if ((input == 'l') || (input == 'L')) {
				printf("���� �̸��� �Է��Ͻÿ�.\n");
				printf(">> ");
				scanf_s("%s", filename, 40);
				load(filename);
				//���� �̸����� ���͸� �ϼ��Ͽ� �ش� ��ġ�� ���� �ε�
			}
			else if ((input == 'j') || (input == 'J')) {
				printf("PC���� �Է��Ͻʽÿ�. ��) 0x400000\n");
				printf(">> ");
				scanf_s("%x", &tempPC);
				setPC(tempPC);
			}
			else if ((input == 'i') || (input == 'I')) {
				printf("PC���� �ʱ�ȭ�Ǿ����ϴ�.\n");
				PC = 0x400000;
			}
			else if ((input == 'b') || (input == 'B')) {
				printf("Break Point���� �Է��Ͻʽÿ�. ��) 0x400000\n");
				printf(">> ");
				scanf_s("%x", &tempBP);
				while (tempBP % 4 != 0) {
					printf("�߸� �Է��ϼ̽��ϴ�. �ٽ� �Է��Ͻʽÿ�. ��) 0x400000\n");
					printf(">> ");
					scanf_s("%x", &tempBP);
				}
				BreakPoint = tempBP;
			}
			else if ((input == 'g') || (input == 'G')) {
				go(); //break point���� �����Ǵ� �� ����
			}
			else if ((input == 's') || (input == 'S')) {
				step(); //����� ��������, �޸� ���� ����
			}
			else if ((input == 'm') || (input == 'M')) {
				printf("����� ������ �Է����ֽʽÿ�. ��) 0x10000000 0x10000040\n");
				printf(">> ");
				scanf_s("%x %x", &start, &end);
				showMEM(start, end);
			}
			else if ((input == 'r') || (input == 'R')) {
				showRegister();
			}
			else if ((input == 'x') || (input == 'X')) {
				break;
			}
			else {
				printf("��ɾ �߸� �Է��ϼ̽��ϴ�.\n");
			}
		}
		while (getchar() != '\n');
	}
}