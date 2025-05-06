#include <iostream>
#include <vector>
#include <algorithm>
#include <cmath>
#include <omp.h>
#include <mpi.h>
#include <fstream>
#include <string>
#include <iomanip>
#include <sstream>

using namespace std;

// Enable/disable output
#define OUTPUT_ACTIVE 1

// Factorials up to n=10
const vector<long long> factorial_table = {1, 1, 2, 6, 24, 120, 720, 5040, 40320, 362880, 3628800};

// Swap value x with the next value in array a
void swap_next(vector<int>& a, int x) {
    auto idx = find(a.begin(), a.end(), x) - a.begin();
    if (idx < a.size() - 1) {
        swap(a[idx], a[idx + 1]);
    }
}

// Find the rightmost index where value is out of place
int get_rightmost_misplaced(const vector<int>& a) {
    int n = a.size();
    for (int i = n - 1; i >= 0; --i) {
        if (a[i] != i + 1) {
            return i + 1;
        }
    }
    return 0;
}

// Determine ancestor for array a when last element is n and tree ≠ n-1
vector<int> get_predecessor(const vector<int>& a, int tree, int n) {
    vector<int> ordered(n);
    for (int i = 0; i < n; ++i) ordered[i] = i + 1;
    vector<int> res = a;
    if (tree == 2) {
        swap_next(res, tree);
        if (res == ordered) {
            res = a;
            swap_next(res, tree - 1);
            return res;
        }
    } else if (a[n - 2] == tree || a[n - 2] == n - 1) {
        int j = get_rightmost_misplaced(a);
        if (j > 0 && j <= n) {
            swap_next(res, a[j - 1]);
            return res;
        }
    }
    swap_next(res, tree);
    return res;
}

// Compute the ancestor of array a in the tree-th spanning tree
vector<int> find_ancestor(const vector<int>& a, int tree, int n) {
    vector<int> ordered(n);
    for (int i = 0; i < n; ++i) ordered[i] = i + 1;
    vector<int> res = a;
    if (a[n - 1] == n) {
        if (tree != n - 1) {
            return get_predecessor(a, tree, n);
        } else {
            swap_next(res, a[n - 2]);
            return res;
        }
    } else if (a[n - 1] == n - 1 && a[n - 2] == n) {
        swap_next(res, n);
        if (res != ordered) {
            if (tree == 1) {
                return res;
            } else {
                res = a;
                swap_next(res, tree - 1);
                return res;
            }
        }
    }
    if (a[n - 1] == tree) {
        swap_next(res, n);
    } else {
        swap_next(res, tree);
    }
    return res;
}

// Generate the k-th sequence in lexicographical order
vector<int> create_sequence(int n, long long k) {
    vector<int> seq(n);
    vector<int> available(n);
    for (int i = 0; i < n; ++i) available[i] = i + 1;
    k = k % factorial_table[n];
    for (int i = 0; i < n; ++i) {
        long long fact = factorial_table[n - 1 - i];
        int idx = k / fact;
        k = k % fact;
        seq[i] = available[idx];
        available.erase(available.begin() + idx);
    }
    return seq;
}

int main(int argc, char** argv) {
    // Initialize MPI
    MPI_Init(&argc, &argv);
    int process_id, num_processes;
    MPI_Comm_rank(MPI_COMM_WORLD, &process_id);
    MPI_Comm_size(MPI_COMM_WORLD, &num_processes);

    // Configure OpenMP threads
    int thread_count = 1; // making sure mpi only
    omp_set_num_threads(thread_count);

    // Timing variables
    double begin_total, end_total, begin_calc, end_calc, begin_output, end_output;
    begin_total = MPI_Wtime();

    int dimension = 10; // Bubble-sort network size
    long long total_nodes = factorial_table[dimension];
    long long output_cap = 120; // Output limited to first 5! sequences

    // Distribute computation workload
    long long chunk_size = total_nodes / num_processes;
    long long start_index = process_id * chunk_size;
    long long end_index = (process_id == num_processes - 1) ? total_nodes : (process_id + 1) * chunk_size;

    // Distribute output workload
    long long output_chunk_size = output_cap / num_processes;
    long long output_start = process_id * output_chunk_size;
    long long output_end = (process_id == num_processes - 1) ? output_cap : (process_id + 1) * output_chunk_size;

    // Open results file
    ofstream result_stream("mpi_only_output_rank_" + to_string(process_id) + ".txt");
    if (!result_stream.is_open()) {
        cerr << "Process " << process_id << ": Failed to open output file" << endl;
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    // Open timing file
    ofstream timing_stream("mpi_only_timing_rank_" + to_string(process_id) + ".txt");
    if (!timing_stream.is_open()) {
        cerr << "Process " << process_id << ": Failed to open timing file" << endl;
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    // Write output header
    if (OUTPUT_ACTIVE) {
        result_stream << left;
        result_stream << setw(8) << "Rank" << setw(20) << "Vertex" << setw(10) << "Tree" << setw(20) << "Parent" << "\n";
        result_stream << string(58, '-') << "\n";
    }

    // Start computation timing
    begin_calc = MPI_Wtime();

    // Compute ancestors for assigned sequences
   // #pragma omp parallel
    {
        int thread_id = omp_get_thread_num();
       // #pragma omp for schedule(dynamic)
        for (long long i = start_index; i < end_index; ++i) {
            vector<int> seq = create_sequence(dimension, i);
            for (int t = 1; t <= dimension - 1; ++t) {
                vector<int> ancestor = find_ancestor(seq, t, dimension);
            }
        }
    }

    // End computation timing
    end_calc = MPI_Wtime();

    // Start output timing
    begin_output = MPI_Wtime();

    // Output assigned portion of first 120 sequences
    if (OUTPUT_ACTIVE) {
        vector<stringstream> thread_buffers(thread_count);
        //#pragma omp parallel
        {
            int thread_id = omp_get_thread_num();
         //   #pragma omp for schedule(dynamic)
            for (long long i = output_start; i < output_end; ++i) {
                vector<int> seq = create_sequence(dimension, i);
                for (int t = 1; t <= dimension - 1; ++t) {
                    vector<int> ancestor = find_ancestor(seq, t, dimension);
                    string seq_str, ancestor_str;
                    for (int val : seq) seq_str += to_string(val);
                    for (int val : ancestor) ancestor_str += to_string(val);
                    thread_buffers[thread_id] << setw(8) << process_id
                                             << setw(20) << seq_str
                                             << setw(10) << t
                                             << setw(20) << ancestor_str << "\n";
                }
            }
        }
        // Write buffers to file
        for (int tid = 0; tid < thread_count; ++tid) {
            result_stream << thread_buffers[tid].str();
        }
    }

    // End output timing
    end_output = MPI_Wtime();

    result_stream.close();

    // End total timing
    end_total = MPI_Wtime();

    // Write timing metrics
    timing_stream << fixed << setprecision(6);
    timing_stream << "Total Time: " << (end_total - begin_total) << " seconds\n";
    timing_stream << "Compute Time: " << (end_calc - begin_calc) << " seconds\n";
    timing_stream << "I/O Time: " << (end_output - begin_output) << " seconds\n";
    timing_stream << "MPI Processes: " << num_processes << "\n";
    timing_stream << "OpenMP Threads per Process: " << thread_count << "\n";
    timing_stream << "Total Threads: " << (num_processes * thread_count) << "\n";
    timing_stream.close();

    MPI_Finalize();
    return 0;
}

