#include <ncurses.h>
#include <stdio.h>

#define DELAY_LOWEST 20    // Наименьшая задержка
#define DELAY_HIGHEST 200  // Наибольшая задержка
#define DELAY_STEP 10      // Шаг задержки

#define ROWS 25     // Колонки поля
#define COLUMNS 80  // Ряды поля

#define STATES_DIR "./datasets/"  // Каталог с начальными состояниями

// Вывод начального меню
int start_menu(int field[ROWS][COLUMNS]);

// Создание пустого состояния
int generate_blank_state();

// Считывание состояния из потока ввода (не используется)
// void read_from_input(int field[ROWS][COLUMNS]);

// Расчёт диапазона файлов с начальным состоянием
int count_states(int range[2]);

// Считывание состояния из файла
int read_from_file(int field[ROWS][COLUMNS], int choice);

// Вывод кадра игры
void print_game(int field[ROWS][COLUMNS], int counter, int delay);

// Считывание и обработка клавиш
int handle_input(int* delay);

// Обновления состояния игры
void advance_game_state(int previous_field[ROWS][COLUMNS], int field[ROWS][COLUMNS], int* counter);

// Подсчёт соседей
int count_neighbours(const int previous_field[ROWS][COLUMNS], int y, int x);

int main() {
    int previous_field[ROWS][COLUMNS], field[ROWS][COLUMNS];
    // read_from_input(field);

    int start_menu_result = start_menu(field);
    if (start_menu_result == 1) {
        return 1;
    } else if (start_menu_result == 2) {
        printf("Blank state \"0.txt\" was successfully generated.\n");
        return 0;
    }

    // Перенаправляем стандартный поток ввода обратно из консоли для считывания клавиш
    if (freopen("/dev/tty", "r", stdin) == NULL) {
        return 1;  // Ошибка переоткрытия
    }

    // Инициализация ncurses
    initscr();

    // Инициализация цветов
    if (has_colors() == FALSE) {
        endwin();
        printf("This terminal doesn't support colors\n");
        return 1;
    }
    start_color();

    cbreak();               // Посимвольное считывание
    noecho();               // Отключение отображения введённых символов
    curs_set(0);            // Скрытие курсора
    nodelay(stdscr, TRUE);  // Неблокирующий ввод
    keypad(stdscr, TRUE);  // Обработка специальных клавиш (F1, Home, стрелки и т. д.)

    int counter = 0, delay = 50;
    print_game(field, counter, delay);

    while (1) {
        if (handle_input(&delay) == 1) break;

        advance_game_state(previous_field, field, &counter);

        print_game(field, counter, delay);

        napms(delay);
    }

    endwin();  // Завершение ncurses

    return 0;
}

int start_menu(int field[ROWS][COLUMNS]) {
    int choice, flag = 0;

    // Вычисляем диапазон файлов с начальными состояниями
    int range[2];
    flag = count_states(range);

    if (flag == 0) {
        int scanf_flag;

        // Нашли хотя бы один файл с начальным состоянием
        do {
            if (range[0] != range[1]) {
                // Нашли более 1-го файла
                printf("Choose a starting configuration (%d-%d), or 0 to generate a blank state: ", range[0],
                       range[1]);
            } else {
                // Найшли 1 файл
                printf("Choose a starting configuration (%d), or 0 to generate a blank state: ", range[0]);
            }

            scanf_flag = scanf("%d", &choice);

            if (scanf_flag != 1) {
                // Введено не число
                printf("Invalid input. Please enter a number.\n");

                int c;
                while ((c = getchar()) != '\n' && c != EOF);
            } else if (choice != 0 && (choice < range[0] || choice > range[1])) {
                // Введено число вне диапазона файлов с состояниями и не 0
                if (range[0] != range[1]) {
                    // Если более 1-го файла
                    printf("Invalid input. Please enter a number between %d and %d, or 0.\n", range[0],
                           range[1]);
                } else {
                    // Если 1 файл
                    printf("Invalid input. Please enter the number %d or 0.\n", range[0]);
                }
            }
        } while (scanf_flag != 1 || (choice != 0 && (choice < range[0] || choice > range[1])));
    } else {
        // Не нашли файлов с начальными состояниями
        printf("Initial states are not found. Generating blank state instead...\n");
        choice = 0;
    }

    // Создание файла с пустым начальным состоянием
    if (choice == 0) {
        flag = generate_blank_state();
    }
    // Загрузка данных из файла
    else if (read_from_file(field, choice) == 1) {
        printf("Error: Could not open file %d.txt\n", choice);
        flag = 1;
    }

    return flag;
}

int generate_blank_state() {
    int flag = 0;
    char filename[64];
    snprintf(filename, sizeof filename, "%s0.txt", STATES_DIR);

    FILE* file = fopen(filename, "w");
    if (!file) flag = 1;

    if (flag != 1) {
        for (int i = 0; i < ROWS; i++) {
            for (int j = 0; j < COLUMNS; j++) {
                fprintf(file, "0");  // Заполняем нулями
                if (j != COLUMNS - 1) fprintf(file, " ");
            }
            if (i != ROWS - 1) fprintf(file, "\n");
        }

        fclose(file);
        flag = 2;  // Флаг созданного пустого состояния
    }

    return flag;
}

// void read_from_input(int field[ROWS][COLUMNS]) {
//     for (int i = 0; i < ROWS; i++) {
//         for (int j = 0; j < COLUMNS; j++) {
//             if (scanf("%d", &field[i][j]) != 1) break;
//         }
//     }
// }

int count_states(int range[2]) {
    int flag = 1;
    int depth = 100;
    char filename[64];
    FILE* f;

    // Находим первое доступное состояние
    for (range[0] = 1; range[0] < depth; range[0]++) {
        snprintf(filename, sizeof filename, "%s%d.txt", STATES_DIR, range[0]);
        f = fopen(filename, "r");
        if (f) {
            fclose(f);
            flag = 0;
            break;
        }
    }

    if (flag == 0) {
        // Находим последнее доступное состояние
        for (range[1] = range[0];; range[1]++) {
            snprintf(filename, sizeof filename, "%s%d.txt", STATES_DIR, range[1] + 1);

            f = fopen(filename, "r");
            if (f) {
                fclose(f);
            } else {
                break;
            }
        }
    }

    return flag;
}

int read_from_file(int field[ROWS][COLUMNS], int choice) {
    int flag = 0;

    char filename[64];
    // Формируем имя: "1.txt", "2.txt" и т.д.
    snprintf(filename, sizeof filename, "%s%d.txt", STATES_DIR, choice);

    FILE* file = fopen(filename, "r");
    if (!file) flag = 1;

    if (flag != 1) {
        for (int i = 0; i < ROWS; i++) {
            for (int j = 0; j < COLUMNS; j++) {
                if (fscanf(file, "%d", &field[i][j]) != 1) {
                    field[i][j] = 0;  // Записываем ноль, если значение некорректное
                }
            }
        }

        fclose(file);
    }

    return flag;
}

void print_game(int field[ROWS][COLUMNS], int counter, int delay) {
    const char BORDER_HORIZONTAL = '-';
    const char BORDER_VERTICAL = '|';
    const char BORDER_CORNER = '*';
    const char ALIVE_CELL = ' ';
    const char DEAD_CELL = ' ';

    // Кол-во цветов клеток (включая мёртвые), устанавливается вручную
    const int COLORS_AMOUNT = 5;

    // Цвет мёртвой клетки
    init_pair(1, COLOR_BLACK, COLOR_BLACK);

    // Цвет живой клетки (возраст 1)
    init_pair(2, COLOR_GREEN, COLOR_GREEN);

    // Цвет живой клетки (возраст 2)
    init_pair(3, COLOR_YELLOW, COLOR_YELLOW);

    // Переопределяем пурпурный цвет в оранжевый, если можем
    if (can_change_color() == TRUE) {
        init_color(COLOR_MAGENTA, 1000, 700, 0);
    }

    // Цвет живой клетки (возраст 3)
    init_pair(4, COLOR_MAGENTA, COLOR_MAGENTA);

    // Цвет живой клетки (возраст 4)
    init_pair(5, COLOR_RED, COLOR_RED);

    // Цвет границы
    init_pair(6, COLOR_WHITE, COLOR_BLACK);

    // Цвет текста
    init_pair(7, COLOR_CYAN, COLOR_BLACK);

    clear();

    // Верхняя граница
    attron(COLOR_PAIR(6));
    printw("%c", BORDER_CORNER);

    for (int j = 0; j < COLUMNS; j++) printw("%c", BORDER_HORIZONTAL);

    printw("%c\n", BORDER_CORNER);
    attroff(COLOR_PAIR(6));

    // Вывод поля
    for (int i = 0; i < ROWS; i++) {
        attron(COLOR_PAIR(6));
        printw("%c", BORDER_VERTICAL);  // Левая граница
        attroff(COLOR_PAIR(6));

        for (int j = 0; j < COLUMNS; j++) {
            // Указываем цвет клетки в зависимости от её возраста
            attron(COLOR_PAIR((field[i][j] <= COLORS_AMOUNT - 1) ? field[i][j] + 1 : COLORS_AMOUNT));

            // Выводим символ мёртвой или живой клетки
            if (field[i][j] == 0)
                printw("%c", DEAD_CELL);
            else
                printw("%c", ALIVE_CELL);

            attroff(COLOR_PAIR((field[i][j] <= COLORS_AMOUNT - 1) ? field[i][j] + 1 : COLORS_AMOUNT));
        }

        attron(COLOR_PAIR(6));
        printw("%c\n", BORDER_VERTICAL);  // Правая граница
        attroff(COLOR_PAIR(6));
    }

    // Нижняя граница
    attron(COLOR_PAIR(6));
    printw("%c", BORDER_CORNER);

    for (int j = 0; j < COLUMNS; j++) printw("%c", BORDER_HORIZONTAL);

    printw("%c\n", BORDER_CORNER);
    attroff(COLOR_PAIR(6));

    // Вывод текста интерфейса
    attron(COLOR_PAIR(7));
    printw("The loop is running. Press A/Z to increase/decrease speed. Press Q or Space to quit.\n\n");

    printw("Generation # %d\n", counter);

    printw("Delay: %d ms", delay);
    attroff(COLOR_PAIR(7));

    refresh();
}

int handle_input(int* delay) {
    int ch = ERR;

    ch = getch();  // Пробуем считать нажатую клавишу

    int quit_flag = 0;
    switch (ch) {
        // Увеличение скорости (уменьшение задержки)
        case 'a':
        case 'A':
            if (*delay - DELAY_STEP >= DELAY_LOWEST) *delay -= DELAY_STEP;
            break;

        // Уменьшение скорости (увеличение задержки)
        case 'z':
        case 'Z':
            if (*delay + DELAY_STEP <= DELAY_HIGHEST) *delay += DELAY_STEP;
            break;

        // Выход из цикла
        case ' ':
        case 'q':
        case 'Q':
            quit_flag = 1;
            break;
    }

    return quit_flag;
}

void advance_game_state(int previous_field[ROWS][COLUMNS], int field[ROWS][COLUMNS], int* counter) {
    (*counter)++;

    // Сохранение предыдущего состояния
    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLUMNS; j++) {
            previous_field[i][j] = field[i][j];
        }
    }

    // Вычисление текущего состояния
    int neighboors;

    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLUMNS; j++) {
            // Считаем соседей
            neighboors = count_neighbours(previous_field, i, j);

            if (previous_field[i][j] == 0) {  // Клетка мертва
                field[i][j] = (neighboors == 3);
            } else {  // Клетка жива
                // Клетка "стареет", если выживает
                int age = (previous_field[i][j] + 1) * (neighboors == 2 || neighboors == 3);
                // Максимальный отслеживаемый возраст клетки
                field[i][j] = (age <= 9) ? age : 9;
            }
        }
    }
}

int count_neighbours(const int previous_field[ROWS][COLUMNS], int y, int x) {
    int result = 0, y_coord, x_coord;

    for (int i = -1; i < 2; i++) {
        for (int j = -1; j < 2; j++) {
            if (i != 0 ||
                j != 0) {  // Клетка не считает сама себя (де Моргана от условия !(i == 0 && j == 0))
                y_coord = (y + i + ROWS) % ROWS;
                x_coord = (x + j + COLUMNS) % COLUMNS;
                if (previous_field[y_coord][x_coord] != 0) result++;
            }
        }
    }

    return result;
}