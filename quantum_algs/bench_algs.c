#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>

#include "sylvan.h"
#include "sylvan_qdd_complex.h"
#include "grover.h"
#include "random_circuit.h"
#include "shor.h"
#include "supremacy.h"

#ifdef HAVE_PROFILER
#include <gperftools/profiler.h>
static char* profile_name = NULL; //"bench_qdd.prof";
#endif

// don't use for heap / malloced arrays
#define len(x) (sizeof(x) / sizeof(x[0]))

static bool VERBOSE = false;

static size_t min_tablesize;
static size_t max_tablesize;
static size_t min_cachesize;
static size_t max_cachesize;
static size_t ctable_size;
static double ctable_tolerance;
static double ctable_gc_thres;
static int caching_granularity;

/**
 * Obtain current wallclock time
 */
static double
wctime()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec + 1E-6 * tv.tv_usec);
}

static void
write_parameters(FILE *file)
{
    fprintf(file, "{\n");
    fprintf(file, "  \"min_tablesize\": %ld,\n", min_tablesize);
    fprintf(file, "  \"max_tablesize\": %ld,\n", max_tablesize);
    fprintf(file, "  \"min_cachesize\": %ld,\n", min_cachesize);
    fprintf(file, "  \"max_cachesize\": %ld,\n", max_cachesize);
    fprintf(file, "  \"ctable_size\": %ld,\n", ctable_size);
    fprintf(file, "  \"ctable_tolerance\": %.5e,\n", ctable_tolerance);
    fprintf(file, "  \"ctable_gc_thres\": %lf,\n", ctable_gc_thres);
    fprintf(file, "  \"propagate_complex\": %d,\n", propagate_complex);
    fprintf(file, "  \"caching_granularity\": %d\n", caching_granularity);
    fprintf(file, "}\n");
    fclose(file);
}


int bench_supremacy_5_1(uint32_t depth, uint32_t workers)
{
    // 5x1 "grid" from [Characterizing Quantum Supremacy in Near-Term Devices]
    printf("bench sup5_1, depth %3d, %2d worker(s), ", depth, workers);
    fflush(stdout);

    uint64_t node_count;
    double t_start, t_end, runtime;
    t_start = wctime();

    // Init Lace
    lace_init(workers, 0);
    lace_startup(0, NULL, NULL);
    LACE_ME;

    // Init Sylvan
    sylvan_set_limits(4LL<<30, 1, 6);
    sylvan_init_package();
    sylvan_init_qdd(1LL<<23, -1);

    srand(42);
    QDD res = supremacy_5_1_circuit(depth);
    node_count = qdd_countnodes(res);

    t_end = wctime();
    runtime = (t_end - t_start);

    printf("%4ld nodes, %lf sec\n", node_count, runtime);

    // Cleanup
    sylvan_quit();
    lace_exit();

    return 0;
}

double bench_supremacy_5_4_once(uint32_t depth, uint32_t workers, uint64_t rseed, char *fpath, uint64_t *nodes_peak, uint64_t *n_gates)
{
    if (VERBOSE) {
        printf("bench sup5_4, depth %3d, %2d worker(s), ", depth, workers);
        fflush(stdout);
    }

    FILE *logfile = NULL;
    if (fpath != NULL)
        logfile = fopen(fpath, "w");

    uint64_t node_count;
    double t_start, t_end, runtime;
    t_start = wctime();

    // Init Lace
    lace_init(workers, 0);
    lace_startup(0, NULL, NULL);

    // Init Sylvan
    sylvan_set_sizes(min_tablesize, max_tablesize, min_cachesize, max_cachesize);
    sylvan_init_package();
    sylvan_init_qdd(ctable_size, ctable_tolerance);
    qdd_set_gc_ctable_thres(ctable_gc_thres);
    qdd_set_caching_granularity(caching_granularity);

    qdd_stats_start(logfile);

    srand(rseed);
    QDD res = supremacy_5_4_circuit(depth);

    if (nodes_peak != NULL) *nodes_peak = qdd_stats_get_nodes_peak();
    if (n_gates    != NULL) *n_gates = qdd_stats_get_logcounter();
    if (logfile    != NULL) qdd_stats_finish();

    t_end = wctime();
    runtime = (t_end - t_start);

    if (VERBOSE) {
        node_count = qdd_countnodes(res); 
        printf("%4ld nodes, %lf sec\n", node_count, runtime);
    }

    // Cleanup
    sylvan_quit();
    lace_exit();

    return runtime;
}

double bench_random_circuit_once(int qubits, int gates, int workers, uint64_t rseed,
                                 char *fpath, uint64_t *nodes_peak, double *nodes_avg,
                                 uint64_t *n_gates)
{
    if (VERBOSE) {
        printf("bench random circuit: %2d qubits, %d worker(s), ", qubits, workers);
        fflush(stdout);
    }

    FILE *logfile = NULL;
    if (fpath != NULL)
        logfile = fopen(fpath, "w");

    uint64_t node_count;
    double t_start, t_end, runtime;
    t_start = wctime();

    // Init Lace
    lace_init(workers, 0);
    lace_startup(0, NULL, NULL);

    // Init Sylvan
    sylvan_set_sizes(min_tablesize, max_tablesize, min_cachesize, max_cachesize);
    sylvan_init_package();
    sylvan_init_qdd(ctable_size, ctable_tolerance);
    qdd_set_gc_ctable_thres(ctable_gc_thres);
    qdd_set_caching_granularity(caching_granularity);

    qdd_stats_start(logfile);

    // Random circuit
    QDD qdd = qdd_run_random_single_qubit_gates(qubits, gates, rseed);

    if (nodes_peak != NULL) *nodes_peak = qdd_stats_get_nodes_peak();
    if (nodes_avg  != NULL) *nodes_avg = qdd_stats_get_nodes_avg();
    if (n_gates    != NULL) *n_gates = qdd_stats_get_logcounter();
    if (logfile    != NULL) qdd_stats_finish();

    t_end = wctime();
    runtime = (t_end - t_start);

    if (VERBOSE) {
        node_count = qdd_countnodes(qdd); 
        printf("%4ld nodes, %lf sec\n", node_count, runtime);
    }

    // Cleanup
    sylvan_quit();
    lace_exit();

    return runtime;

    return 0;
}


double bench_grover_once(int num_bits, bool flag[], int workers, char *fpath, uint64_t *nodes_peak, uint64_t *n_gates)
{
    if (VERBOSE) {
        printf("bench grover, %d qubits, %2d worker(s), ", num_bits+1, workers); 
        printf("flag = [");
        for (int i = 0; i < num_bits; i++)
            printf("%d",flag[i]);
        printf("], ");
        fflush(stdout);
    }

    FILE *logfile = NULL;
    if (fpath != NULL)
        logfile = fopen(fpath, "w");

    double t_start, t_end, runtime;
    t_start = wctime();

    // Init Lace
    lace_init(workers, 0);
    lace_startup(0, NULL, NULL);

    // Init Sylvan
    sylvan_set_sizes(min_tablesize, max_tablesize, min_cachesize, max_cachesize);
    sylvan_init_package();
    sylvan_init_qdd(ctable_size, ctable_tolerance);
    qdd_set_gc_ctable_thres(ctable_gc_thres);
    qdd_set_caching_granularity(caching_granularity);

    QDD grov;
    uint64_t node_count_end;
    
    qdd_stats_start(logfile);

    grov = qdd_grover(num_bits, flag);

    if (nodes_peak != NULL) *nodes_peak = qdd_stats_get_nodes_peak();
    if (n_gates    != NULL) *n_gates = qdd_stats_get_logcounter();
    if (logfile    != NULL) qdd_stats_finish();

    t_end = wctime();
    runtime = (t_end - t_start);

    if (VERBOSE) {
        node_count_end = qdd_countnodes(grov);
        double prob = comp_to_prob(comp_value(qdd_get_amplitude(grov, flag)))*2;
        uint64_t np = (nodes_peak == NULL) ? 0 : *nodes_peak;
        printf("%ld nodes end (%ld peak), Pr(flag)=%.3lf, %lf sec\n", node_count_end, np, prob, runtime);
    }

    if (logfile != NULL)
        fclose(logfile);

    // Cleanup
    sylvan_quit();
    lace_exit();
    return runtime;
}

double bench_shor_once(uint64_t N, uint64_t a, int workers, int rseed, bool *success, char *fpath, uint64_t *nodes_peak, uint64_t *n_gates)
{
    if (VERBOSE) {
        uint32_t num_qubits = (int)ceil(log2(N))*2 + 3;
        printf("bench shor, factor %ld (%d qubits), %2d worker(s), ", N, num_qubits, workers); 
        fflush(stdout);
    }

    FILE *logfile = NULL;
    if (fpath != NULL)
        logfile = fopen(fpath, "w");

    double t_start, t_end, runtime;
    t_start = wctime();

    // Init Lace
    lace_init(workers, 0);
    lace_startup(0, NULL, NULL);

    // Init Sylvan
    sylvan_set_sizes(min_tablesize, max_tablesize, min_cachesize, max_cachesize);
    sylvan_init_package();
    sylvan_init_qdd(ctable_size, ctable_tolerance);
    qdd_set_gc_ctable_thres(ctable_gc_thres);
    qdd_set_caching_granularity(caching_granularity);

    qdd_stats_start(logfile);

    srand(rseed);
    uint64_t fac = run_shor(N, a, false);

    t_end = wctime();
    runtime = (t_end - t_start);

    if (nodes_peak != NULL) *nodes_peak = qdd_stats_get_nodes_peak();
    if (n_gates    != NULL) *n_gates = qdd_stats_get_logcounter();
    if (logfile    != NULL) {
        qdd_stats_finish();
        fclose(logfile);
    }
    *success = (fac == 0) ? 0 : 1;

    if (VERBOSE) printf("found factor %ld, %lf sec\n", fac, runtime);

    // Cleanup
    sylvan_quit();
    lace_exit();
    return runtime;
}

int bench_random_circuit()
{
    VERBOSE = true;

    // output dir
    mkdir("benchmark_data/random_circuit/", 0700);
    char output_dir[256];
    sprintf(output_dir, "benchmark_data/random_circuit/%ld/", time(NULL));
    mkdir(output_dir, 0700);
    char history_dir[256];
    strcpy(history_dir, output_dir);
    strcat(history_dir, "run_histories/");
    mkdir(history_dir, 0700);
    // output file for runtime data
    char overview_fname[256];
    strcpy(overview_fname, output_dir);
    strcat(overview_fname, "summary.csv");
    FILE *overview_file = fopen(overview_fname, "w");
    fprintf(overview_file, "qubits, rseed, peak_nodes, avg_nodes, workers, "
                           "gates, runtime, avg_gate_time, "
                           "plus_cacheput, plus_cached, "
                           "gate_cacheput, gate_cached, "
                           "cgate_cacheput, cgate_cached\n");
    // output file for sylvan parameters
    char param_fname[256];
    strcpy(param_fname, output_dir);
    strcat(param_fname, "parameters.json");
    FILE *param_file = fopen(param_fname, "w");

    // sylvan / qdd params
    min_tablesize = max_tablesize = 1LL<<30;
    min_cachesize = max_cachesize = 1LL<<16;
    ctable_size   = 1LL<<23;
    ctable_gc_thres = 0.25;
    ctable_tolerance = 1e-14;
    caching_granularity = 1;
    write_parameters(param_file);

    // params
    int nqubits[] = {10,20,30,40,50,60,70,80,90,100};
    int ngates[] = {1000};

    // different number of workers to test
    int nworkers[] = {1};

    // re-runs for different depths
    int reruns = 2;

    // runtimes are written to single file
    double runtime, avg_gate_time, nodes_avg;
    uint64_t nodes_peak, n_gates;
    uint64_t plus_cacheput, gate_cacheput, cgate_cacheput;
    uint64_t plus_cached, gate_cached, cgate_cached;

    for (uint32_t q = 0; q < len(nqubits); q++) {
        for (uint32_t g = 0; g < len(ngates); g++) {
            for (int r = 0; r < reruns; r++) {
                uint64_t rseed = rand();
                for (uint32_t w = 0; w < len(nworkers); w++) {

                    // output file for history of this run
                    char history_path[256];
                    char history_fname[256];
                    sprintf(history_fname, "rand_q%d_g%d_w%d_rseed%ld.csv", nqubits[q], ngates[g], nworkers[w], rseed);
                    strcpy(history_path, history_dir);
                    strcat(history_path, history_fname);

                    // bench twice, once with logging and once for timing
                    runtime = bench_random_circuit_once(nqubits[q], ngates[g], nworkers[w], rseed, NULL, NULL, NULL, NULL);
                    bench_random_circuit_once(nqubits[q], ngates[g], nworkers[w], rseed, history_path, &nodes_peak, &nodes_avg, &n_gates);

                    // add summary of this run to overview file
                    avg_gate_time = runtime / (double) n_gates;
                    #if SYLVAN_STATS
                    plus_cacheput  = sylvan_stats.counters[QDD_PLUS_CACHEDPUT];
                    gate_cacheput  = sylvan_stats.counters[QDD_GATE_CACHEDPUT];
                    cgate_cacheput = sylvan_stats.counters[QDD_CGATE_CACHEDPUT];
                    plus_cached    = sylvan_stats.counters[QDD_PLUS_CACHED];
                    gate_cached    = sylvan_stats.counters[QDD_GATE_CACHED];
                    cgate_cached   = sylvan_stats.counters[QDD_CGATE_CACHED];
                    #else
                    plus_cached = gate_cached = cgate_cached = 0;
                    plus_cacheput = gate_cacheput = cgate_cacheput = 0;
                    #endif
                    fprintf(overview_file, "%d, %ld, %ld, %lf, %d, %d, %lf, %.3e, %ld, %ld, %ld, %ld, %ld, %ld\n",
                                            nqubits[q], rseed, nodes_peak, nodes_avg, nworkers[w],
                                            ngates[g], runtime, avg_gate_time,
                                            plus_cacheput, plus_cached,
                                            gate_cacheput, gate_cached,
                                            cgate_cacheput, cgate_cached);
                }
            }
        }
    }

    return 0;
}

int bench_supremacy()
{
    VERBOSE = true;

    // output dir
    mkdir("benchmark_data/supremacy/", 0700);
    char output_dir[256];
    sprintf(output_dir, "benchmark_data/supremacy/%ld/", time(NULL));
    mkdir(output_dir, 0700);
    char history_dir[256];
    strcpy(history_dir, output_dir);
    strcat(history_dir, "run_histories/");
    mkdir(history_dir, 0700);
    // output file for runtime data
    char overview_fname[256];
    strcpy(overview_fname, output_dir);
    strcat(overview_fname, "summary.csv");
    FILE *overview_file = fopen(overview_fname, "w");
    fprintf(overview_file, "qubits, depth, rseed, peak_nodes, workers, "
                           "gates, runtime, avg_gate_time, "
                           "plus_cacheput, plus_cached, "
                           "gate_cacheput, gate_cached, "
                           "cgate_cacheput, cgate_cached\n");
    // output file for sylvan parameters
    char param_fname[256];
    strcpy(param_fname, output_dir);
    strcat(param_fname, "parameters.json");
    FILE *param_file = fopen(param_fname, "w");

    // sylvan / qdd params
    min_tablesize = max_tablesize = 1LL<<30;
    min_cachesize = max_cachesize = 1LL<<16;
    ctable_size   = 1LL<<23;
    ctable_gc_thres = 0.25;
    ctable_tolerance = 1e-14;
    caching_granularity = 1;
    write_parameters(param_file);

    // params
    int nqubits = 20; // always 20 for 5x4 grid
    int depths[] = {15};//{15,16,17,18,19,20};
    int ndepths = 1;

    // different number of workers to test
    int n_workers[] = {1, 2, 4};
    int nn_workers  = 3;

    // re-runs for different depths
    int re_runs = 2;
    uint64_t rseeds[] = {66, 123};

    // runtimes are written to single file
    double runtime, avg_gate_time;
    uint64_t nodes_peak, n_gates;
    uint64_t plus_cacheput, gate_cacheput, cgate_cacheput;
    uint64_t plus_cached, gate_cached, cgate_cached;

    for (int i = 0; i < ndepths; i++) {
        for (int r = 0; r < re_runs; r++) {
            uint64_t rseed = rseeds[r];
            for (int w = 0; w < nn_workers; w++) {

                // output file for history of this run
                char history_path[256];
                char history_fname[256];
                sprintf(history_fname, "sup5x4_d%d_w%d_rseed%ld.csv", depths[i], n_workers[w], rseed);
                strcpy(history_path, history_dir);
                strcat(history_path, history_fname);

                // bench twice, once with logging and once for timing
                runtime = bench_supremacy_5_4_once(depths[i], n_workers[w], rseed, NULL, NULL, NULL);
                bench_supremacy_5_4_once(depths[i], n_workers[w], rseed, history_path, &nodes_peak, &n_gates);

                // add summary of this run to overview file
                avg_gate_time = runtime / (double) n_gates;
                #if SYLVAN_STATS
                plus_cacheput  = sylvan_stats.counters[QDD_PLUS_CACHEDPUT];
                gate_cacheput  = sylvan_stats.counters[QDD_GATE_CACHEDPUT];
                cgate_cacheput = sylvan_stats.counters[QDD_CGATE_CACHEDPUT];
                plus_cached    = sylvan_stats.counters[QDD_PLUS_CACHED];
                gate_cached    = sylvan_stats.counters[QDD_GATE_CACHED];
                cgate_cached   = sylvan_stats.counters[QDD_CGATE_CACHED];
                #else
                plus_cached = gate_cached = cgate_cached = 0;
                plus_cacheput = gate_cacheput = cgate_cacheput = 0;
                #endif
                fprintf(overview_file, "%d, %d, %ld, %ld, %d, %ld, %lf, %.3e, %ld, %ld, %ld, %ld, %ld, %ld\n",
                                        nqubits, depths[i], rseed, nodes_peak, n_workers[w],
                                        n_gates, runtime, avg_gate_time,
                                        plus_cacheput, plus_cached,
                                        gate_cacheput, gate_cached,
                                        cgate_cacheput, cgate_cached);
            }
        }
    }

    return 0;
}

int bench_grover()
{
    VERBOSE = true;
    
    // output dir
    mkdir("benchmark_data/grover/", 0700);
    char output_dir[256];
    sprintf(output_dir, "benchmark_data/grover/%ld/", time(NULL));
    mkdir(output_dir, 0700);
    char history_dir[256];
    strcpy(history_dir, output_dir);
    strcat(history_dir, "run_histories/");
    mkdir(history_dir, 0700);
    // output file for runtime data
    char overview_fname[256];
    strcpy(overview_fname, output_dir);
    strcat(overview_fname, "summary.csv");
    FILE *overview_file = fopen(overview_fname, "w");
    fprintf(overview_file, "qubits, peak_nodes, workers, "
                           "gates, runtime, avg_gate_time, "
                           "plus_cacheput, plus_cached, "
                           "gate_cacheput, gate_cached, "
                           "cgate_cacheput, cgate_cached, "
                           "flag\n");
    // output file for sylvan parameters
    char param_fname[256];
    strcpy(param_fname, output_dir);
    strcat(param_fname, "parameters.json");
    FILE *param_file = fopen(param_fname, "w");

    // sylvan / qdd params
    min_tablesize = max_tablesize = 1LL<<25;
    min_cachesize = max_cachesize = 1LL<<16;
    ctable_size   = 1LL<<18;
    ctable_tolerance = 1e-14;
    ctable_gc_thres = 0.5;
    caching_granularity = 1;
    write_parameters(param_file);

    // different number of bits for the flag to test
    int n_bits[] = {15, 20};
    int nn_bits  = 2;
    
    // different number of workers to test
    int n_workers[] = {1, 2, 4};
    int nn_workers  = 3;

    // different number of random flags to test
    int n_flags = 3;
    bool *flag;
    int f_int;

    // runtimes are written to single file
    double runtime, avg_gate_time;
    uint64_t nodes_peak, n_gates;
    uint64_t plus_cacheput, gate_cacheput, cgate_cacheput;
    uint64_t plus_cached, gate_cached, cgate_cached;

    // run benchmarks
    srand(42);
    for (int q = 0; q < nn_bits; q++) {

        for (int f = 0; f < n_flags; f++) {
            flag  = qdd_grover_random_flag(n_bits[q]);
            f_int = bitarray_to_int(flag, n_bits[q], true);

            for (int w = 0; w < nn_workers; w++) {

                // output file for history of this run
                char history_path[256];
                char history_fname[256];
                sprintf(history_fname, "grov_hist_n%d_w%d_f%d.csv", n_bits[q]+1, n_workers[w], f_int);
                strcpy(history_path, history_dir);
                strcat(history_path, history_fname);

                // bench twice, once with logging and once for timing
                runtime = bench_grover_once(n_bits[q], flag, n_workers[w], NULL, NULL, NULL);
                bench_grover_once(n_bits[q], flag, n_workers[w], history_path, &nodes_peak, &n_gates);

                // add summary of this run to overview file
                avg_gate_time = runtime / (double) n_gates;
                #if SYLVAN_STATS
                plus_cacheput  = sylvan_stats.counters[QDD_PLUS_CACHEDPUT];
                gate_cacheput  = sylvan_stats.counters[QDD_GATE_CACHEDPUT];
                cgate_cacheput = sylvan_stats.counters[QDD_CGATE_CACHEDPUT];
                plus_cached    = sylvan_stats.counters[QDD_PLUS_CACHED];
                gate_cached    = sylvan_stats.counters[QDD_GATE_CACHED];
                cgate_cached   = sylvan_stats.counters[QDD_CGATE_CACHED];
                #else
                plus_cached = gate_cached = cgate_cached = 0;
                plus_cacheput = gate_cacheput = cgate_cacheput = 0;
                #endif
                fprintf(overview_file, "%d, %ld, %d, %ld, %lf, %.3e, %ld, %ld, %ld, %ld, %ld, %ld, %d\n",
                                        n_bits[q]+1, nodes_peak, n_workers[w],
                                        n_gates, runtime, avg_gate_time, 
                                        plus_cacheput, plus_cached,
                                        gate_cacheput, gate_cached,
                                        cgate_cacheput, cgate_cached,
                                        f_int);
            }
        }
    }

    fclose(overview_file);

    return 0;
}

int bench_shor()
{
    VERBOSE = true;
    
    // output dir
    mkdir("benchmark_data/shor/", 0700);
    char output_dir[256];
    sprintf(output_dir, "benchmark_data/shor/%ld/", time(NULL));
    mkdir(output_dir, 0700);
    char history_dir[256];
    strcpy(history_dir, output_dir);
    strcat(history_dir, "run_histories/");
    mkdir(history_dir, 0700);
    // output file for runtime data
    char overview_fname[256];
    strcpy(overview_fname, output_dir);
    strcat(overview_fname, "summary.csv");
    FILE *overview_file = fopen(overview_fname, "w");
    fprintf(overview_file, "N, a, qubits, peak_nodes, success, "
                           "workers, gates, runtime, avg_gate_time, "
                           "plus_cacheput, plus_cached, "
                           "gate_cacheput, gate_cached, "
                           "cgate_cacheput, cgate_cached\n");
    // output file for sylvan parameters
    char param_fname[256];
    strcpy(param_fname, output_dir);
    strcat(param_fname, "parameters.json");
    FILE *param_file = fopen(param_fname, "w");

    // sylvan / qdd params
    min_tablesize = max_tablesize = 1LL<<25;
    min_cachesize = max_cachesize = 1LL<<16;
    ctable_size   = 1LL<<20;
    ctable_gc_thres = 0.5;
    ctable_tolerance = 1e-14;
    caching_granularity = 1;
    write_parameters(param_file);

    // Different sized N to test
    //  3 x  5 =  15 (11 qubits)
    //  5 x  7 =  35 (15 qubits)
    // 11 x 13 = 143 (19 qubits)
    int Ns[] = {15, 35, 143};
    int nN = 3;
    uint64_t N, a;
    uint32_t nqubits;

    // how often to re-run the same (N,a) (TODO: different 'a' for each run?)
    int re_runs = 2;
    
    // different number of workers to test
    int n_workers[] = {1, 2, 4};
    int nn_workers  = 3;

    // runtimes are written to single file
    double runtime, avg_gate_time;
    uint64_t nodes_peak, n_gates;
    uint64_t plus_cacheput, gate_cacheput, cgate_cacheput;
    uint64_t plus_cached, gate_cached, cgate_cached;

    // run benchmarks
    srand(42);
    for (int q = 0; q < nN; q++) {
        N = Ns[q];
        a = shor_generate_a(N);
        nqubits = (int)ceil(log2(N))*2 + 3;
        for (int r = 0; r < re_runs; r++) {
            uint64_t rseed = rand();
            for (int w = 0; w < nn_workers; w++) {
                
                // output file for history of this run
                char history_path[256];
                char history_fname[256];
                sprintf(history_fname, "shor_hist_n%d_w%d_N%ld_a%ld_r%d.csv", nqubits, n_workers[w], N,a,r);
                strcpy(history_path, history_dir);
                strcat(history_path, history_fname);

                // bench twice, once with logging and once for timing
                bool success;
                runtime = bench_shor_once(N, a, n_workers[w], rseed, &success, NULL, NULL, NULL);
                bench_shor_once(N, a, n_workers[w], rseed, &success, history_path, &nodes_peak, &n_gates);

                // add summary of this run to overview file
                avg_gate_time = runtime / (double) n_gates;
                #if SYLVAN_STATS
                plus_cacheput  = sylvan_stats.counters[QDD_PLUS_CACHEDPUT];
                gate_cacheput  = sylvan_stats.counters[QDD_GATE_CACHEDPUT];
                cgate_cacheput = sylvan_stats.counters[QDD_CGATE_CACHEDPUT];
                plus_cached    = sylvan_stats.counters[QDD_PLUS_CACHED];
                gate_cached    = sylvan_stats.counters[QDD_GATE_CACHED];
                cgate_cached   = sylvan_stats.counters[QDD_CGATE_CACHED];
                #else
                plus_cached = gate_cached = cgate_cached = 0;
                plus_cacheput = gate_cacheput = cgate_cacheput = 0;
                #endif
                fprintf(overview_file, "%ld, %ld, %d, %ld, %d, %d, %ld, %lf, %.3e, %ld, %ld, %ld, %ld, %ld, %ld\n",
                                        N, a, nqubits, nodes_peak, success,
                                        n_workers[w], n_gates, runtime, avg_gate_time, 
                                        plus_cacheput, plus_cached,
                                        gate_cacheput, gate_cached,
                                        cgate_cacheput, cgate_cached);
            }
        }
    }

    fclose(overview_file);

    return 0;
}

int main()
{
    #ifdef HAVE_PROFILER
        // TODO: automate file name depending on which circuit / parameteres
        if (profile_name != NULL) {
            printf("writing profile to %s\n", profile_name);
            ProfilerStart(profile_name);
        }
    #endif

    mkdir("benchmark_data", 0700);
    
    bench_random_circuit();
    //bench_grover();
    //bench_shor();
    //bench_supremacy();

    #ifdef HAVE_PROFILER
        if (profile_name != NULL) ProfilerStop();
    #endif

    return 0;
}
