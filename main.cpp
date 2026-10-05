#include <windows.h>
#include <iostream>
#include <vector>
#include "MarkerLogic.h"

int arraySize = 0;
int* targetArray = nullptr;

CRITICAL_SECTION csArray;
CRITICAL_SECTION csConsole;

HANDLE startAllEvent = NULL;

std::vector<HANDLE> cannotContinueEvents;
std::vector<HANDLE> continueEvents;
std::vector<HANDLE> terminateEvents;
std::vector<HANDLE> threadHandles;
std::vector<bool> isThreadTerminated;

DWORD WINAPI markerRoutine(LPVOID threadParameter) {
    int threadNumber = (int)(intptr_t)threadParameter;
    int threadIndex = threadNumber - 1;

    WaitForSingleObject(startAllEvent, INFINITE);

    srand(threadNumber);

    int markedElementsCount = 0;

    HANDLE responseEvents[2] = {
        terminateEvents[threadIndex],
        continueEvents[threadIndex]
    };

    while (true) {
        int randomNumber = rand();
        int targetIndex = randomNumber % arraySize;

        EnterCriticalSection(&csArray);

        if (canMarkElement(targetArray, targetIndex)) {
            Sleep(5);
            targetArray[targetIndex] = threadNumber;
            Sleep(5);
            markedElementsCount++;
            LeaveCriticalSection(&csArray);
            continue;
        }
        else {
            LeaveCriticalSection(&csArray);

            EnterCriticalSection(&csConsole);
            std::cout << "поток marker номер: " << threadNumber << "\n";
            std::cout << "кол-во помеченных элементов: " << markedElementsCount << "\n";
            std::cout << "невозможно пометить элемент с индексом: " << targetIndex << "\n\n";
            LeaveCriticalSection(&csConsole);

            SetEvent(cannotContinueEvents[threadIndex]);

            DWORD waitResult = WaitForMultipleObjects(2, responseEvents, FALSE, INFINITE);

            if (waitResult == WAIT_OBJECT_0) {
                EnterCriticalSection(&csArray);
                clearElementsMarkedByThread(targetArray, arraySize, threadNumber);
                LeaveCriticalSection(&csArray);
                return 0;
            }
            else if (waitResult == WAIT_OBJECT_0 + 1) {
                continue;
            }
        }
    }

    return 0;
}

int main() {
    SetConsoleCP(1251);
    SetConsoleOutputCP(1251);

    InitializeCriticalSection(&csArray);
    InitializeCriticalSection(&csConsole);

    std::cout << "введи размерность массива: ";
    std::cin >> arraySize;

    targetArray = new int[arraySize];
    for (int i = 0; i < arraySize; ++i) {
        targetArray[i] = 0;
    }

    int markerThreadsCount = 0;
    std::cout << "введите кол-во потоков marker: ";
    std::cin >> markerThreadsCount;

    startAllEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    cannotContinueEvents.resize(markerThreadsCount);
    continueEvents.resize(markerThreadsCount);
    terminateEvents.resize(markerThreadsCount);
    threadHandles.resize(markerThreadsCount);
    isThreadTerminated.assign(markerThreadsCount, false);

    for (int i = 0; i < markerThreadsCount; ++i) {
        cannotContinueEvents[i] = CreateEvent(NULL, FALSE, FALSE, NULL);
        continueEvents[i] = CreateEvent(NULL, FALSE, FALSE, NULL);
        terminateEvents[i] = CreateEvent(NULL, FALSE, FALSE, NULL);

        int threadNumber = i + 1;
        threadHandles[i] = CreateThread(
            NULL,
            0,
            markerRoutine,
            (LPVOID)(intptr_t)threadNumber,
            0,
            NULL
        );
    }

    SetEvent(startAllEvent);

    int activeThreadsCount = markerThreadsCount;

    while (activeThreadsCount > 0) {
        std::vector<HANDLE> activeCannotContinueEvents;
        for (int i = 0; i < markerThreadsCount; ++i) {
            if (!isThreadTerminated[i]) {
                activeCannotContinueEvents.push_back(cannotContinueEvents[i]);
            }
        }

        WaitForMultipleObjects(
            (DWORD)activeCannotContinueEvents.size(),
            activeCannotContinueEvents.data(),
            TRUE,
            INFINITE
        );

        EnterCriticalSection(&csConsole);
        std::cout << "- - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n";
        std::cout << "Текущее состояние массива:\n";
        for (int i = 0; i < arraySize; ++i) {
            std::cout << targetArray[i] << " ";
        }
        std::cout << "\n- - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n\n";
        LeaveCriticalSection(&csConsole);

        int chosenThreadNumber = 0;
        while (true) {
            std::cout << "введи порядковый номер потока marker: ";
            if (!(std::cin >> chosenThreadNumber)) {
                std::cin.clear();
                std::cin.ignore(10000, '\n');
                std::cout << "ошибка ввода\n";
                continue;
            }

            int index = chosenThreadNumber - 1;
            if (index >= 0 && index < markerThreadsCount && !isThreadTerminated[index]) {
                break;
            }

            std::cout << "некорректный номер\n";
        }

        int chosenIndex = chosenThreadNumber - 1;
        SetEvent(terminateEvents[chosenIndex]);

        WaitForSingleObject(threadHandles[chosenIndex], INFINITE);
        CloseHandle(threadHandles[chosenIndex]);
        isThreadTerminated[chosenIndex] = true;
        activeThreadsCount--;

        EnterCriticalSection(&csConsole);
        std::cout << "массив после завершения работы потока " << chosenThreadNumber << ":\n";
        for (int i = 0; i < arraySize; ++i) {
            std::cout << targetArray[i] << " ";
        }
        std::cout << "\n\n";
        LeaveCriticalSection(&csConsole);

        if (activeThreadsCount > 0) {
            for (int i = 0; i < markerThreadsCount; ++i) {
                if (!isThreadTerminated[i]) {
                    SetEvent(continueEvents[i]);
                }
            }
        }
    }

    CloseHandle(startAllEvent);
    for (int i = 0; i < markerThreadsCount; ++i) {
        CloseHandle(cannotContinueEvents[i]);
        CloseHandle(continueEvents[i]);
        CloseHandle(terminateEvents[i]);
    }

    DeleteCriticalSection(&csArray);
    DeleteCriticalSection(&csConsole);

    delete[] targetArray;

    std::cout << "потоки marker завершили работу\n";
    std::cout << "завершено\n";
    return 0;
}