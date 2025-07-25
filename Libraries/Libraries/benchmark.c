#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <dirent.h>  // For opendir(), readdir(), closedir()
#include <sys/stat.h>

#include <ctype.h>

#define MAX_PATH_LENGTH 4096
struct match {
    int score;
    char* text;
    int text_index;
    long long time[2];
};

//hash helper
typedef struct {
    const char *key;
    int value;
} StringToInt;

void check_nonprintable(const char* str, const char* var_name) {
    int has_nonprintable = 0;

    printf("Checking %s:\n", var_name);
    for (size_t i = 0; i < strlen(str); i++) {
        unsigned char ch = (unsigned char)str[i];

        // Check if character is printable
        if (!isprint(ch)) {
            if (!has_nonprintable) {
                printf("Raw text before processing: \"%s\"\n", str);

                printf("WARNING: Non-printable characters found in %s!\n", var_name);
                has_nonprintable = 1;
            }
            printf("Index %zu: Char '%c' (HEX: %02X)\n", i, ch, ch);
        }
    }

    if (!has_nonprintable) {
        printf("%s is clean (all printable characters).\n", var_name);
    }
    printf("\n");
}

void generate_dna(char *sequence, int length) {
    const char bases[] = "ACGT";
    for (int i = 0; i < length; i++) {
        sequence[i] = bases[rand() % 4];
    }
    sequence[length] = '\0';
}

int count_sequences(const char* filename) {

    FILE *file = fopen(filename, "r");
    
    if (!file) {
        perror("Error opening file");
        return -1;
    }
    
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    int count = 0;

    while ((read = getline(&line, &len, file)) != -1) {
        if (line[0] == '>')
            count++;
    }

    free(line);
    fclose(file);
    return count;
}

int count_sequences_modded(const char* filename) {

    FILE *file = fopen(filename, "r");
    
    if (!file) {
        perror("Error opening file");
        return -1;
    }
    
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    int count = 0;

    while ((read = getline(&line, &len, file)) != -1) {
        if (strlen(line) > 0)
            count++;
    }

    //fprintf(stderr, "[Debug]: Sequence Counting Completed\n");
    free(line);
    fclose(file);
    return count;
}

char* extract_sequence(const char* filename, int target_line) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        perror("Error opening file");
        return NULL;
    }

    int curr_line = -1;
    char* line = NULL;
    char* sequence = NULL;
    size_t len = 0;
    ssize_t read;
 
    while ((read = getline(&line, &len, file)) != -1) {
        if (line[0] == '>') {
            curr_line++;
            continue;
        }
        if (curr_line == target_line) {
            if (read > 0 && line[read - 1] == '\n') {
                line[read - 1] = '\0';
            }

            char* seq_start = strchr(line, '|');
            if (seq_start) {
                seq_start = strchr(seq_start + 1, '|');
                if (seq_start) {
                    seq_start++;
                } else {
                    seq_start = line;
                }
            } else {
                seq_start = line;
            }

            sequence = strdup(seq_start);
            break;
        }
    }
    
    fclose(file);
    if (line) {
        free(line);
    }
    return sequence;
}

char* extract_sequence_modded(const char* filename, int target_line) {
    //fprintf(stderr, "[DEBUG]: Filename received (%s)\n", filename);
    FILE* file = fopen(filename, "r");
    if (!file) {
        perror("Error opening file");
        return NULL;
    }

    int curr_line = 0;
    char* line = NULL;
    char* sequence = NULL;
    size_t len = 0;
    ssize_t read;
 
    while ((read = getline(&line, &len, file)) != -1) {
        if (curr_line == target_line) {
            if (read > 0 && line[read - 1] == '\n') {
                line[read - 1] = '\0';
            }
            //no more parsing
            sequence = strdup(line);
            break;
        } else {
            curr_line++;
        }
    }
    
    // fprintf(stderr, "[DEBUG]: Sequences Extracted\n");
    fclose(file);
    if (line) {
        free(line);
    }
    return sequence;
}

void print_sequences(char* pattern, char* text, int pattern_number, int text_number) {
    printf("Pattern (Seq %d):\t%s\n", pattern_number, pattern);
    printf("Text (Seq %d):\t%s\n", text_number, text);
}

// int execute_wfa_basic(char* pattern, char* text, bool avx) {
//     const char *wfa_path = avx ? "./wfa_basic" : "/home/jupyter-administrator/WFA/WFA2-lib/examples/wfa_basic";

//     // Construct command
//     char command[1024];
//     snprintf(command, sizeof(command), "%s %s %s", wfa_path, pattern, text);

//     // Open process
//     FILE *fp = popen(command, "r");
//     if (!fp) {
//         perror("popen failed");
//         return -1;
//     }

//     // Read score from output
//     int score = -1;
//     if (fscanf(fp, "%d", &score) != 1) {
//         fprintf(stderr, "Failed to read score from wfa_basic.\n");
//     }

//     pclose(fp);
//     return score;
// }

void read_metrics(const char* file_name, int* score, long long* time_taken) {
    FILE *file = fopen(file_name, "r");
    if (!file) {
        perror("Error opening score.txt");
        return;
    }
    
    if (fscanf(file, "%d %lld", score, time_taken) != 2) {
        fprintf(stderr, "Failed to read score and time from score.txt\n");
    }

    fclose(file);
}
void read_metrics_modded(bool biwfa, const char* file_name, int* score, long long* time_taken, long* memory, char* CIGAR) {
    fprintf(stderr, "[DEBUG]: Read Metrics Modded\n");
    FILE *file = fopen(file_name, "r");
    char *line = NULL;
    size_t len = 0;
    if (!file) {
        perror("Error opening score.txt");
        return;
    }
    if (biwfa)
    {
        if (getline(&line, &len, file) != -1) {
            if (sscanf(line,"%d %lld %ld", score, time_taken, memory) != 3) {
                fprintf(stderr, "Failed to read score, time, and memory from score.txt\n");
            }
        }
        if (getline(&line, &len, file) != -1) {
            if (sscanf(line, "%s", CIGAR) != 1) {
                fprintf(stderr, "Failed to read CIGAR from score.txt\n");
            }
        }
        // if (fscanf(file, "%d %lld %ld", score, time_taken, memory) != 3) {
        //     fprintf(stderr, "Failed to read score, time, and memory from score.txt\n");
        // }
        // if (fscanf(file, "%s", CIGAR) != 1)
        // {
        //     fprintf(stderr, "Failed to read CIGAR from score.txt\n");
        // }
        free(line);
    }
    else {
        if (getline(&line, &len, file) != -1) {
            if (sscanf(line, "%lld %ld", time_taken, memory) != 2) {
                fprintf(stderr, "Failed to read time from score.txt\n");
            }
        }
        free(line);
        // if (fscanf(file, "%lld %ld", time_taken, memory) != 2) {
        //     fprintf(stderr, "Failed to read time from score.txt\n");
        // }
    }
    
    
    

    fclose(file);
}

void execute_wfa_basic(char* pattern, char* text, bool avx, int* score, long long* time_taken) {
    pid_t pid = fork();
    
    if (pid < 0) {
        perror("Fork failed");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        const char* wfa_path = avx ? "./Bi-WaveX/wfa_basic" : "./BiWFA_avx/examples/wfa_basic";
        char *args[] = {(char*)wfa_path, (char*)pattern, (char*)text, NULL};
        // printf("%s, %s, %s", args[0], args[1], args[2]);
        execvp(args[0], args);
        
        // If execlp fails, print error and exit child process
        perror("execvp failed");
        exit(EXIT_FAILURE);
    } else {
        // Parent process: Wait for child to complete
        int status;
        waitpid(pid, &status, 0);

        const char* file_name = avx ? "./scores/biwavex-score.txt" : "./scores/biwfa_avx-score.txt";
        read_metrics(file_name, score, time_taken);
        
        if (WIFEXITED(status)) {
            // printf("wfa_basic exited with status %d\n", WEXITSTATUS(status));
        } else {
            fprintf(stderr, "wfa_basic terminated abnormally\n");
        }
    }
}

void execute_wfa_basic_modded(int mode, int* score, long long* time_taken, long* mem, char* CIGAR) {
    pid_t pid = fork();
    if (pid < 0) {
        perror("Fork failed");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        const char* wfa_path = NULL;
        switch (mode)
        {
        case 1:
        //our avx
            wfa_path = "./Bi-WaveX/examples/wfa_basic";
            break;
        case 2:
        //avx from their side
            wfa_path = "./BiWFA_avx/examples/wfa_basic";
            break;
        case 3:
        //avxless
            wfa_path = "./BiWFA_og/examples/wfa_basic";
            break;
        case 4:
            wfa_path = "./ksw2/aligner";
            break;
        case 5: 
            wfa_path = "./Minimap2/minimap2";
            //-m NW /home/retraxius/inputs/input1.txt /home/retraxius/inputs/input2.txt 
            break;
        case 6:
            wfa_path = "./edlib";
            break;
        default:
            fprintf(stderr, "Failed to find a path, please choose between 1-6.\n");
            break;
        }
        if (mode == 5) {
            char *args[] = {(char*)wfa_path, "-ax", "asm5", "./inputs/input1.txt", "./inputs/input2.txt", NULL};
            execvp(args[0], args);
        }
        else if (mode == 6)
        {
            char *args[] = {(char*)wfa_path, "-m", "NW", "./inputs/input1.txt", "./inputs/input2.txt", NULL};
            execvp(args[0], args);
        }
        else {
            char *args[] = {(char*)wfa_path, NULL};
            execvp(args[0], args);
        }
        
        // printf("%s, %s, %s", args[0], args[1], args[2]);
        // printf ("Mode: %d\n", mode);
        
        // If execlp fails, print error and exit child process
        perror("execvp failed");
        exit(EXIT_FAILURE);
    } else {
        // Parent process: Wait for child to complete
        int status;
        waitpid(pid, &status, 0);
        const char* wfa_path = NULL;
        switch (mode)
        {
        case 1:
            wfa_path = "./scores/biwavex-score.txt";
            break;
        case 2:
            wfa_path = "./scores/biwfa_avx-score.txt";
            break;
        case 3:
            wfa_path = "./scores/biwfa_og-score.txt";
            break;
        case 4:
            wfa_path = "./scores/ksw2-score.txt";
            break;
        case 5: 
            wfa_path = "./scores/minimap2-score.txt";
            break;
        case 6:
            wfa_path = "./scores/edlib-score.txt";
            break;
        default:
            fprintf(stderr, "Failed to find a path, please choose between 1-6.\n");
            break;
        }
        // const char* file_name = avx ? "/home/retraxius/avx/score.txt" : "/home/retraxius/orig/score.txt";
        bool biwfa = mode < 5 ? true : false ;
        read_metrics_modded(biwfa, wfa_path, score, time_taken, mem, CIGAR);
        
        if (WIFEXITED(status)) {
            // printf("wfa_basic exited with status %d\n", WEXITSTATUS(status));
        } else {
            fprintf(stderr, "wfa_basic terminated abnormally\n");
        }
    }
}

void write_inputfile(char* sequence, const char* filename) {
    fprintf(stderr, "[DEBUG]: Writing Input File\n");
    int64_t count = 0;
    FILE* file = fopen(filename, "w");
    if (!file)
    {
        fprintf(stderr, "Error opening %s", filename);
    } else {
        fprintf(file, ">q{%ld}\n", count);
        fprintf(file, "%s\n", sequence);
        fclose(file);
        count++;
    }
}

void update_top_results(struct match best_matches[], struct match curr_match) {
    for (int i = 0; i < 5; i++) {
        if (curr_match.score > best_matches[i].score) {
            for (int k = 4; k > i; k--) {
                if (best_matches[k].text != NULL)
                    free(best_matches[k].text);
                
                best_matches[k] = best_matches[k-1];
                
                if (best_matches[k-1].text != NULL)
                    best_matches[k].text = strdup(best_matches[k-1].text);
            }
            if (best_matches[i].text != NULL)
                free(best_matches[i].text);
            
            best_matches[i] = curr_match;
            best_matches[i].text = strdup(curr_match.text);
            break;
        }
    }
}

int get_max_score(int score_1, int score_2) {
    return (score_1*(score_1>=score_2)) + (score_2*(score_2>score_1));
}

void write_output(const char* file_name, char* text, char* pattern, int text_index, int pattern_index, long long* time, int score) {
    const char* dir_name = "output";

    char full_path[256];
    snprintf(full_path, sizeof(full_path), "%s/%s.txt", dir_name, file_name);

    FILE* file = fopen(full_path, "a+");
    
    if (!file) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    fprintf(file, "Pattern\t[Sequence %04d] (Length %05ld): %s\n", pattern_index, strlen(pattern), pattern);
    fprintf(file, "Text\t[Sequence %04d] (Length %05ld): %s\n", text_index, strlen(text), text);
    fprintf(file, "Execution Time (Base)\t: %.4f ms\n", time[1]/1000000.0f);
    fprintf(file, "Execution Time (AVX)\t\t: %.4f ms\n", time[0]/1000000.0f);
    fprintf(file, "Score: %d\n\n", score);
    fprintf(file, "----------------------------\n\n");

    fclose(file);
}
void write_output_modded(const char* file_name, char* libname, char* text, char* pattern, int text_index, int pattern_index, long long* time, int score, long mem, char* CIGAR) {
    fprintf(stderr, "[DEBUG]: Write Output Modded\n");
    const char* dir_name = "output";

    char full_path[256];
    snprintf(full_path, sizeof(full_path), "%s/%s/%s", dir_name, libname, file_name);

    FILE* file = fopen(full_path, "a+");
    
    if (!file) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    fprintf(file, "Pattern\t[Sequence %04d] (Length %05ld): %s\n", pattern_index, strlen(pattern), pattern);
    fprintf(file, "Text\t[Sequence %04d] (Length %05ld): %s\n", text_index, strlen(text), text);
    fprintf(file, "Execution Time (%s_%s)\t: %.4f ms\n", libname, file_name, time[0]/1000000.0f);
    fprintf(file, "Peak Memory Consumed: %ld kb\n", mem);
    fprintf(file, "CIGAR: %s\n", CIGAR);
    fprintf(file, "Score: %d\n", score);
    fprintf(file, "----------------------------\n\n");

    fclose(file);
}

void clear_file(const char* file_name, char* libname) {
    const char* dir_name = "output";

    char full_path[256];
    snprintf(full_path, sizeof(full_path), "%s/%s/%s", dir_name, libname, file_name);

    FILE* file = fopen(full_path, "w");

    if (!file) {
        
        perror("Error opening file for clearing");
        exit(EXIT_FAILURE);
    }

    fclose(file);
}

void clear_file_modded(const char* file_name) {

    char full_path[256];
    snprintf(full_path, sizeof(full_path), "./scores/%s-score.txt", file_name);

    FILE* file = fopen(full_path, "w");

    if (!file) {
        perror("Error opening file for clearing");
        exit(EXIT_FAILURE);
    }

    fclose(file);
}

int getTextFilesFromDir(const char* testcase, char ***file_paths) {
    char directory[100] = "";
    snprintf(directory, sizeof(directory), "./test_cases/%s", testcase);

    size_t file_count = 0;
    struct dirent *entry;
    DIR *dp = opendir(directory);
    if (dp == NULL) {
        perror("opendir");
        return -1;
    }

    while ((entry = readdir(dp)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        char full_path[1024];
        snprintf(full_path, sizeof(full_path), "%s/%s", directory, entry->d_name);

        struct stat statbuf;
        if (stat(full_path, &statbuf) == -1) {
            perror("[stat]");
            continue;
        }

        if (S_ISREG(statbuf.st_mode)) {
            char **temp = realloc(*file_paths, sizeof(char *) * (file_count + 1));
            if (!temp) {
                perror("realloc");
                closedir(dp);
                return -1;
            }

            *file_paths = temp;
            (*file_paths)[file_count] = strdup(full_path);
            if (!(*file_paths)[file_count]) {
                perror("strdup");
                closedir(dp);
                return -1;
            }

            file_count++;
        }
    }

    closedir(dp);
    return file_count;
}

int getLibMode(const char *key) {
    StringToInt table[] = {
        {"biwavex", 1},
        {"biwfa_avx", 2},
        {"biwfa_og",   3},
        {"ksw2",   4},
        {"minimap2", 5},
        {"edlib", 6},
        {"nw", 7}
    };
    //fprintf(stderr, "[DEBUG]: getLibMode start.\n");
    for (int i = 0; i < sizeof(table)/sizeof(table[0]); i++) {
        if (strcmp(table[i].key, key) == 0)
            return table[i].value;
    }
    //fprintf(stderr, "[DEBUG]: getLibMode end.\n");
    return -1;  // or default
}

int main(int argc, char * const argv[]) {
    
    struct timespec start, end;
    struct match best_matches[5];
    struct match curr_match;

    // for (int i = 0; i < 5; i++) {
    //     best_matches[i].score = -2147483648;
    //     best_matches[i].text = NULL;
    //     best_matches[i].text_index = 0;
    //     best_matches[i].time[0] = 0;
    //     best_matches[i].time[1] = 0;
    // }

    int total_score[2] = {0, 0};
    int curr_score[2];
    long curr_mem[2];
    float average_score[2];
    float average_score_per_len[2];
    long long total_time[2];
    long long average_time[2];
    long long total_time_per_len[2];
    long long average_time_per_len[2];
    long long time_taken[2];

    // int num_len = 5;
    // // const char* file_names[] = {"150", "300", "500", "750", "1000"};
    // // const char* ref_file = "./test_cases/UNIPROT_DNA_ALL.fasta.txt";
    // //100K bp
    // const char* ref_file_l = "./test_cases/longSplits_100K/200K_idx_1.txt";
    // const char* ref_file_l_err10 = "./test_cases/longSplits_100K/200K_idx_1_err10.txt";
    // const char* ref_file_scale = "./test_cases/2.5Kbp_Scalability.txt";
    // const char* ref_file_l_err20 = "./test_cases/longSplits_100K/200K_idx_1_err20.txt";
    // //10k bp
    // const char* ref_med = "./test_cases/medSplits_10K/001_med.txt";
    // //50bp or 100 character length
    // const char* ref_short = "./test_cases/shortSplits_100/001_modded.txt";
    // char curr_file[100];
    // char target_file[100];

    // srand(time(NULL));
    // int total_text = count_sequences_modded(ref_med);
    // int num_text = 10;
    // if (total_text <= 0) {
    //     fprintf(stderr, "No sequences found in file.\n");
    //     return EXIT_FAILURE;
    // }
    //start of fixed version
    //declarations
    char library[100] = "biwavex";
    char testLen[100] = "10K";
    char **queryFiles = NULL;
    char **referenceFiles = NULL;
    char referenceDir[100] = "";
    char CIGAR[1000000];
    int libMode = 0;
    int option = 0;

    bool invalidArgs = false;
    struct dirent *entry;
    //main area
    fprintf(stderr, "Starting Benchmark\n");
    while ((option = getopt(argc, argv, "l:t:")) != -1)
    {
        switch (option) {
            case 'l': strcpy(library, optarg); fprintf(stderr, "Starting Lib Get\n"); break;
            case 't': strcpy(testLen, optarg); fprintf(stderr, "Starting TestLen Get\n"); break;
            default: invalidArgs = true;
        }
    }

    //decipher args
    fprintf(stderr, "[DEBUG]: Lib = {%s} & testLen = {%s}\n", library, testLen);
    libMode = getLibMode(library);
    if (libMode < 1)
    {
        perror("Parsing");
        return EXIT_FAILURE;
    }
    

    // //confirm if testcase dir exists
    snprintf(referenceDir, sizeof(referenceDir), "%s/references", testLen);
    int numQuery = getTextFilesFromDir(testLen, &queryFiles);
    int numReferences = getTextFilesFromDir(referenceDir, &referenceFiles);
    //main loop
    for (int i = 0; i < numQuery; i++)
    {
        fprintf(stderr, "Query File [%d]: {%s}\n", i, queryFiles[i]);
        char* querySource = queryFiles[i];
        for (int j = 0; j < numReferences; j++)
        {
            char* referenceSource = referenceFiles[j];
            char* minifile = strrchr(referenceSource, '/');
            minifile++;
            // minifile[strlen(minifile) - 4] = '\0'; //.txt suffix removal
            char tempfile[100];
            strcpy(tempfile, minifile);
            tempfile[strlen(tempfile) - 4] = '\0';
            fprintf(stderr, "Reference File [%d]: {%s}\n", j, referenceFiles[j]);
            //extract the total number of sequences
            clear_file_modded(library);
            clear_file(minifile, library);

            int querySeqs = count_sequences_modded(querySource);
            int referenceSeqs = count_sequences_modded(referenceSource);

            fprintf(stderr, "Query Seq Count [%d] | Reference Seq Count [%d] \n", querySeqs, referenceSeqs);

            if ((querySeqs <= 0 || referenceSeqs <= 0))
            {
                fprintf(stderr, "[ERROR]: One of the files are empty.");
                return EXIT_FAILURE;
            }
            
            for (int z = 0; z < referenceSeqs; z++)
            {
                char *query = extract_sequence_modded(querySource, z);

                if (!query)
                {
                    fprintf(stderr, "Failed to get query--- Skipping...\n");
                    return EXIT_FAILURE;
                }
                
                char *reference = extract_sequence_modded(referenceSource, z);
                fprintf(stderr, "RefSource: %s\n", referenceSource);
                if (!reference)
                {
                    fprintf(stderr, "Failed to get reference--- Skipping...\n");
                    return EXIT_FAILURE;
                }

                //Write Sequences into file for Reading
                write_inputfile(reference, "./inputs/input1.txt");
                write_inputfile(query, "./inputs/input2.txt");

                execute_wfa_basic_modded(libMode, &curr_score[0], &time_taken[0], &curr_mem[0], CIGAR);

                total_score[0] += curr_score[0];
                        
                curr_match.score = curr_score[0];
                curr_match.text = reference;
                curr_match.text_index = z;
                curr_match.time[0] = time_taken[0];
                //requires filenames to be cleaned
                //take filename, remove suffix

                write_output_modded(minifile, library, query, reference, z, z, time_taken, curr_score[0], curr_mem[0], CIGAR);
                free(query);
                free(reference);
                printf("Finished (%d/%d).\n", z+1, referenceSeqs);
            }
            
        }
        
    }
    

    // if (/*system("bash build.sh") ||*/ 1) {
    //     if (longCase)
    //     {
    //         //manual switch for scoring
    //         bool scored = true;
    //         //only interact with single file
    //         const char* mainfile = ref_file_scale;
    //         snprintf(curr_file, sizeof(curr_file), "%s", mainfile);
    //         char* minifile = strrchr(mainfile, '/');
    //         if (minifile)
    //         {
    //             minifile++;
    //         }
    //         //remove suffix
    //         char filename[100];
    //         strncpy(filename, minifile, sizeof(filename));
    //         filename[strlen(filename) - 4] = '\0'; //add null terminator
    //         //fprintf(stderr, "[DEBUG]: Filename = %s\n", filename);
    //         //manual for now
    //         clear_file_modded("biwfa_og");
    //         clear_file(filename);

    //         int total_seq = count_sequences_modded(curr_file);
    //         if (total_seq <= 0) {
    //             fprintf(stderr, "No sequences found in file. \n");
    //             return EXIT_FAILURE;
    //         }
    //         total_time_per_len[0] = 0;
    //         average_time_per_len[0] = 0;
    //         average_score_per_len[0] = 0;

    //         total_time_per_len[1] = 0;
    //         average_time_per_len[1] = 0;
    //         average_score_per_len[1] = 0;

    //         //2 loops
    //         //alternating loop
    //         if (alternating)
    //         {
    //             //fprintf(stderr, "[DEBUG]: Starting Alternating Loop\n");
    //             //manual for now
    //             for (int z = 0; z < total_seq; z++)
    //             {
    //                 //2 sequences per loop
    //                 int target_pattern = z;
    //                 char* pattern = extract_sequence_modded(curr_file, z);

    //                 if (!pattern) {
    //                     fprintf(stderr, "Failed to get pattern Skipping...\n");
    //                     continue;
    //                 }

    //                 z++;
    //                 int target_text = z;
    //                 char* text = extract_sequence_modded(curr_file, z);
    //                 if (!pattern) {
    //                     fprintf(stderr, "Failed to get text. Skipping...\n");
    //                     free(pattern);
    //                     continue;
    //                 }
    //                 fprintf(stderr, "Pattern length: %ld", strlen(pattern));
    //                 fprintf(stderr, "Text length: %ld", strlen(text));

    //                 total_time[0] = 0;
    //                 average_time[0] = 0;
    //                 time_taken[0] = 0;
    //                 total_score[0] = 0;

    //                 total_time[1] = 0;
    //                 average_time[1] = 0;
    //                 time_taken[1] = 0;
    //                 total_score[1] = 0;

    //                 for (int p = 0; p < 5; p++) {
    //                     best_matches[p].score = -2147483648;
    //                     best_matches[p].text = NULL;
    //                     best_matches[p].text_index = 0;
    //                     best_matches[p].time[0] = 0;
    //                     best_matches[p].time[1] = 0;
    //                 }
    //                 //write input files
    //                 fprintf(stderr, "Reading Start.");
    //                 write_inputfile(pattern, "./inputs/input1.txt");
    //                 write_inputfile(text, "./inputs/input2.txt");
    //                 fprintf(stderr, "Reading Success.");
    //                 // Test avx
    //                 // mode
    //                 execute_wfa_basic_modded(pattern, text, 1, &curr_score[0], &time_taken[0]);
    //                 total_time[0] += time_taken[0];
    //                 // printf("Iteration (A) %d: Time taken = %.6fs (%lldns)\t", j+1, time_taken[0]/1e9, time_taken[0]);

    //                 // Test original
    //                 // execute_wfa_basic_modded(pattern, text, 4, &curr_score[1], &time_taken[1]);
    //                 // total_time[1] += time_taken[1];
    //                 // printf("Iteration (O) %d: Time taken = %.6fs (%lldns)\n", j+1, time_taken[1]/1e9, time_taken[1]);
            
    //                 //2 versions: scored and no score
    //                 if (scored)
    //                 {
    //                     // if (curr_score[0] != curr_score[1]) {
    //                     //     printf("Error: Differing scores detected.\n");
    //                     //     print_sequences(pattern, text, target_pattern, target_text);
    //                     //     printf("AVX Score: %d\tOriginal Score: %d\n", curr_score[0], curr_score[1]);
    //                     //     printf("Exiting...\n");
    //                     //     free(pattern);
    //                     //     free(text);
    //                     //     return 1;
    //                     // }
    //                     total_score[0] += curr_score[0];
    //                     // total_score[1] += curr_score[1];
                        
    //                     curr_match.score = curr_score[0];
    //                     curr_match.text = text;
    //                     curr_match.text_index = target_text;
    //                     curr_match.time[0] = time_taken[0];
    //                     // curr_match.time[1] = time_taken[1];

    //                     update_top_results(best_matches, curr_match);

    //                     write_output_modded(filename, "avx", text, pattern, target_text, target_pattern, time_taken, curr_score[0]);
    //                 }
    //                 else 
    //                 {
    //                     write_output_modded(filename, "avx", text, pattern, target_text, target_pattern, time_taken, curr_score[0]);
    //                 }
    //                 free(text);
    //                 free(pattern);
    //                 printf("Finished (%d/%d).\n", z/2, total_seq/2);
    //             } 
    //         } else if (twofiles) {
    //             //fprintf(stderr, "[DEBUG]: Starting Alternating Loop\n");
    //             //for every sequence
    //             snprintf(target_file, sizeof(target_file), "%s", ref_file_l_err20);
    //             for (int z = 0; z < total_seq; z++)
    //             {
    //                 //2 sequences per loop
    //                 //main file is query source
    //                 int target_pattern = z;
    //                 char* pattern = extract_sequence_modded(curr_file, z);

    //                 if (!pattern) {
    //                     fprintf(stderr, "Failed to get pattern Skipping...\n");
    //                     continue;
    //                 }

    //                 int target_text = z;
    //                 char* text = extract_sequence_modded(target_file, z);
    //                 if (!pattern) {
    //                     fprintf(stderr, "Failed to get text. Skipping...\n");
    //                     free(pattern);
    //                     continue;
    //                 }
    //                 fprintf(stderr, "Pattern length: %ld ", strlen(pattern));
    //                 fprintf(stderr, "Text length: %ld\n", strlen(text));

    //                 total_time[0] = 0;
    //                 average_time[0] = 0;
    //                 time_taken[0] = 0;
    //                 total_score[0] = 0;

    //                 total_time[1] = 0;
    //                 average_time[1] = 0;
    //                 time_taken[1] = 0;
    //                 total_score[1] = 0;

    //                 for (int p = 0; p < 5; p++) {
    //                     best_matches[p].score = -2147483648;
    //                     best_matches[p].text = NULL;
    //                     best_matches[p].text_index = 0;
    //                     best_matches[p].time[0] = 0;
    //                     best_matches[p].time[1] = 0;
    //                 }
    //                 //write input files
    //                 write_inputfile(pattern, "./inputs/input1.txt");
    //                 write_inputfile(text, "./inputs/input2.txt");
    //                 // Test avx
    //                 // mode
    //                 execute_wfa_basic_modded(pattern, text, 3, &curr_score[0], &time_taken[0]);
    //                 total_time[0] += time_taken[0];
    //                 // printf("Iteration (A) %d: Time taken = %.6fs (%lldns)\t", j+1, time_taken[0]/1e9, time_taken[0]);

    //                 // Test original
    //                 // execute_wfa_basic_modded(pattern, text, 4, &curr_score[1], &time_taken[1]);
    //                 // total_time[1] += time_taken[1];
    //                 // printf("Iteration (O) %d: Time taken = %.6fs (%lldns)\n", j+1, time_taken[1]/1e9, time_taken[1]);
            
    //                 //2 versions: scored and no score
    //                 if (scored)
    //                 {
    //                     // if (curr_score[0] != curr_score[1]) {
    //                     //     printf("Error: Differing scores detected.\n");
    //                     //     print_sequences(pattern, text, target_pattern, target_text);
    //                     //     printf("AVX Score: %d\tOriginal Score: %d\n", curr_score[0], curr_score[1]);
    //                     //     printf("Exiting...\n");
    //                     //     free(pattern);
    //                     //     free(text);
    //                     //     return 1;
    //                     // }
    //                     total_score[0] += curr_score[0];
    //                     // total_score[1] += curr_score[1];
                        
    //                     curr_match.score = curr_score[0];
    //                     curr_match.text = text;
    //                     curr_match.text_index = target_text;
    //                     curr_match.time[0] = time_taken[0];
    //                     // curr_match.time[1] = time_taken[1];

    //                     update_top_results(best_matches, curr_match);
    //                     //NOTE: The 2nd arg is the folder name
    //                     write_output_modded(filename, "avxless_err20", text, pattern, target_text, target_pattern, time_taken, curr_score[0]);
    //                 }
    //                 else 
    //                 {
    //                     write_output_modded(filename, "avxless_err20", text, pattern, target_text, target_pattern, time_taken, curr_score[0]);
    //                 }
    //                 free(text);
    //                 free(pattern);
    //                 printf("Finished (%d/%d).\n", z/2, total_seq/2);
    //             } 
    //         } else  {
    //             //fprintf(stderr, "[DEBUG]: Starting Non-alternating Loop\n");
    //             //take first 5 sequences and compare each to all others in file
    //             for (int x = 0; x < num_seq; x++)
    //             {
    //                 //use num_seq as total number of patterns
    //                 int target_pattern = x;
    //                 char* pattern = extract_sequence_modded(curr_file, x);

    //                 if (!pattern) {
    //                     fprintf(stderr, "Failed to get pattern Skipping...\n");
    //                     continue;
    //                 }

    //                 write_inputfile(pattern, "./inputs/input1.txt");

    //                 for (int z = 0; z < total_seq; z++)
    //                 {
    //                     //target will include itself
    //                     int target_text = z;
    //                     char* text = extract_sequence_modded(curr_file, z);
    //                     if (!pattern) {
    //                         fprintf(stderr, "Failed to get text. Skipping...\n");
    //                         free(pattern);
    //                         continue;
    //                     }
    //                     // fprintf(stderr, "Pattern length: %ld", strlen(pattern));
    //                     // fprintf(stderr, "Text length: %ld", strlen(text));

    //                     total_time[0] = 0;
    //                     average_time[0] = 0;
    //                     time_taken[0] = 0;
    //                     total_score[0] = 0;

    //                     total_time[1] = 0;
    //                     average_time[1] = 0;
    //                     time_taken[1] = 0;
    //                     total_score[1] = 0;

    //                     for (int p = 0; p < 5; p++) {
    //                         best_matches[p].score = -2147483648;
    //                         best_matches[p].text = NULL;
    //                         best_matches[p].text_index = 0;
    //                         best_matches[p].time[0] = 0;
    //                         best_matches[p].time[1] = 0;
    //                     }
    //                     //write input files
    //                     write_inputfile(text, "./inputs/input2.txt");
    //                     // Test avx
    //                     execute_wfa_basic_modded(pattern, text, 4, &curr_score[0], &time_taken[0]);
    //                     total_time[0] += time_taken[0];
    //                     // printf("Iteration (A) %d: Time taken = %.6fs (%lldns)\t", j+1, time_taken[0]/1e9, time_taken[0]);

    //                     // Test original
    //                     // execute_wfa_basic_modded(pattern, text, 4, &curr_score[1], &time_taken[1]);
    //                     // total_time[1] += time_taken[1];
    //                     // printf("Iteration (O) %d: Time taken = %.6fs (%lldns)\n", j+1, time_taken[1]/1e9, time_taken[1]);
            
    //                     //2 versions: scored and no score
    //                     if (scored)
    //                     {
    //                         // if (curr_score[0] != curr_score[1]) {
    //                         //     printf("Error: Differing scores detected.\n");
    //                         //     print_sequences(pattern, text, target_pattern, target_text);
    //                         //     printf("AVX Score: %d\tOriginal Score: %d\n", curr_score[0], curr_score[1]);
    //                         //     printf("Exiting...\n");
    //                         //     free(pattern);
    //                         //     free(text);
    //                         //     return 1;
    //                         // }
    //                         total_score[0] += curr_score[0];
    //                         // total_score[1] += curr_score[1];
                        
    //                         curr_match.score = curr_score[0];
    //                         curr_match.text = text;
    //                         curr_match.text_index = target_text;
    //                         curr_match.time[0] = time_taken[0];
    //                         // curr_match.time[1] = time_taken[1]; 

    //                         update_top_results(best_matches, curr_match);

    //                         write_output_modded(filename, "ksw2", text, pattern, target_text, target_pattern, time_taken, curr_score[0]);
    //                     }
    //                     else 
    //                     {
    //                         write_output_modded(filename, "ksw2", text, pattern, target_text, target_pattern, time_taken, curr_score[0]);
    //                     }
    //                     free(text);
    //                     printf("Finished (%d/%d) of Pattern [%d].\n", z, total_seq, x);
    //                 }//
    //                 printf("Pattern %d finished.\n\n", x);
    //                 free(pattern);
    //             }
                
    //         }
    //         average_time[0] = total_time[0]/num_iters;
    //         total_time_per_len[0] += average_time[0];
    //         average_score[0] = (total_score[0]*1.0)/num_iters;

    //         average_time[1] = total_time[1]/num_iters;
    //         total_time_per_len[1] += average_time[1];
    //         average_score[1] = (total_score[1]*1.0)/num_iters;
    //         // printf("Average execution time: %.6fs (%lldns)\t%.6fs (%lldns)\n\n", average_time[0]/1e9, average_time[0], average_time[1]/1e9, average_time[1]);

    //         // char file_path[256];
    //         // snprintf(file_path, sizeof(file_path), "best_score/%s/%d", file_names[k], target_line);
    //         // clear_file(file_path);

    //     }
    //     else 
    //     {

    //         for (int k = 0; k < num_len; k++) {
            
    //         snprintf(curr_file, sizeof(curr_file), "./test_cases/%s.txt", file_names[k]);
    //         clear_file(file_names[k]);
            
    //         int total_seq = count_sequences(curr_file);
    //         if (total_seq <= 0) {
    //             fprintf(stderr, "No sequences found in file.\n");
    //             return EXIT_FAILURE;
    //         }

    //         total_time_per_len[0] = 0;
    //         average_time_per_len[0] = 0;
    //         average_score_per_len[0] = 0;

    //         total_time_per_len[1] = 0;
    //         average_time_per_len[1] = 0;
    //         average_score_per_len[1] = 0;
            
    //         for (int i = 0; i < num_seq; i++) {
    //             int pattern_length = 150;
    
    //             int text_length = (rand()%10000)+1;
    //             // char* text = (char*)malloc(text_length+1);
    //             // if (!text) {
    //             //     perror("Memory allocation failed.");
    //             //     exit(EXIT_FAILURE);
    //             // }
                
    //             // generate_dna(text, text_length);
                
    //             // int target_line = rand() % total_seq;
    //             int target_line = i;
    //             char* pattern = extract_sequence(curr_file, target_line);
            
    //             if (!pattern) {
    //                 fprintf(stderr, "Failed to get random pattern. Skipping...\n");
    //                 continue;
    //             }
                
    //             total_time[0] = 0;
    //             average_time[0] = 0;
    //             time_taken[0] = 0;
    //             total_score[0] = 0;

    //             total_time[1] = 0;
    //             average_time[1] = 0;
    //             time_taken[1] = 0;
    //             total_score[1] = 0;

    //             for (int i = 0; i < 5; i++) {
    //                 best_matches[i].score = -2147483648;
    //                 best_matches[i].text = NULL;
    //                 best_matches[i].text_index = 0;
    //                 best_matches[i].time[0] = 0;
    //                 best_matches[i].time[1] = 0;
    //             }

    //             for (int l = 0; l < num_text; l++) {
    //                 int target_text = l;
    //                 char* text = extract_sequence(ref_file, target_text);
    //                 if (!text) {
    //                     fprintf(stderr, "Failed to get random text. Skipping...\n");
    //                     free(pattern);
    //                     continue;
    //                 }

    //                 // print_sequences(pattern, text, target_line, text_length);
                    
    //                 for (int j = 0; j < num_iters; j++) {
    //                     // Test avx
    //                     execute_wfa_basic(pattern, text, true, &curr_score[0], &time_taken[0]);
    //                     total_time[0] += time_taken[0];
    //                     // printf("Iteration (A) %d: Time taken = %.6fs (%lldns)\t", j+1, time_taken[0]/1e9, time_taken[0]);

    //                     // Test original
    //                     execute_wfa_basic(pattern, text, false, &curr_score[1], &time_taken[1]);
    //                     total_time[1] += time_taken[1];
    //                     // printf("Iteration (O) %d: Time taken = %.6fs (%lldns)\n", j+1, time_taken[1]/1e9, time_taken[1]);

    //                     if (curr_score[0] != curr_score[1]) {
    //                         printf("Error: Differing scores detected.\n");
    //                         print_sequences(pattern, text, target_line, text_length);
    //                         printf("AVX Score: %d\tOriginal Score: %d\n", curr_score[0], curr_score[1]);
    //                         printf("Exiting...\n");
    //                         free(pattern);
    //                         free(text);
    //                         return 1;
    //                     }
    //                     total_score[0] += curr_score[0];
    //                     total_score[1] += curr_score[1];
                        
    //                     curr_match.score = curr_score[0];
    //                     curr_match.text = text;
    //                     curr_match.text_index = target_text;
    //                     curr_match.time[0] = time_taken[0];
    //                     curr_match.time[1] = time_taken[1];

    //                     update_top_results(best_matches, curr_match);

    //                     write_output(file_names[k], text, pattern, target_text, target_line, time_taken, curr_score[0]);
    //                 }

    //                 free(text);
    //                 if (!(l%100)) {
    //                     printf("%.2f%% finished (%d/%d).\n",
    //                             (((k*num_seq*num_text)+(i*num_text)+l*1.0)/(num_len*num_seq*num_text))*100.0,
    //                             (k*num_seq*num_text)+(i*num_text)+l, num_len*num_seq*num_text);
    //                 }
    //             }
                
    //             average_time[0] = total_time[0]/num_iters;
    //             total_time_per_len[0] += average_time[0];
    //             average_score[0] = (total_score[0]*1.0)/num_iters;

    //             average_time[1] = total_time[1]/num_iters;
    //             total_time_per_len[1] += average_time[1];
    //             average_score[1] = (total_score[1]*1.0)/num_iters;
    //             // printf("Average execution time: %.6fs (%lldns)\t%.6fs (%lldns)\n\n", average_time[0]/1e9, average_time[0], average_time[1]/1e9, average_time[1]);

    //             char file_path[256];
    //             snprintf(file_path, sizeof(file_path), "best_score/%s/%d", file_names[k], target_line);
    //             clear_file(file_path);
    //             for (int m = 0; m < 5; m++) {
    //                 write_output(file_path, best_matches[m].text, pattern, best_matches[m].text_index, 
    //                             target_line, best_matches[m].time, best_matches[m].score);
    //             }

    //             printf("Pattern %d finished.\n\n", i);
    //             free(pattern);
    //         }

    //         average_time_per_len[0] = total_time_per_len[0]/num_len;
    //         average_time_per_len[1] = total_time_per_len[1]/num_len;
    //         average_score_per_len[0] = average_score[0]/num_len;
    //         average_score_per_len[1] = average_score[1]/num_len;
    //         // printf("| Text Length: %s\t|\t%.6fs (%lldns)\t|\t%.6fs (%lldns)\t|\t%d\t|\n", file_names[k], average_time_per_len[0]/1e9, average_time_per_len[0],  average_time_per_len[1]/1e9, average_time_per_len[1], best_score);
    //         }
    //     }
    // }
    
    return 0;
}