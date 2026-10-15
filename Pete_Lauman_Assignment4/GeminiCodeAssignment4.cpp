#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <iomanip>

/**
 * @class SudokuSolver
 * @brief Encapsulates a 9x9 Sudoku puzzle grid, file loading, validation, and solving logic.
 */
class SudokuSolver {
private:
    std::vector<std::vector<char>> grid;
    std::string filename;

    /**
     * @brief Checks if placing a number at a specific row and column violates Sudoku rules.
     * @param row Row index (0-8)
     * @param col Column index (0-8)
     * @param num Character digit ('1'-'9') to check
     * @return true if the placement is safe, false otherwise
     */
    bool isSafe(int row, int col, char num) const {
        // Check row and column
        for (int i = 0; i < 9; ++i) {
            if (grid[row][i] == num) return false;
            if (grid[i][col] == num) return false;
        }

        // Check 3x3 subgrid
        int startRow = row - (row % 3);
        int startCol = col - (col % 3);
        for (int r = 0; r < 3; ++r) {
            for (int c = 0; c < 3; ++c) {
                if (grid[startRow + r][startCol + c] == num) return false;
            }
        }

        return true;
    }

    /**
     * @brief Recursive helper function to find and print all solutions using backtracking.
     * @param row Current row index
     * @param col Current column index
     * @param solutionCount Reference counter tracking the number of valid solutions found
     */
    void solveAllRecursive(int row, int col, int& solutionCount) {
        // Base case: Reached past the last row, meaning a complete solution is found
        if (row == 9) {
            solutionCount++;
            std::cout << "--- Solution " << solutionCount << " ---\n";
            printGrid();
            return;
        }

        // Determine next cell coordinates
        int nextRow = (col == 8) ? row + 1 : row;
        int nextCol = (col == 8) ? 0 : col + 1;

        // If the current cell is already filled, move to the next cell
        if (grid[row][col] != '_') {
            solveAllRecursive(nextRow, nextCol, solutionCount);
            return;
        }

        // Try digits '1' through '9'
        for (char digit = '1'; digit <= '9'; ++digit) {
            if (isSafe(row, col, digit)) {
                grid[row][col] = digit;            // Place digit
                solveAllRecursive(nextRow, nextCol, solutionCount); // Recurse
                grid[row][col] = '_';            // Backtrack
            }
        }
    }

public:
    /**
     * @brief Constructor initializing the SudokuSolver with a target filename.
     * @param fname The name of the puzzle input file
     */
    explicit SudokuSolver(std::string fname) : filename(std::move(fname)), grid(9, std::vector<char>(9, '_')) {}

    /**
     * @brief Loads the 9x9 Sudoku puzzle from the associated input file.
     * @return true if successfully loaded, false otherwise
     */
    bool loadPuzzle() {
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Error: Could not open file " << filename << std::endl;
            return false;
        }

        for (int r = 0; r < 9; ++r) {
            for (int c = 0; c < 9; ++c) {
                if (!(file >> grid[r][c])) {
                    std::cerr << "Error: Malformed content or insufficient data in " << filename << std::endl;
                    return false;
                }
            }
        }
        file.close();
        return true;
    }

    /**
     * @brief Prints the current 9x9 Sudoku grid to standard output in a clean format.
     */
    void printGrid() const {
        for (int r = 0; r < 9; ++r) {
            if (r > 0 && r % 3 == 0) {
                std::cout << "------+-------+------\n";
            }
            for (int c = 0; c < 9; ++c) {
                if (c > 0 && c % 3 == 0) {
                    std::cout << "| ";
                }
                std::cout << grid[r][c] << " ";
            }
            std::cout << "\n";
        }
        std::cout << "\n";
    }

    /**
     * @brief Validates the initial puzzle configuration for pre-existing rule violations.
     * @return true if the initial puzzle is valid, false if conflicts exist
     */
    bool isValidInitial() const {
        for (int r = 0; r < 9; ++r) {
            for (int c = 0; c < 9; ++c) {
                char val = grid[r][c];
                if (val != '_') {
                    if (val < '1' || val > '9') {
                        std::cerr << "Error: Invalid character '" << val << "' found at (" << r << ", " << c << ").\n";
                        return false;
                    }
                    // Temporarily remove the current cell to check against duplicates
                    SudokuSolver tempSolver = *this;
                    tempSolver.grid[r][c] = '_';
                    if (!tempSolver.isSafe(r, c, val)) {
                        return false; // Conflict found in initial setup
                    }
                }
            }
        }
        return true;
    }

    /**
     * @brief Initiates the solving process, checks initial validity, and outputs results.
     */
    void solve() {
        std::cout << "========================================\n";
        std::cout << "Puzzle File: " << filename << "\n";
        std::cout << "========================================\n";
        
        std::cout << "Original Puzzle:\n";
        printGrid();

        // Validate initial puzzle configuration before solving
        if (!isValidInitial()) {
            std::cout << "Validation Error: The initial puzzle contains conflicting numbers.\n";
            std::cout << "No solution found.\n\n";
            return;
        }

        int solutionCount = 0;
        solveAllRecursive(0, 0, solutionCount);

        if (solutionCount == 0) {
            std::cout << "No solution found.\n\n";
        } else {
            std::cout << "Total solutions found: " << solutionCount << "\n\n";
        }
    }
};

/**
 * @brief Main entry point of the program.
 * @return int Exit status code
 */
int main() {
    // Array containing the five required input files
    const std::string puzzleFiles[5] = {
        "puzzle1.txt",
        "puzzle2.txt",
        "puzzle3.txt",
        "puzzle4.txt",
        "puzzle5.txt"
    };

    // Iterate through each puzzle file, instantiate solver, and process
    for (const std::string& file : puzzleFiles) {
        SudokuSolver solver(file);
        if (solver.loadPuzzle()) {
            solver.solve();
        } else {
            std::cout << "Skipping file " << file << " due to loading errors.\n\n";
        }
    }

    return 0;
}