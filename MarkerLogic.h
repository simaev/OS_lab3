#pragma once

inline bool canMarkElement(const int* targetArray, int targetIndex) {
    return targetArray[targetIndex] == 0;
}

inline void clearElementsMarkedByThread(int* targetArray, int arraySize, int threadNumber) {
    for (int i = 0; i < arraySize; ++i) {
        if (targetArray[i] == threadNumber) {
            targetArray[i] = 0;
        }
    }
}