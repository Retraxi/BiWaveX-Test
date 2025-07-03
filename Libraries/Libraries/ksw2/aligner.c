#include <string.h>
#include <stdio.h>
#include <time.h>
#include <sys/resource.h>
#include "ksw2.h"

int align(const char *tseq, const char *qseq, int sc_mch, int sc_mis, int gapo, int gape)
{
	int i, a = sc_mch, b = sc_mis < 0? sc_mis : -sc_mis; // a>0 and b<0
	int8_t mat[25] = { a,b,b,b,0, b,a,b,b,0, b,b,a,b,0, b,b,b,a,0, 0,0,0,0,0 };
	int tl = strlen(tseq), ql = strlen(qseq);
	uint8_t *ts, *qs, c[256];
	ksw_extz_t ez;

	memset(&ez, 0, sizeof(ksw_extz_t));
	memset(c, 4, 256);
	c['A'] = c['a'] = 0; c['C'] = c['c'] = 1;
	c['G'] = c['g'] = 2; c['T'] = c['t'] = 3; // build the encoding table
	ts = (uint8_t*)malloc(tl);
	qs = (uint8_t*)malloc(ql);
	for (i = 0; i < tl; ++i) ts[i] = c[(uint8_t)tseq[i]]; // encode to 0/1/2/3
	for (i = 0; i < ql; ++i) qs[i] = c[(uint8_t)qseq[i]];
    //            1  2   3   4   5   6   7    8     9    10  11  12 13  14
	//ksw_extz(0, ql, qs, tl, ts, 5, mat, gapo, gape, -1, -1, 0, &ez);
  fprintf(stderr, "Encoding completed");
  ksw_extz2_sse(0, ql, qs, tl, ts, 5, mat, gapo, gape, -1, -1, 0, 0, &ez);
  fprintf(stderr, "Function Call completed");
	
  for (i = 0; i < ez.n_cigar; ++i) // print CIGAR
		printf("%d%c", ez.cigar[i]>>4, "MID"[ez.cigar[i]&0xf]);
	putchar('\n');

  fprintf(stderr, "Alignment Score: %d \n", ez.score);

  free(ez.cigar); free(ts); free(qs);

  return ez.score;
}

int main(int argc, char *argv[])
{
    //read from file instead
  char* pattern = NULL;
  char * text = NULL;
  //read from file
  FILE* file = fopen("./inputs/input1.txt", "r");
  if (!file)
  {
    perror("Error opening file");
    return EXIT_FAILURE;
  }
  char* line = NULL;
  size_t len = 0;
  ssize_t read;

  while ((read = getline(&line, &len, file)) != -1)
  {
    if(read > 0 && line[read-1] == '\n') {
      line[read-1] = '\0';
    }
    if(line[0] != '>') {
      pattern = strdup(line);
    }
  }

  fclose(file);
  if (line)
  {
    free(line);
  }

  //read from file 2
  file = fopen("./inputs/input2.txt", "r");
  if (!file)
  {
    perror("Error opening file");
    return EXIT_FAILURE;
  }
  line = NULL;
  len = 0;
  ssize_t read2;

  while ((read2 = getline(&line, &len, file)) != -1)
  {
    if(read2 > 0 && line[read2-1] == '\n') {
      line[read2-1] = '\0';
    }
    if(line[0] != '>') {
      text = strdup(line);
    }
  }

  fclose(file);
  if (line)
  {
    free(line);
  }
    // fprintf(stderr, "Pattern: %s", pattern);
    // fprintf(stderr, "Text: %s", text);
    //query, text
    int score;
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    score = align(pattern, text, 0, -4, -6, -2);
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    long long elapsed_time = (end.tv_sec - start.tv_sec) * 1e9 + (end.tv_nsec - start.tv_nsec);
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    long peak_memory_kb = usage.ru_maxrss;

    FILE *file2 = fopen("./scores/ksw2-score.txt", "w");
    if (file2 == NULL) {
      perror("fopen");
      exit(EXIT_FAILURE);
    }
    //int32_t tempscore = score;
    //fprintf(file, "%" PRId32 "\n", tempscore);
    fprintf(file2, "%d %lld %ld\n", score, elapsed_time, peak_memory_kb);
    fclose(file2);


	return 0;
}