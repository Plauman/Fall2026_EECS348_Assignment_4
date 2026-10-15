#include <array>
#include <bitset>
#include <cstddef>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

/*
 * SudokuSolver
 * ------------
 * Holds one 9x9 puzzle and everything needed to validate and solve it.
 * Row, column, and box "used digit" bitmasks make legality checks O(1).
 * Bit d (1..9) of a mask is set when digit d is already used in that unit.
 */
class SudokuSolver {
public:
    // ---- Board constants -------------------------------------------------
    static constexpr int kSize = 9;       // Grid is kSize x kSize
    static constexpr int kBoxSize = 3;    // Each subgrid is kBoxSize x kBoxSize
    static constexpr int kEmpty = 0;      // Internal value for an empty cell

    using Grid = std::array<std::array<int, kSize>, kSize>;

    // Creates a solver for the puzzle stored in the given file.
    explicit SudokuSolver(std::string filename)
        : filename_(std::move(filename)), grid_{}, originalGrid_{},
          rowMask_{}, colMask_{}, boxMask_{} {}

    // ---- Public workflow methods ----------------------------------------

    /*
     * Reads the puzzle from the file. Returns true on success. On failure
     * returns false and stores a description in errorMessage_.
     */
    bool loadPuzzle() {
        std::ifstream input(filename_);
        if (!input) {
            errorMessage_ = "Could not open file '" + filename_ + "'.";
            return false;
        }

        // Read exactly 81 whitespace-separated tokens, row by row.
        for (int row = 0; row < kSize; ++row) {
            for (int col = 0; col < kSize; ++col) {
                std::string token;
                if (!(input >> token)) {
                    errorMessage_ = "File ended early: expected 81 cells but "
                                    "found " + std::to_string(row * kSize + col) + ".";
                    return false;
                }
                if (token == "_") {
                    grid_[row][col] = kEmpty;
                } else if (token.size() == 1 && token[0] >= '1' && token[0] <= '9') {
                    grid_[row][col] = token[0] - '0';
                } else {
                    errorMessage_ = "Invalid token '" + token + "' at row " +
                                    std::to_string(row + 1) + ", column " +
                                    std::to_string(col + 1) +
                                    " (expected 1-9 or _).";
                    return false;
                }
            }
        }

        // Any extra token means the file is not a clean 9x9 grid.
        std::string extra;
        if (input >> extra) {
            errorMessage_ = "File contains more than 81 cells.";
            return false;
        }

        originalGrid_ = grid_;  // Keep an untouched copy for display.
        return true;
    }

    /*
     * Checks the givens for conflicts (a digit repeated in a row, column, or
     * 3x3 box). Fills the bitmasks used by the solver. Returns true if the
     * puzzle is conflict-free; otherwise records each conflict and returns false.
     */
    bool validateInitialPuzzle() {
        conflicts_.clear();
        rowMask_.fill(0);
        colMask_.fill(0);
        boxMask_.fill(0);

        for (int row = 0; row < kSize; ++row) {
            for (int col = 0; col < kSize; ++col) {
                const int digit = grid_[row][col];
                if (digit == kEmpty) {
                    continue;  // Empty cells cannot conflict.
                }
                const int bit = 1 << digit;
                const int box = boxIndex(row, col);

                // Report which units already contain this digit.
                if (rowMask_[row] & bit) {
                    conflicts_.push_back("Digit " + std::to_string(digit) +
                        " repeats in row " + std::to_string(row + 1) +
                        " (cell row " + std::to_string(row + 1) + ", column " +
                        std::to_string(col + 1) + ").");
                }
                if (colMask_[col] & bit) {
                    conflicts_.push_back("Digit " + std::to_string(digit) +
                        " repeats in column " + std::to_string(col + 1) +
                        " (cell row " + std::to_string(row + 1) + ", column " +
                        std::to_string(col + 1) + ").");
                }
                if (boxMask_[box] & bit) {
                    conflicts_.push_back("Digit " + std::to_string(digit) +
                        " repeats in 3x3 box " + std::to_string(box + 1) +
                        " (cell row " + std::to_string(row + 1) + ", column " +
                        std::to_string(col + 1) + ").");
                }

                // Mark the digit as used in its row, column, and box.
                rowMask_[row] |= bit;
                colMask_[col] |= bit;
                boxMask_[box] |= bit;
            }
        }
        return conflicts_.empty();
    }

    /*
     * Finds all solutions by recursive backtracking. Call only after a
     * successful validateInitialPuzzle() so the bitmasks are current.
     */
    void solve() {
        solutions_.clear();
        backtrack();
    }

    // ---- Output methods --------------------------------------------------

    // Prints the puzzle exactly as it was read (underscores for blanks).
    void printOriginal() const {
        printGrid(originalGrid_, true);
    }

    // Prints every solution found, or "No solution found" if there are none.
    void printSolutions() const {
        if (solutions_.empty()) {
            std::cout << "No solution found" << std::endl;
            return;
        }
        if (solutions_.size() > 1) {
            std::cout << solutions_.size() << " solutions found." << std::endl;
        }
        for (std::size_t i = 0; i < solutions_.size(); ++i) {
            if (solutions_.size() > 1) {
                std::cout << "Solution " << (i + 1) << " of "
                          << solutions_.size() << ":" << std::endl;
            } else {
                std::cout << "Solution:" << std::endl;
            }
            printGrid(solutions_[i], false);
            if (i + 1 < solutions_.size()) {
                std::cout << std::endl;  // Blank line between solutions.
            }
        }
    }

    // Prints the reason(s) a puzzle could not be processed, then the verdict.
    void printProblems() const {
        for (const std::string& conflict : conflicts_) {
            std::cout << "Invalid puzzle: " << conflict << std::endl;
        }
        if (!errorMessage_.empty()) {
            std::cout << "Error: " << errorMessage_ << std::endl;
        }
        std::cout << "No solution found" << std::endl;
    }

    // ---- Accessors -------------------------------------------------------
    const std::string& getFilename() const { return filename_; }

private:
    // ---- Helper methods --------------------------------------------------

    // Maps a cell to its 3x3 box number, 0..8 in row-major order.
    static int boxIndex(int row, int col) {
        return (row / kBoxSize) * kBoxSize + (col / kBoxSize);
    }

    // Bitmask of digits (bits 1..9) that may legally go in the given cell.
    int candidateMask(int row, int col) const {
        const int used = rowMask_[row] | colMask_[col] | boxMask_[boxIndex(row, col)];
        return ~used & 0x3FE;  // 0x3FE keeps only bits 1..9.
    }

    // Places (or removes) a digit and updates the three bitmasks to match.
    void placeDigit(int row, int col, int digit) {
        const int bit = 1 << digit;
        grid_[row][col] = digit;
        rowMask_[row] |= bit;
        colMask_[col] |= bit;
        boxMask_[boxIndex(row, col)] |= bit;
    }

    void removeDigit(int row, int col, int digit) {
        const int bit = 1 << digit;
        grid_[row][col] = kEmpty;
        rowMask_[row] &= ~bit;
        colMask_[col] &= ~bit;
        boxMask_[boxIndex(row, col)] &= ~bit;
    }

    /*
     * Recursive depth-first search with backtracking.
     *
     * Each call picks the empty cell with the fewest legal candidates (this
     * prunes the search tree dramatically), tries each candidate in turn, and
     * recurses. If every cell is filled, the grid is saved as a solution.
     * The search does not stop at the first solution, so all are collected.
     */
    void backtrack() {
        int bestRow = -1;
        int bestCol = -1;
        int bestMask = 0;
        std::size_t bestCount = kSize + 1;

        // Scan for the most constrained empty cell.
        for (int row = 0; row < kSize; ++row) {
            for (int col = 0; col < kSize; ++col) {
                if (grid_[row][col] != kEmpty) {
                    continue;
                }
                const int mask = candidateMask(row, col);
                const std::size_t count = std::bitset<kSize + 1>(mask).count();
                if (count == 0) {
                    return;  // Dead end: this cell has no legal digit.
                }
                if (count < bestCount) {
                    bestCount = count;
                    bestRow = row;
                    bestCol = col;
                    bestMask = mask;
                }
            }
        }

        // Base case: no empty cells remain, so the grid is a full solution.
        if (bestRow == -1) {
            solutions_.push_back(grid_);
            return;
        }

        // Recursive case: try every legal digit in the chosen cell.
        for (int digit = 1; digit <= kSize; ++digit) {
            if (!(bestMask & (1 << digit))) {
                continue;  // Digit not allowed here.
            }
            placeDigit(bestRow, bestCol, digit);  // Make the choice...
            backtrack();                          // ...explore deeper...
            removeDigit(bestRow, bestCol, digit); // ...then undo (backtrack).
        }
    }

    // Prints a grid, one row per line, digits separated by spaces.
    static void printGrid(const Grid& grid, bool showUnderscoreForEmpty) {
        for (int row = 0; row < kSize; ++row) {
            for (int col = 0; col < kSize; ++col) {
                if (col > 0) {
                    std::cout << ' ';
                }
                if (grid[row][col] == kEmpty) {
                    std::cout << (showUnderscoreForEmpty ? '_' : '.');
                } else {
                    std::cout << grid[row][col];
                }
            }
            std::cout << std::endl;
        }
    }

    // ---- Data members ----------------------------------------------------
    std::string filename_;                 // Source file name
    Grid grid_;                            // Working grid mutated during search
    Grid originalGrid_;                    // Puzzle exactly as read
    std::array<int, kSize> rowMask_;       // Used digits per row
    std::array<int, kSize> colMask_;       // Used digits per column
    std::array<int, kSize> boxMask_;       // Used digits per 3x3 box
    std::vector<Grid> solutions_;          // Every solution discovered
    std::vector<std::string> conflicts_;   // Conflicts found in the givens
    std::string errorMessage_;             // File/format error, if any
};

/*
 * PuzzleRunner
 * ------------
 * Drives the program: processes each puzzle file in order and prints
 * the report for it.
 */
class PuzzleRunner {
public:
    // Processes puzzle1.txt through puzzle5.txt.
    void runAll() const {
        constexpr int kPuzzleCount = 5;
        for (int number = 1; number <= kPuzzleCount; ++number) {
            runOne("puzzle" + std::to_string(number) + ".txt");
            if (number < kPuzzleCount) {
                std::cout << std::endl
                          << "----------------------------------------"
                          << std::endl << std::endl;
            }
        }
    }

private:
    // Loads, validates, solves, and reports a single puzzle file.
    void runOne(const std::string& filename) const {
        std::cout << "Puzzle file: " << filename << std::endl;

        SudokuSolver solver(filename);

        // Step 1: read the file. A missing or malformed file ends here.
        if (!solver.loadPuzzle()) {
            solver.printProblems();
            return;
        }

        // Step 2: show the puzzle as read.
        std::cout << "Original puzzle:" << std::endl;
        solver.printOriginal();
        std::cout << std::endl;

        // Step 3: reject puzzles whose givens already conflict.
        if (!solver.validateInitialPuzzle()) {
            solver.printProblems();
            return;
        }

        // Step 4: search for all solutions and print them.
        solver.solve();
        solver.printSolutions();
    }
};

// Program entry point: delegates all work to PuzzleRunner.
int main() {
    PuzzleRunner runner;
    runner.runAll();
    return 0;
}