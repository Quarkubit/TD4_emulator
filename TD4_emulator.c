#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <locale.h>
#include <windows.h>
#include <conio.h>
#include <stdbool.h>

#define MEMORY_SIZE 16

// Структура для эмулятора TD4
typedef struct {
    uint8_t memory[MEMORY_SIZE];
    uint8_t A, B; // Регистры
    uint8_t C; // Флаг переноса
    uint8_t PC; // Выполняемая строка
    uint8_t IN_line, OUT_line; // Линии входа и выхода
} TD4Emulator;

// Структура для хранения информации о команде
typedef struct {
    const char* name;
    uint8_t opcode;
    int has_immediate;
} CommandInfo;


// Преобразует 4-битное значение в строку вида "1011"
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

// Таблица команд TD4
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

// Функция для загрузки программы из файла
int load_program(TD4Emulator* emu, const char* filename) {
    FILE* file;
    errno_t err = fopen_s(&file, filename, "rb");
    if (err != 0 || file == NULL) {
        printf("Ошибка открытия файла: %s\n", filename);
        return 0;
    }

    // Читаем программу в память
    fread(emu->memory, 1, MEMORY_SIZE, file);

    if (ferror(file)) {
        printf("Ошибка чтения файла\n");
        fclose(file);
        return 0;
    }

    fclose(file);
    return 1;
}

// Функция для получения мнемоники команды
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

    // Если команда не найдена
    sprintf_s(buffer, 50, "UNKNOWN %02X", instruction);
}

// Функция выполнения инструкции
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
            printf("Неизвестная команда: %02X\n\'", instruction, "\'");
            emu->PC = (emu->PC + 1) & 0x0F;
            break;
        }
    }
}

// Функция для вывода состояния процессора в двоичном виде
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

    printf("Такт: %2d | PC: %s | Команда: %02X (%s) | A: %s | B: %s | OUT: %s | IN: %s | C: %s\n",
        cycle, bin_PC, instruction, mnemonic, bin_A, bin_B, bin_OUT, bin_IN, bin_C);
}

// Функция для отображения содержимого памяти в двоичном виде
void print_memory(TD4Emulator* emu) {
    printf("\nСодержимое памяти (в двоичном виде):\n");
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

// Функция для отображения справки
void print_help() {
    printf("\nУправление эмулятором:\n");
    printf("  Enter - выполнить следующую команду (в ручном режиме)\n");
    printf("  'a'   - переключиться в автоматический режим\n");
    printf("  'm'   - переключиться в ручной режим\n");
    printf("  's'   - показать содержимое памяти\n");
    printf("  'i'   - изменить значение входного порта\n");
    printf("  'r'   - сбросить процессор\n");
    printf("  'h'   - показать справку\n");
    printf("  'q'   - выйти из эмулятора\n");
}

int main() {
    // Установка русской локали и кодовой страницы консоли
    setlocale(LC_ALL, "Russian");
    SetConsoleCP(1251);
    SetConsoleOutputCP(1251);

    TD4Emulator emu;
    memset(&emu, 0, sizeof(emu));

    char filename[256];
    printf("Введите имя файла с программой: ");
    scanf_s("%255s", filename, (unsigned)_countof(filename));

    // Добавляем расширение .bin если его нет
    if (strstr(filename, ".") == NULL) {
        strcat_s(filename, sizeof(filename), ".bin");
    }

    // Загрузка программы
    if (!load_program(&emu, filename)) {
        printf("Нажмите любую клавишу для выхода...");
        _getch();
        return 1;
    }

    printf("Программа успешно загружена из файла %s\n", filename);

    int cycle = 0;
    int auto_mode = 0;
    int delay_ms = 1000;

    printf("Эмулятор процессора TD4\n");
    print_help();
    printf("\nНажмите любую клавишу для начала...");
    _getch();

    // Основной цикл эмулятора
    while (1) {
        system("cls");

        printf("Эмулятор процессора TD4 - %s\n", auto_mode ? "АВТОМАТИЧЕСКИЙ РЕЖИМ" : "РУЧНОЙ РЕЖИМ");
        printf("Файл: %s\n", filename);
        print_state(&emu, cycle);

        if (auto_mode) {
            printf("\nАвтоматический режим. Задержка: %d мс\n", delay_ms);
            printf("Нажмите 'm' для перехода в ручной режим или 'q' для выхода\n");

            execute_instruction(&emu);
            cycle++;

            Sleep(delay_ms);
        }
        else {
            printf("\nРучной режим. Нажмите Enter для выполнения команды\n");
            printf("Или выберите действие: a-авто, s-память, i-ввод, r-сброс, h-справка, q-выход\n");
            while (_kbhit()==NULL){}
        }

        // Обработка ввода
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
                printf("\nПереключение в автоматический режим\n");
                Sleep(1000);
                break;

            case 'm':
            case 'M':
                auto_mode = 0;
                printf("\nПереключение в ручной режим\n");
                Sleep(1000);
                break;

            case 's':
            case 'S':
                print_memory(&emu);
                printf("Нажмите любую клавишу для продолжения...");
                _getch();
                break;

            case 'i':
            case 'I':
            {
                printf("\nТекущее значение входного порта: %X (двоичное: ", emu.IN_line);
                char bin_cur[5];
                to_binary_str(emu.IN_line, bin_cur);
                printf("%s)\n", bin_cur);
                printf("Введите новое значение (0-15 или 4-битное двоичное, например 1011): ");

                char input[10];
                scanf_s("%9s", input, (unsigned)_countof(input));
                while (getchar() != '\n'); // Очистка буфера

                int value = -1;
                int len = (int)strlen(input);

                // Попытка интерпретировать как двоичное (ровно 4 символа, только 0/1)
                if (len == 4) {
                    bool is_binary = true;
                    for (int i = 0; i < 4; i++) {
                        if (input[i] != '0' && input[i] != '1') {
                            is_binary = false;
                            break;
                        }
                    }
                    if (is_binary) {
                        // Преобразуем двоичную строку в число
                        value = 0;
                        for (int i = 0; i < 4; i++) {
                            value = (value << 1) | (input[i] - '0');
                        }
                    }
                }

                // Если не двоичное — пробуем как десятичное
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
                    printf("Значение входного порта изменено на: %d (двоичное: %s)\n", value, bin_new);
                }
                else {
                    printf("Ошибка: введите число от 0 до 15 или 4-битную двоичную строку (например, 1011)!\n");
                }

                printf("Нажмите любую клавишу для продолжения...");
                _getch();
                break;
            }

            case 'r':
            case 'R':
                memset(&emu, 0, sizeof(emu));
                load_program(&emu, filename);
                cycle = 0;
                printf("\nПроцессор сброшен. Программа скоро будет перезагружена.\n");
                Sleep(2000);
                break;

            case 'h':
            case 'H':
                print_help();
                printf("Нажмите любую клавишу для продолжения...");
                _getch();
                break;

            case 'q':
            case 'Q':
                printf("\nВыход из эмулятора\n");
                return 0;
            }
        }
    }

    return 0;
}