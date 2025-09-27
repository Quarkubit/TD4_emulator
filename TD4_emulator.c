#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <locale.h>
#include <windows.h>
#include <conio.h>
#include <stdbool.h>

#define MEMORY_SIZE 16

// ��������� ��� ��������� TD4
typedef struct {
    uint8_t memory[MEMORY_SIZE];
    uint8_t A, B; // ��������
    uint8_t C; // ���� ��������
    uint8_t PC; // ����������� ������
    uint8_t IN_line, OUT_line; // ����� ����� � ������
} TD4Emulator;

// ��������� ��� �������� ���������� � �������
typedef struct {
    const char* name;
    uint8_t opcode;
    int has_immediate;
} CommandInfo;


// ����������� 4-������ �������� � ������ ���� "1011"
void to_binary_str(uint8_t value, char* buffer) {
    for (int i = 3; i >= 0; i--) {
        if ((value >> i) & 1) {
            buffer[3 - i] = '1';
        }
        else {
            buffer[3 - i] = '0';
        }
        //buffer[3 - i] = ((value >> i) & 1) ? '1' : '0';
    }
    buffer[4] = '\0';
}

// ������� ������ TD4
CommandInfo commands[] = {
    {"ADD A", 0x00, 1},
    {"MOV A, B", 0x10, 0},
    {"IN A", 0x20, 0},
    {"MOV A", 0x30, 1},
    {"MOV B, A", 0x40, 0},
    {"ADD B", 0x50, 1},
    {"IN B", 0x60, 0},
    {"MOV B", 0x70, 1},
    {"OUT B", 0x90, 0},
    {"OUT", 0xB0, 1},
    {"JNC", 0xE0, 1},
    {"JMP", 0xF0, 1}
};

#define NUM_COMMANDS (sizeof(commands) / sizeof(commands[0]))

// ������� ��� �������� ��������� �� �����
int load_program(TD4Emulator* emu, const char* filename) {
    FILE* file;
    errno_t err = fopen_s(&file, filename, "rb");
    if (err != 0 || file == NULL) {
        printf("������ �������� �����: %s\n", filename);
        return 0;
    }

    // ������ ��������� � ������
    fread(emu->memory, 1, MEMORY_SIZE, file);

    if (ferror(file)) {
        printf("������ ������ �����\n");
        fclose(file);
        return 0;
    }

    fclose(file);
    return 1;
}

// ������� ��� ��������� ��������� �������
void get_mnemonic(uint8_t instruction, char* buffer) {
    uint8_t opcode = instruction & 0xF0;
    uint8_t im = instruction & 0x0F;

    for (int i = 0; i < NUM_COMMANDS; i++) {
        if (commands[i].opcode == opcode) {
            if (commands[i].has_immediate) {
                char bin_im[5];
                to_binary_str(im, bin_im);
                sprintf_s(buffer, 50, "%s %s", commands[i].name, bin_im);
            }
            else {
                strcpy_s(buffer, 50, commands[i].name);
            }
            return;
        }
    }

    // ���� ������� �� �������
    sprintf_s(buffer, 50, "UNKNOWN %02X", instruction);
}

// ������� ���������� ����������
void execute_instruction(TD4Emulator* emu) {
    uint8_t instruction = emu->memory[emu->PC];
    uint8_t opcode = instruction & 0xF0;
    uint8_t im = instruction & 0x0F;

    switch (opcode) 
    {
        case 0x00: // ADD A, im
        {
            uint16_t result = emu->A + im;
            emu->A = result & 0x0F;
            emu->C = (result > 0x0F) ? 1 : 0;
            emu->PC = (emu->PC + 1) & 0x0F;
            break;
        }

        case 0x10: // MOV A, B
        {
            emu->A = emu->B;
            emu->C = 0;
            emu->PC = (emu->PC + 1) & 0x0F;
            break;
        }

        case 0x20: // IN A
        {
            emu->A = emu->IN_line;
            emu->C = 0;
            emu->PC = (emu->PC + 1) & 0x0F;
            break;
        }

        case 0x30: // MOV A, im
        {
            emu->A = im;
            emu->C = 0;
            emu->PC = (emu->PC + 1) & 0x0F;
            break;
        }

        case 0x40: // MOV B, A
        {
            emu->B = emu->A;
            emu->C = 0;
            emu->PC = (emu->PC + 1) & 0x0F;
            break;
        }

        case 0x50: // ADD B, im
        {
            uint16_t result = emu->B + im;
            emu->B = result & 0x0F;
            emu->C = (result > 0x0F) ? 1 : 0;
            emu->PC = (emu->PC + 1) & 0x0F;
            break;
        }

        case 0x60: // IN B
        {
            emu->B = emu->IN_line;
            emu->C = 0;
            emu->PC = (emu->PC + 1) & 0x0F;
            break;
        }

        case 0x70: // MOV B, im
        {
            emu->B = im;
            emu->C = 0;
            emu->PC = (emu->PC + 1) & 0x0F;
            break;
        }

        case 0x90: // OUT B
        {
            emu->OUT_line = emu->B;
            emu->C = 0;
            emu->PC = (emu->PC + 1) & 0x0F;
            break;
        }

        case 0xB0: // OUT im
        {
            emu->OUT_line = im;
            emu->C = 0;
            emu->PC = (emu->PC + 1) & 0x0F;
            break;
        }

        case 0xE0: // JNC im
        {
            if (emu->C == 0) {
                emu->PC = im;
            }
            else {
                emu->PC = (emu->PC + 1) & 0x0F;
            }
            break;
        }

        case 0xF0: // JMP im
        {
            emu->PC = im;
            break;
        }

        default:
        {
            printf("����������� �������: %02X\n\'", instruction, "\'");
            emu->PC = (emu->PC + 1) & 0x0F;
            break;
        }
    }
}

// ������� ��� ������ ��������� ���������� � �������� ����
void print_state(TD4Emulator* emu, int cycle) {
    uint8_t instruction = emu->memory[emu->PC];
    char mnemonic[50];
    get_mnemonic(instruction, mnemonic);

    char bin_PC[5], bin_A[5], bin_B[5], bin_OUT[5], bin_IN[5], bin_C[2];
    to_binary_str(emu->PC & 0x0F, bin_PC);
    to_binary_str(emu->A & 0x0F, bin_A);
    to_binary_str(emu->B & 0x0F, bin_B);
    to_binary_str(emu->OUT_line & 0x0F, bin_OUT);
    to_binary_str(emu->IN_line & 0x0F, bin_IN);
    sprintf_s(bin_C, sizeof(bin_C), "%d", emu->C);

    printf("����: %2d | PC: %s | �������: %02X (%s) | A: %s | B: %s | OUT: %s | IN: %s | C: %s\n",
        cycle, bin_PC, instruction, mnemonic, bin_A, bin_B, bin_OUT, bin_IN, bin_C);
}

// ������� ��� ����������� ����������� ������ � �������� ����
void print_memory(TD4Emulator* emu) {
    printf("\n���������� ������ (� �������� ����):\n");
    for (int i = 0; i < MEMORY_SIZE; i++) {
        if (i % 8 == 0) {
            printf("\n%02X: ", i);
        }
        char bin_val[5];
        to_binary_str(emu->memory[i] & 0x0F, bin_val);
        printf("%s ", bin_val);
    }
    printf("\n");
}

// ������� ��� ����������� �������
void print_help() {
    printf("\n���������� ����������:\n");
    printf("  Enter - ��������� ��������� ������� (� ������ ������)\n");
    printf("  'a'   - ������������� � �������������� �����\n");
    printf("  'm'   - ������������� � ������ �����\n");
    printf("  's'   - �������� ���������� ������\n");
    printf("  'i'   - �������� �������� �������� �����\n");
    printf("  'r'   - �������� ���������\n");
    printf("  'h'   - �������� �������\n");
    printf("  'q'   - ����� �� ���������\n");
}

int main() {
    // ��������� ������� ������ � ������� �������� �������
    setlocale(LC_ALL, "Russian");
    SetConsoleCP(1251);
    SetConsoleOutputCP(1251);

    TD4Emulator emu;
    memset(&emu, 0, sizeof(emu));

    char filename[256];
    printf("������� ��� ����� � ����������: ");
    scanf_s("%255s", filename, (unsigned)_countof(filename));

    // ��������� ���������� .bin ���� ��� ���
    if (strstr(filename, ".") == NULL) {
        strcat_s(filename, sizeof(filename), ".bin");
    }

    // �������� ���������
    if (!load_program(&emu, filename)) {
        printf("������� ����� ������� ��� ������...");
        _getch();
        return 1;
    }

    printf("��������� ������� ��������� �� ����� %s\n", filename);

    int cycle = 0;
    int auto_mode = 0;
    int delay_ms = 1000;

    printf("�������� ���������� TD4\n");
    print_help();
    printf("\n������� ����� ������� ��� ������...");
    _getch();

    // �������� ���� ���������
    while (1) {
        system("cls");

        printf("�������� ���������� TD4 - %s\n", auto_mode ? "�������������� �����" : "������ �����");
        printf("����: %s\n", filename);
        print_state(&emu, cycle);

        if (auto_mode) {
            printf("\n�������������� �����. ��������: %d ��\n", delay_ms);
            printf("������� 'm' ��� �������� � ������ ����� ��� 'q' ��� ������\n");

            execute_instruction(&emu);
            cycle++;

            Sleep(delay_ms);
        }
        else {
            printf("\n������ �����. ������� Enter ��� ���������� �������\n");
            printf("��� �������� ��������: a-����, s-������, i-����, r-�����, h-�������, q-�����\n");
            while (_kbhit()==NULL){}
        }

        // ��������� �����
        if (_kbhit()) {
            int ch = _getch();

            switch (ch) {
            case '\r': // Enter
                if (!auto_mode) 
                {
                    execute_instruction(&emu);
                    cycle++;
                }
                break;

            case 'a':
            case 'A':
                auto_mode = 1;
                printf("\n������������ � �������������� �����\n");
                Sleep(1000);
                break;

            case 'm':
            case 'M':
                auto_mode = 0;
                printf("\n������������ � ������ �����\n");
                Sleep(1000);
                break;

            case 's':
            case 'S':
                print_memory(&emu);
                printf("������� ����� ������� ��� �����������...");
                _getch();
                break;

            case 'i':
            case 'I':
            {
                printf("\n������� �������� �������� �����: %X (��������: ", emu.IN_line);
                char bin_cur[5];
                to_binary_str(emu.IN_line, bin_cur);
                printf("%s)\n", bin_cur);
                printf("������� ����� �������� (0-15 ��� 4-������ ��������, �������� 1011): ");

                char input[10];
                scanf_s("%9s", input, (unsigned)_countof(input));
                while (getchar() != '\n'); // ������� ������

                int value = -1;
                int len = (int)strlen(input);

                // ������� ���������������� ��� �������� (����� 4 �������, ������ 0/1)
                if (len == 4) {
                    bool is_binary = true;
                    for (int i = 0; i < 4; i++) {
                        if (input[i] != '0' && input[i] != '1') {
                            is_binary = false;
                            break;
                        }
                    }
                    if (is_binary) {
                        // ����������� �������� ������ � �����
                        value = 0;
                        for (int i = 0; i < 4; i++) {
                            value = (value << 1) | (input[i] - '0');
                        }
                    }
                }

                // ���� �� �������� � ������� ��� ����������
                if (value == -1) {
                    char* end;
                    long dec_val = strtol(input, &end, 10);
                    if (*end == '\0' && dec_val >= 0 && dec_val <= 15) {
                        value = (int)dec_val;
                    }
                }

                if (value >= 0 && value <= 15) {
                    emu.IN_line = (uint8_t)value;
                    char bin_new[5];
                    to_binary_str(emu.IN_line, bin_new);
                    printf("�������� �������� ����� �������� ��: %d (��������: %s)\n", value, bin_new);
                }
                else {
                    printf("������: ������� ����� �� 0 �� 15 ��� 4-������ �������� ������ (��������, 1011)!\n");
                }

                printf("������� ����� ������� ��� �����������...");
                _getch();
                break;
            }

            case 'r':
            case 'R':
                memset(&emu, 0, sizeof(emu));
                load_program(&emu, filename);
                cycle = 0;
                printf("\n��������� �������. ��������� ����� ����� �������������.\n");
                Sleep(2000);
                break;

            case 'h':
            case 'H':
                print_help();
                printf("������� ����� ������� ��� �����������...");
                _getch();
                break;

            case 'q':
            case 'Q':
                printf("\n����� �� ���������\n");
                return 0;
            }
        }
    }

    return 0;
}