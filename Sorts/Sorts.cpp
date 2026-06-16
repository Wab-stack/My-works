#include <iostream>

void bubbleSort(int arr[], int n) {
    int count{};
    for (int i = 0; i < n - 1; i++) {
        bool isSwap = false;
        for (int j = 0; j < n - 1; j++) {
            if (arr[j] < arr[j + 1]) {
                std::swap(arr[j], arr[j + 1]);
                isSwap = true;
            }
            std::cout << ++count << " |";
        }
        if (!isSwap)
            break;
    }
}

void selectionSort(int arr[], int n, bool direct = true) {
    for (int i = 0; i < n - 1; i++) {
        int minIndex = i;  // Индекс минимального элемента
        for (int j = i + 1; j < n; j++) {
            if ((arr[j] < arr[minIndex] && direct) ||
                (arr[j] > arr[minIndex] && !direct)) {
                minIndex = j;
            }
        }
        std::swap(arr[i], arr[minIndex]);
    }
}

void insertionSort(int arr[], int n) {
    for (int i = 1; i < n; i++) {
        int key = arr[i];  // Текущий элемент для вставки
        int j = i - 1;

        // Сдвигаем элементы больше key вправо
        while (j >= 0 && arr[j] > key) {
            arr[j + 1] = arr[j];
            j--;
        }
        arr[j + 1] = key;  // Вставляем key в правильную позицию
    }
}

void quickSort(int mas[], int first, int last) {

    int f = first;
    int l = last;

    int mid = mas[(f + l) / 2];

    do {
        while (mas[f] < mid) f++;
        while (mas[l] > mid) l--;

        if (f <= l) {
            std::swap(mas[f], mas[l]);
            f++;
            l--;
        }

    } while (f < l);

    if (first < l) quickSort(mas, first, l);
    if (f < last) quickSort(mas, f, last);
}

void merge(int mas[], int first, int mid, int last) {
    int size = last - first + 1;
    int* temp = new int[size];

    int i = first;      // Индекс для левой половины
    int j = mid + 1;    // Индекс для правой половины
    int k = 0;          // Индекс для временного массива

    // Слияние двух отсортированных половин
    while (i <= mid && j <= last) {
        if (mas[i] <= mas[j]) {
            temp[k++] = mas[i++];
        }
        else {
            temp[k++] = mas[j++];
        }
    }

    // Копирование оставшихся элементов из левой части
    while (i <= mid) {
        temp[k++] = mas[i++];
    }

    // Копирование оставшихся элементов из правой части
    while (j <= last) {
        temp[k++] = mas[j++];
    }

    // Копирование отсортированного временного массива обратно в исходный
    for (int idx = 0; idx < size; ++idx) {
        mas[first + idx] = temp[idx];
    }

    delete[] temp;
}

// Сортировка слиянием (рекурсивная)
void mergeSortReq(int mas[], int first, int last) {
    if (first >= last) {
        return;
    }

    int mid = (first + last) / 2;

    // Рекурсивно сортируем левую и правую половины
    mergeSortReq(mas, first, mid);
    mergeSortReq(mas, mid + 1, last);

    // Сливаем две отсортированные половины
    merge(mas, first, mid, last);
}

//Итеративная сортировка слиянием
void mergeSort(int mas[], int first, int last) {
    int n = last - first + 1;

    for (int size = 1; size < n; size *= 2) {
        // Перебираем начальные позиции левых подмассивов
        for (int left = first; left <= last - size; left += 2 * size) {
            int mid = left + size - 1;
            int right = left + 2 * size - 1;
            if (right > last) right = last;

            merge(mas, left, mid, right);
        }
    }
}

int main()
{
    return 0;
}