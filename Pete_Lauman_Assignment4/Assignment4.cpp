/*
Program: EECS 348 Assignment 4 - Recursive Sudoku Solver
Description: Reads five Sudoku puzzles and prints every valid solution using recursive backtracking.
Inputs: puzzle1.txt, puzzle2.txt, puzzle3.txt, puzzle4.txt, puzzle5.txt
Outputs: Original puzzle and every solution, or "No solution found".
Author: Peter Lauman
Created: October 15, 2026
Collaborators / sources: OpenAI ChatGPT
*/
#include <array>      // Fixed-size containers hold the nine-by-nine grid.
#include <fstream>    // Input streams read each puzzle file.
#include <iostream>   // Standard output displays puzzles and solutions.
#include <string>     // File names and error messages are strings.
#include <vector>     // Candidate lists store valid numbers for a cell.

class SudokuSolver {
private:
    std::array<std::array<int, 9>, 9> grid{}; // Zero represents an empty square.
    unsigned long long solutions = 0;        // Track how many completed grids are found.

    // Check whether a number violates any Sudoku constraint at a given square.
    bool allowed(int row, int col, int value) const {
        for (int i = 0; i < 9; ++i) { // Scan the corresponding row and column.
            if (grid[row][i] == value || grid[i][col] == value) return false;
        }
        int firstRow = (row / 3) * 3; // Find the top row of the containing block.
        int firstCol = (col / 3) * 3; // Find the left column of the containing block.
        for (int r = firstRow; r < firstRow + 3; ++r) { // Visit each block row.
            for (int c = firstCol; c < firstCol + 3; ++c) { // Visit each block column.
                if (grid[r][c] == value) return false; // Reject repeated digits.
            }
        }
        return true; // No conflict was found.
    }

    // Use the empty square with the fewest legal candidates to reduce branching.
    bool chooseCell(int& bestRow, int& bestCol, std::vector<int>& bestValues) const {
        bestRow = -1; // Negative row means the board has no empty cells.
        bestCol = -1; // Negative column also indicates no choice yet.
        bestValues.clear(); // Remove candidates from the previous recursion level.
        for (int r = 0; r < 9; ++r) { // Inspect all rows.
            for (int c = 0; c < 9; ++c) { // Inspect all columns.
                if (grid[r][c] != 0) continue; // Ignore already filled cells.
                std::vector<int> candidates; // Store legal values for this square.
                for (int v = 1; v <= 9; ++v) { // Try digits one through nine.
                    if (allowed(r, c, v)) candidates.push_back(v); // Keep legal digits.
                }
                if (candidates.empty()) { // An empty square has no valid digit.
                    bestRow = r; // Identify the contradictory square.
                    bestCol = c; // Identify its column.
                    bestValues.clear(); // Signal that this branch cannot work.
                    return true; // A blank cell exists, but this branch is impossible.
                }
                if (bestRow == -1 || candidates.size() < bestValues.size()) { // Prefer fewer options.
                    bestRow = r; // Save the strongest next square.
                    bestCol = c; // Save its column.
                    bestValues = candidates; // Save its available digits.
                }
            }
        }
        return bestRow != -1; // False means the entire board is filled.
    }

    // Recursively explore every valid completion of the current grid.
    void backtrack() {
        int row, col; // Coordinates of the next empty square.
        std::vector<int> values; // Valid candidates for that square.
        if (!chooseCell(row, col, values)) { // No empty squares remain.
            ++solutions; // Count the completed Sudoku board.
            std::cout << "Solution " << solutions << ":\n"; // Label each solution.
            print(); // Print the completed board.
            return; // Continue searching after returning to the caller.
        }
        for (int value : values) { // Explore each candidate in order.
            grid[row][col] = value; // Tentatively fill the chosen square.
            backtrack(); // Recursively solve the smaller puzzle.
            grid[row][col] = 0; // Undo the guess so other possibilities can be tried.
        }
    }

public:
    // Read exactly 81 tokens, allowing underscores for blank squares.
    bool load(const std::string& filename) {
        std::ifstream input(filename); // Open the named puzzle.
        if (!input) return false; // Report missing or unreadable files.
        grid = {}; // Reset the grid before reading another puzzle.
        solutions = 0; // Reset the solution counter.
        std::string token; // Hold each number or underscore.
        for (int r = 0; r < 9; ++r) { // Read nine rows.
            for (int c = 0; c < 9; ++c) { // Read nine cells per row.
                if (!(input >> token)) return false; // Reject short or malformed files.
                if (token == "_") grid[r][c] = 0; // Convert a blank to zero.
                else if (token.size() == 1 && token[0] >= '1' && token[0] <= '9')
                    grid[r][c] = token[0] - '0'; // Convert a digit to an integer.
                else return false; // Reject unexpected input.
            }
        }
        if (input >> token) return false; // Reject files containing extra cells.
        for (int r = 0; r < 9; ++r) { // Check all given values.
            for (int c = 0; c < 9; ++c) { // Check every square in each row.
                if (grid[r][c] == 0) continue; // Blank squares cannot conflict.
                int value = grid[r][c]; // Temporarily remove a given digit.
                grid[r][c] = 0; // Avoid comparing the given with itself.
                bool valid = allowed(r, c, value); // Test row, column, and block.
                grid[r][c] = value; // Restore the original given digit.
                if (!valid) return false; // Reject conflicting clues.
            }
        }
        return true; // The puzzle was read and validated.
    }

    // Print the grid in the same simple format as the input files.
    void print() const {
        for (const auto& row : grid) { // Iterate through the nine rows.
            for (int c = 0; c < 9; ++c) { // Print all nine columns.
                if (c) std::cout << ' '; // Separate adjacent cells with spaces.
                if (row[c] == 0) std::cout << '_'; // Show blanks as underscores.
                else std::cout << row[c]; // Show filled squares as digits.
            }
            std::cout << '\n'; // End the current row.
        }
        std::cout << '\n'; // Separate grids visually.
    }

    // Print all solutions, including an explicit message for unsolvable boards.
    void solve() {
        backtrack(); // Begin recursive depth-first search.
        if (solutions == 0) std::cout << "No solution found\n"; // Handle failure.
        else std::cout << "Total solutions: " << solutions << "\n"; // Report count.
    }
};

// Run the same object-oriented solver on all five assigned puzzle files.
int main() {
    for (int number = 1; number <= 5; ++number) { // Process every supplied puzzle.
        std::string filename = "puzzle" + std::to_string(number) + ".txt"; // Build file name.
        SudokuSolver sudoku; // Create a fresh object for this puzzle.
        std::cout << "=== " << filename << " ===\n"; // Identify the input file.
        if (!sudoku.load(filename)) { // Read and validate the puzzle.
            std::cout << "Unable to read puzzle or invalid starting grid.\n\n"; // Explain failure.
            continue; // Move on to the next puzzle file.
        }
        std::cout << "Original puzzle:\n"; // Label the given grid.
        sudoku.print(); // Show the initial board.
        sudoku.solve(); // Find and print every solution.
        std::cout << '\n'; // Separate results from different puzzles.
    }
    return 0; // Finish successfully.
}
