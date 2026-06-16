#include <iostream>

// Глобальные константы и массивы рисунков
const int SIZE = 5;          // размер рисунка фигуры
const int ROWS = 17, COLS = 17;
const int MAX_HISTORY = 1000;

char cross[5][5] = {
    {'*', ' ', ' ', ' ', '*'},
    {' ', '*', ' ', '*', ' '},
    {' ', ' ', '*', ' ', ' '},
    {' ', '*', ' ', '*', ' '},
    {'*', ' ', ' ', ' ', '*'}
};
char cross_del[5][5] = {
    {'*', ' ', ' ', ' ', '*'},
    {' ', '*', ' ', '*', ' '},
    {' ', ' ', '0', ' ', ' '},
    {' ', '*', ' ', '*', ' '},
    {'*', ' ', ' ', ' ', '*'}
};
char null[5][5] = {
    {' ', '*', '*', '*', ' '},
    {'*', ' ', ' ', ' ', '*'},
    {'*', ' ', ' ', ' ', '*'},
    {'*', ' ', ' ', ' ', '*'},
    {' ', '*', '*', '*', ' '}
};
char null_del[5][5] = {
    {' ', '*', '*', '*', ' '},
    {'*', ' ', ' ', ' ', '*'},
    {'*', ' ', '0', ' ', '*'},
    {'*', ' ', ' ', ' ', '*'},
    {' ', '*', '*', '*', ' '}
};
int square_Starts[3] = { 0, 6, 12 };

// Функции работы с массивом
char** create_array(int rows, int cols) {
    char** arr = new char* [rows];
    for (int i = 0; i < rows; ++i)
        arr[i] = new char[cols] {};
    return arr;
}
void delete_array(char** arr, int rows) {
    for (int i = 0; i < rows; ++i) delete[] arr[i];
    delete[] arr;
}
void gen_box(char** arr, int rows, int cols) {
    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            if (j == 5 || j == 11) arr[i][j] = '|';
            else if (i == 5 || i == 11) arr[i][j] = '-';
            else arr[i][j] = ' ';
        }
    }
}
void print_box(char** arr, int rows, int cols) {
    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j)
            std::cout << arr[i][j] << ' ';
        std::cout << '\n';
    }
}

// Универсальная функция нанесения рисунка или очистки
char** apply_pattern(char** arr, int rows, int cols, int square, char(&box)[3][3],
    char pattern[5][5], char boxSymbol) {
    char** temp = new char* [rows];
    for (int i = 0; i < rows; ++i) {
        temp[i] = new char[cols] {};
        for (int j = 0; j < cols; ++j)
            temp[i][j] = arr[i][j];
    }

    int row = (square - 1) / 3;
    int col = (square - 1) % 3;
    int start_Row = square_Starts[row];
    int start_Col = square_Starts[col];

    if (pattern) {
        for (int i = 0; i < SIZE; ++i)
            for (int j = 0; j < SIZE; ++j)
                temp[start_Row + i][start_Col + j] = pattern[i][j];
    }
    else {
        // очистка – заливаем пробелами
        for (int i = 0; i < SIZE; ++i)
            for (int j = 0; j < SIZE; ++j)
                temp[start_Row + i][start_Col + j] = ' ';
    }
    box[row][col] = boxSymbol;   // 'X', 'O', или ' ' для очистки

    delete_array(arr, rows);
    return temp;
}

// Валидация хода
bool valid_turn(int turn, char box[3][3]) {
    if (std::cin.fail()) {
        std::cout << "Ошибка! Введите число.\n";
        std::cin.clear();
        std::cin.ignore();
        return false;
    }
    if (std::cin.get() != '\n') {
        std::cout << "Ошибка! Введите только одну цифру.\n";
        std::cin.clear();
        return false;
    }
    if (turn < 1 || turn > 9) {
        std::cout << "Некорректный номер квадрата\n";
        return false;
    }
    int row = (turn - 1) / 3;
    int col = (turn - 1) % 3;
    if (box[row][col] != ' ') {
        std::cout << "Квадрат уже занят!\n";
        return false;
    }
    return true;
}
bool valid_choice(int choice) {
    if (std::cin.fail()) {
        std::cout << "Ошибка! Введите число.\n";
        std::cin.clear();
        std::cin.ignore();
        return false;
    }
    if (std::cin.get() != '\n') {
        std::cout << "Ошибка! Введите только одну цифру.\n";
        std::cin.clear();
        return false;
    }
    if (choice < 0 || choice > 3) {
        std::cout << "Такого пункта меню нет\n";
        return false;
    }
    return true;
}
bool checkWin(char box[3][3], char symb) {
    for (int i = 0; i < 3; ++i)
        if (box[i][0] == symb && box[i][1] == symb && box[i][2] == symb) return true;
    for (int j = 0; j < 3; ++j)
        if (box[0][j] == symb && box[1][j] == symb && box[2][j] == symb) return true;
    if (box[0][0] == symb && box[1][1] == symb && box[2][2] == symb) return true;
    if (box[0][2] == symb && box[1][1] == symb && box[2][0] == symb) return true;
    return false;
}

// Обработка одного хода (для игрока)
void process_turn(char**& arr, char(&box)[3][3], char player,
    int history[], int& history_size, int& move_count,
    bool& win) {
    int turn;
    bool valid = false;
    while (!valid) {
        std::cout << "Ход " << (player == 'X' ? "крестиков" : "ноликов")
            << " (введите номер квадрата от 1 до 9): ";
        std::cin >> turn;
        if (!valid_turn(turn, box)) continue;
        valid = true;
    }

    // Выбор рисунка в зависимости от игрока и ситуации (обычный или с пометкой на удаление)
    // Для обычного хода используем cross/null
    char (*pattern)[SIZE] = (player == 'X') ? cross : null;
    arr = apply_pattern(arr, ROWS, COLS, turn, box, pattern, player);

    // Сохраняем ход в историю
    if (history_size < MAX_HISTORY) {
        history[history_size++] = turn;
    }
    move_count++;

    // Если сделано 3 хода – помечаем третий (по счёту, т.е. первый из трёх) как удаляемый
    if (move_count >= 3) {
        int idx = move_count - 3;   // индекс первого из трёх последних ходов
        if (idx < history_size) {
            int to_remove = history[idx];
            char (*delPattern)[SIZE] = (player == 'X') ? cross_del : null_del;
            arr = apply_pattern(arr, ROWS, COLS, to_remove, box, delPattern, player);
            print_box(arr, ROWS, COLS);
        }
    }
    // Если сделано 4 хода – удаляем самый старый (четвёртый по счёту, т.е. первый)
    if (move_count >= 4) {
        int idx = move_count - 4;
        if (idx < history_size) {
            int to_remove = history[idx];
            arr = apply_pattern(arr, ROWS, COLS, to_remove, box, nullptr, ' ');
            print_box(arr, ROWS, COLS);
        }
    }

    print_box(arr, ROWS, COLS);
    if (checkWin(box, player)) {
        std::cout << (player == 'X' ? "Крестики" : "Нолики") << " выиграли!\n";
        win = true;
    }
}

// Функция для отображения справки
void showHelp() {
    std::cout << "В данной версии игры Крестики-Нолики на поле могут оставаться всего лишь 3 крестика и 3 нолика.\n"
        << "Предыдущие символы будут удалены, однако перед удалением будут помечены 0 в центре.\n"
        << "Для хода необходимо ввести номер квадрата (1-9), нумерация начинается с левого верхнего квадрата.\n";
    std::cout << "Так выглядит обычный крестик: \n";
    for (int i = 0; i < 5; ++i) {
        for (int j = 0; j < 5; ++j) std::cout << cross[i][j];
        std::cout << '\n';
    }
    std::cout << "А так крестик, который будет удалён: \n";
    for (int i = 0; i < 5; ++i) {
        for (int j = 0; j < 5; ++j) std::cout << cross_del[i][j];
        std::cout << '\n';
    }
}

int main() {
    setlocale(LC_ALL, "Ru");
    char box[3][3] = { {' ',' ',' '},{' ',' ',' '},{' ',' ',' '} };
    char** arr = create_array(ROWS, COLS);
    gen_box(arr, ROWS, COLS);

    int historyX[MAX_HISTORY], historyO[MAX_HISTORY];
    int sizeX = 0, sizeO = 0, moveX = 0, moveO = 0;
    bool win = false;

    int user_choice = -1;
    enum Menu { TwoPlayers = 1, Computer, Help };

    do {
        std::cout << "Добро пожаловать в игру Крестики-Нолики!\n"
            << "1 - 2 игрока\n2 - Против компьютера\n3 - Инструкция\n0 - Выход\n";
        bool valid = false;
        while (!valid) {
            std::cout << "Выберите пункт меню: ";
            std::cin >> user_choice;
            if (!valid_choice(user_choice)) continue;
            valid = true;
        }

        switch (user_choice) {
        case TwoPlayers: {
            // Сброс состояния для новой игры
            // Для удобства просто обнулим историю и счётчики
            sizeX = sizeO = moveX = moveO = 0;
            win = false;
            // очищаем поле и box
            delete_array(arr, ROWS);
            arr = create_array(ROWS, COLS);
            gen_box(arr, ROWS, COLS);
            for (int i = 0; i < 3; ++i)
                for (int j = 0; j < 3; ++j)
                    box[i][j] = ' ';
            print_box(arr, ROWS, COLS);

            while (!win) {
                process_turn(arr, box, 'X', historyX, sizeX, moveX, win);
                if (win) break;
                process_turn(arr, box, 'O', historyO, sizeO, moveO, win);
            }
            break;
        }
        case Computer:
            std::cout << "Находится в стадии разработки...\n";
            break;
        case Help:
            showHelp();
            break;
        default:
            break;
        }
    } while (user_choice != 0);

    delete_array(arr, ROWS);
    return 0;
}