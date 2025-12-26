#include <ncurses.h>
#include <stdio.h>

#define DELAY_LOWEST 20    // Наименьшая задержка
#define DELAY_HIGHEST 200  // Наибольшая задержка
#define DELAY_STEP 10      // Шаг задержки

#define ROWS 25     // Колонки поля
#define COLUMNS 80  // Ряды поля

#define STATES_AMOUNT 3  // Кол-во начальных состояний

// Вывод начального меню
int start_menu(int field[ROWS][COLUMNS]);

// Создание пустого состояния
int generate_blank_state();

// Считывание состояния из потока ввода
void read_from_input(int field[ROWS][COLUMNS]);

// Считывание состояния из файла
int read_from_file(int field[ROWS][COLUMNS], int choice);

// Вывод кадра игры
void print_game(int field[ROWS][COLUMNS], int counter, int delay);

// Считывание и обработка клавиш
int handle_input(int* delay);

// Обновления состояния игры
void advance_game_state(int previous_field[ROWS][COLUMNS], int field[ROWS][COLUMNS], int* counter);

// Подсчёт соседей
int count_neighbours(int previous_field[ROWS][COLUMNS], int y, int x);

int main() {
    int previous_field[ROWS][COLUMNS], field[ROWS][COLUMNS];
    // read_from_input(field);

    int start_menu_result = start_menu(field);
    if (start_menu_result == 1) {
        return 1;
    } else if (start_menu_result == 2) {
        printf("Blank state was successfully generated.\n");
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
        printf("This terminal doesn't support color'\n");
        return 1;
    }
    start_color();

    cbreak();     // Посимвольное считывание
    noecho();     // Отключение отображения введённых символов
    curs_set(0);  // Скрытие курсора
    keypad(stdscr, TRUE);  // Обработка специальных клавиш (F1, Home, стрелки и т. д.)
    nodelay(stdscr, TRUE);  // Неблокирующий ввод

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
    int choice, scanf_flag;
    do {
        printf("Choose a starting configuration (1-%d), or 0 to generate a blank state: ", STATES_AMOUNT);

        scanf_flag = scanf("%d", &choice);

        if (scanf_flag != 1) {
            printf("Invalid input. Please enter a number.\n");

            int c;
            while ((c = getchar()) != '\n' && c != EOF);
        } else if (choice < 0 || choice > STATES_AMOUNT) {
            printf("Invalid input. Please enter a number between 0 and %d.\n", STATES_AMOUNT);
        }
    } while (scanf_flag != 1 || choice < 0 || choice > STATES_AMOUNT);

    // Загрузка данных из файла
    int flag = 0;
    if (choice == 0) {
        flag = generate_blank_state();
    } else if (read_from_file(field, choice) == 1) {
        printf("Error: Could not open file %d.txt\n", choice);
        flag = 1;
    }

    return flag;
}

int generate_blank_state() {
    int flag = 0;
    char filename[20] = "./datasets/0.txt";

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
    }

    if (flag != 1) {
        fclose(file);
        flag = 2;  // Флаг созданного пустого состояния
    }

    return flag;
}

void read_from_input(int field[ROWS][COLUMNS]) {
    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLUMNS; j++) {
            if (scanf("%d", &field[i][j]) != 1) break;
        }
    }
}

int read_from_file(int field[ROWS][COLUMNS], int choice) {
    int flag = 0;

    char filename[20];
    sprintf(filename, "./datasets/%d.txt", choice);  // Формируем имя: "1.txt", "2.txt" и т.д.

    FILE* file = fopen(filename, "r");
    if (!file) flag = 1;

    if (flag != 1) {
        for (int i = 0; i < ROWS; i++) {
            for (int j = 0; j < COLUMNS; j++) {
                if (fscanf(file, "%d", &field[i][j]) != 1) {
                    field[i][j] = 0;  // Заполняем нулями, если файл битый
                }
            }
        }
    }

    if (flag != 1) fclose(file);
    return flag;
}

void print_game(int field[ROWS][COLUMNS], int counter, int delay) {
    const char BORDER_HORIZONTAL = '-';
    const char BORDER_VERTICAL = '|';
    const char BORDER_CORNER = '*';
    const char ALIVE_CELL = '#';
    const char DEAD_CELL = '.';

    // Пара №1: Красный текст на черном фоне
    init_pair(1, COLOR_RED, COLOR_BLACK);

    // Пара №2: Зеленый текст на черном фоне
    init_pair(2, COLOR_GREEN, COLOR_BLACK);

    clear();

    // Верхняя граница
    attron(COLOR_PAIR(1));
    printw("%c", BORDER_CORNER);

    for (int j = 0; j < COLUMNS; j++) printw("%c", BORDER_HORIZONTAL);

    printw("%c\n", BORDER_CORNER);
    attroff(COLOR_PAIR(1));

    // Вывод поля
    for (int i = 0; i < ROWS; i++) {
        attron(COLOR_PAIR(1));
        printw("%c", BORDER_VERTICAL);  // Левая граница
        attroff(COLOR_PAIR(1));

        for (int j = 0; j < COLUMNS; j++) {
            if (field[i][j] == 0)
                printw("%c", DEAD_CELL);
            else
                printw("%c", ALIVE_CELL);
        }

        attron(COLOR_PAIR(1));
        printw("%c\n", BORDER_VERTICAL);  // Правая граница
        attroff(COLOR_PAIR(1));
    }

    // Нижняя граница
    attron(COLOR_PAIR(1));
    printw("%c", BORDER_CORNER);

    for (int j = 0; j < COLUMNS; j++) printw("%c", BORDER_HORIZONTAL);

    printw("%c\n", BORDER_CORNER);
    attroff(COLOR_PAIR(1));

    // Вывод текста интерфейса
    attron(COLOR_PAIR(2));
    printw("The loop is running. Press A/Z to increase/decrease speed. Press Q or Space to quit.\n\n");

    printw("Generation # %d\n", counter);

    printw("Delay: %d ms", delay);
    attroff(COLOR_PAIR(2));

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
                field[i][j] = (neighboors == 2 || neighboors == 3);
            }
        }
    }
}

int count_neighbours(int previous_field[ROWS][COLUMNS], int y, int x) {
    int result = 0, y_coord, x_coord;

    for (int i = -1; i < 2; i++) {
        for (int j = -1; j < 2; j++) {
            if (i != 0 ||
                j != 0) {  // Клетка не считает сама себя (де Моргана от условия !(i == 0 && j == 0))
                y_coord = (y + i + ROWS) % ROWS;
                x_coord = (x + j + COLUMNS) % COLUMNS;
                if (previous_field[y_coord][x_coord] == 1) result++;
            }
        }
    }

    return result;
}