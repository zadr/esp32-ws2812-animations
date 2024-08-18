// Fibonacci.hpp

#ifndef FIBONACCI_HPP
#define FIBONACCI_HPP

#include <array>

// Function to multiply two 2x2 matrices
void multiplyMatrix(std::array<std::array<long long, 2>, 2>& a, const std::array<std::array<long long, 2>, 2>& b) {
    std::array<std::array<long long, 2>, 2> result = {0};

    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 2; j++) {
            result[i][j] = a[i][0] * b[0][j] + a[i][1] * b[1][j];
        }
    }

    // Copy the result back to matrix a
    a = result;
}

// Function to compute matrix exponentiation
void powerMatrix(std::array<std::array<long long, 2>, 2>& matrix, int n) {
    if (n == 0 || n == 1) {
        return;
    }

    std::array<std::array<long long, 2>, 2> baseMatrix = {{{1, 1}, {1, 0}}};

    powerMatrix(matrix, n / 2);
    multiplyMatrix(matrix, matrix);

    if (n % 2 != 0) {
        multiplyMatrix(matrix, baseMatrix);
    }
}

long long fibonacci(int n) {
    if (n == 0) {
        return 0;
    }
    if (n == 1) {
        return 1;
    }

    std::array<std::array<long long, 2>, 2> matrix = {{{1, 1}, {1, 0}}};

    powerMatrix(matrix, n - 1);

    return matrix[0][0];
}

#endif
