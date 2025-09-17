#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <locale.h>
#include <windows.h>
#include <conio.h>

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
        printf("Îøèáêà îòêðûòèÿ ôàéëà: %s\n", filename);
        return 0;
    }

    // Читаем программу в память
    fread(emu->memory, 1, MEMORY_SIZE, file);

    if (ferror(file)) {
        printf("Îøèáêà ÷òåíèÿ ôàéëà\n");
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
                sprintf_s(buffer, 50, "%s %d", commands[i].name, im);
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

// Функция для вывода состояния процессора
void print_state(TD4Emulator* emu, int cycle) {
    uint8_t instruction = emu->memory[emu->PC];
    char mnemonic[50];
    get_mnemonic(instruction, mnemonic);

    printf("Такт: %2d | PC: %X | Команда: %02X (%s) | A: %X | B: %X | OUT: %X | IN: %X | C: %d\n",
        cycle, emu->PC, instruction, mnemonic, emu->A, emu->B, emu->OUT_line, emu->IN_line, emu->C);
}

// Функция для отображения содержимого памяти
void print_memory(TD4Emulator* emu) {
    printf("\Содержимое памяти:\n");
    for (int i = 0; i < MEMORY_SIZE; i++) {
        if (i % 8 == 0) printf("\n%02X: ", i);
        printf("%02X ", emu->memory[i]);
    }
    printf("\n");
}

// Функция для отображения справки
void print_help() {
    printf("\Управление эмулятором:\n");
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
    printf("\Нажмите любую клавишу для начала...");
    _getch();

    // Основной цикл эмулятора
    while (1) {
        system("cls");

        printf("Эмулятор процессора TD4 - %s\n", auto_mode ? "АВТОМАТИЧЕСКИЙ РЕЖИМ" : "РУЧНОЙ РЕЖИМ");
        printf("Файл: %s\n", filename);
        print_state(&emu, cycle);

        if (auto_mode) {
            printf("\Автоматический режим. Задержка: %d мс\n", delay_ms);
            printf("Нажмите 'm' для перехода в ручной режим или 'q' для выхода\n");

            execute_instruction(&emu);
            cycle++;

            Sleep(delay_ms);
        }
        else {
            printf("\Ручной режим. Нажмите Enter для выполнения команды\n");
            printf("Или выберите действие: a-авто, s-память, i-ввод, r-сброс, h-справка, q-выход\n");
            while (_kbhit()==NULL){}
        }

        // Обработка ввода
        if (_kbhit()) {
            int ch = _getch();

            switch (ch) {
            case '\r': // Enter
                if (!auto_mode) {
                    execute_instruction(&emu);
                    cycle++;
                }
                break;

            case 'a':
            case 'A':
                auto_mode = 1;
                printf("\Переключение в автоматический режим\n");
                Sleep(1000);
                break;

            case 'm':
            case 'M':
                auto_mode = 0;
                printf("\Переключение в ручной режим\n");
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
                printf("\Текущее значение входного порта: %X\n", emu.IN_line);
                printf("Введите новое значение (0-15): ");
                {
                    int value;
                    scanf_s("%d", &value);
                    if (value >= 0 && value <= 15) {
                        emu.IN_line = value;
                        printf("Значение входного порта изменено на: %X\n", emu.IN_line);
                    }
                    else {
                        printf("Неверное значение!\n");
                    }
                    while (getchar() != '\n'); // Очистка буфера ввода
                }
                printf("Нажмите любую клавишу для продолжения...");
                _getch();
                break;

            case 'r':
            case 'R':
                memset(&emu, 0, sizeof(emu));
                load_program(&emu, filename);
                cycle = 0;
                printf("\Процессор сброшен. Программа скоро будет перезагружена.\n");
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
                printf("\Выход из эмулятора\n");
                return 0;
            }
        }
    }

    return 0;

}
