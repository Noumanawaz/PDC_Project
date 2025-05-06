#include <iostream>
#include <vector>
#include <algorithm>
#include <fstream>
#include <string>
#include <iomanip>
#include <chrono>

using namespace std;

// Toggle output generation
#define OUTPUT_ENABLED 1

// Precomputed factorials up to n=10
const vector<long long> precomputed_factorials = {1, 1, 2, 6, 24, 120, 720, 5040, 40320, 362880, 3628800};

// Swap the position of value x with the next value in sequence s
void exchange_adjacent(vector<int>& s, int x) {
    auto pos = find(s.begin(), s.end(), x) - s.begin();
    if (pos < s.size() - 1) {
        swap(s[pos], s[pos + 1]);
    }
}

// Find the rightmost index where the value is not in its sorted position
int find_misplaced_rightmost(const vector<int>& s) {
    int len = s.size();
    for (int i = len - 1; i >= 0; --i) {
        if (s[i] != i + 1) {
            return i + 1;
        }
    }
    return 0;
}

// Compute the parent when the last element is n and tree index ≠ n-1
vector<int> locate_ancestor(const vector<int>& s, int tree_idx, int len) {
    vector<int> sorted_seq(len);
    for (int i = 0; i < len; ++i) sorted_seq[i] = i + 1;
    vector<int> outcome = s;
    if (tree_idx == 2) {
        exchange_adjacent(outcome, tree_idx);
        if (outcome == sorted_seq) {
            outcome = s;
            exchange_adjacent(outcome, tree_idx - 1);
            return outcome;
        }
    } else if (s[len - 2] == tree_idx || s[len - 2] == len - 1) {
        int pos = find_misplaced_rightmost(s);
        if (pos > 0 && pos <= len) {
            exchange_adjacent(outcome, s[pos - 1]);
            return outcome;
        }
    }
    exchange_adjacent(outcome, tree_idx);
    return outcome;
}

// Compute the parent of sequence s in the t-th spanning tree
vector<int> compute_parent(const vector<int>& s, int tree_idx, int len) {
    vector<int> sorted_seq(len);
    for (int i = 0; i < len; ++i) sorted_seq[i] = i + 1;
    vector<int> outcome = s;
    if (s[len - 1] == len) {
        if (tree_idx != len - 1) {
            return locate_ancestor(s, tree_idx, len);
        } else {
            exchange_adjacent(outcome, s[len - 2]);
            return outcome;
        }
    } else if (s[len - 1] == len - 1 && s[len - 2] == len) {
        exchange_adjacent(outcome, len);
        if (outcome != sorted_seq) {
            if (tree_idx == 1) {
                return outcome;
            } else {
                outcome = s;
                exchange_adjacent(outcome, tree_idx - 1);
                return outcome;
            }
        }
    }
    if (s[len - 1] == tree_idx) {
        exchange_adjacent(outcome, len);
    } else {
        exchange_adjacent(outcome, tree_idx);
    }
    return outcome;
}

// Generate the k-th permutation in lexicographical order
vector<int> generate_kth_sequence(int len, long long k) {
    vector<int> seq(len);
    vector<bool> used(len + 1, false);
    for (int i = 0; i < len; ++i) {
        int quotient = k / precomputed_factorials[len - 1 - i];
        k %= precomputed_factorials[len - 1 - i];
        int count = 0;
        for (int j = 1; j <= len; ++j) {
            if (!used[j]) {
                if (count == quotient) {
                    seq[i] = j;
                    used[j] = true;
                    break;
                }
                count++;
            }
        }
    }
    return seq;
}

int main() {
    // Timing variables
    auto begin_total = chrono::high_resolution_clock::now();
    auto begin_computation = chrono::high_resolution_clock::now();

    int dimension = 10; // Size of the bubble-sort network
    long long total_sequences = precomputed_factorials[dimension];
    long long max_output = 120; // Restrict output to first 5! sequences

    // Open file for results
    ofstream result_file("Sequential_output.txt");
    if (!result_file.is_open()) {
        cerr << "Error: Unable to open output file" << endl;
        return 1;
    }

    // Open file for timing
    ofstream timing_file("Sequential_timing.txt");
    if (!timing_file.is_open()) {
        cerr << "Error: Unable to open timing file" << endl;
        return 1;
    }

    // Write header for output
    if (OUTPUT_ENABLED) {
        result_file << left;
        result_file << setw(20) << "Vertex" << setw(10) << "Tree" << setw(20) << "Parent" << "\n";
        result_file << string(50, '-') << "\n";
    }

    // Process all sequences, output only the first max_output
    for (long long idx = 0; idx < total_sequences; ++idx) {
        vector<int> seq = generate_kth_sequence(dimension, idx);
        for (int t = 1; t <= dimension - 1; ++t) {
            vector<int> ancestor = compute_parent(seq, t, dimension);
            if (OUTPUT_ENABLED && idx < max_output) {
                string seq_str, ancestor_str;
                for (int val : seq) seq_str += to_string(val);
                for (int val : ancestor) ancestor_str += to_string(val);
                result_file << setw(20) << seq_str << setw(10) << t << setw(20) << ancestor_str << "\n";
            }
        }
    }

    auto end_computation = chrono::high_resolution_clock::now();
    auto begin_io = chrono::high_resolution_clock::now();

    result_file.close();

    auto end_io = chrono::high_resolution_clock::now();
    auto end_total = chrono::high_resolution_clock::now();

    // Write timing information
    timing_file << fixed << setprecision(6);
    timing_file << "Total Time: " << chrono::duration<double>(end_total - begin_total).count() << " seconds\n";
    timing_file << "Compute Time: " << chrono::duration<double>(end_computation - begin_computation).count() << " seconds\n";
    timing_file << "I/O Time: " << chrono::duration<double>(end_io - begin_io).count() << " seconds\n";
    timing_file.close();

    return 0;
}
