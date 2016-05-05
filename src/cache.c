#include "cache.h"
#include "trace.h"
#include "queue.h"

int write_xactions = 0;
int read_xactions = 0;

/*
   Print help message to user
   */
void printHelp(const char * prog) {
  printf("%s [-h] | [-s <size>] [-w <ways>] [-l <line>] [-t <trace>]\n", prog);
  printf("options:\n");
  printf("-h: print out help text\n");
  printf("-s <cache size>: set the total size of the cache in KB\n");
  printf("-w <ways>: set the number of ways in each set\n");
  printf("-l <line size>: set the size of each cache line in bytes\n");
  printf("-t <trace>: use <trace> as the input file for memory traces\n");
  printf("-lru: use LRU replacement policy instead of FIFO\n");
}

// getAddressTag takes a 32-bit address and the number of bits that make up
//  the tag and returns the address's tag field.
uint32_t getAddressTag(uint32_t address, unsigned long tagSize) {
  return (address >> (0x20-tagSize)); 
}

// getAddressIndex takes a 32-bit address, the number of bits that make up 
//  the tag, and the number of bits that make up the index and returns the 
//  address's index field.
uint32_t getAddressIndex(uint32_t address, unsigned long tagSize,
    unsigned long indexSize) {
  return ((address << tagSize) >> (0x20 - indexSize));
}

// cacheLookup looks for the requested address in the cache. If it's in there, 
//  it returns 1. If it isn't in there, it returns 0.
int cacheLookup(int ways, uint32_t cache[][ways], int valid[][ways], uint32_t tagSize, uint32_t reqAddrTag, uint32_t reqAddrIndex) {

  uint32_t i;
  for (i = 0; i < ways; ++i) {
    if (!valid[reqAddrIndex][i]){
      // Once you run into an invalid block, you can stop the search 
      // early because sets fill up from left to right. 
      // ie, invalid i implies for all j > i, invalid j.
      return 0;
    }
    if (getAddressTag(cache[reqAddrIndex][i], tagSize) == reqAddrTag) {
      return 1;
    }
  }
  return 0;
}

// locateCacheBlock looks for a place to put the requested address. First, it 
//  does what cacheLookup does. If cacheLookup would return 1, locateCacheBlock
//  returns the address's location in the cache. If cacheLookup would return 0,
//  it determines which block to overwrite according to the replacement policy
//  and returns that block's coordinates.
uint32_t locateCacheBlock(int ways, uint32_t cache[][ways], int valid[][ways], uint32_t tagSize, Queue_t *replacementQueue, uint32_t reqAddrTag, uint32_t reqAddrIndex) {

  uint32_t i;

  for (i = 0; i < ways; ++i) {
    if (valid[reqAddrIndex][i]) {
      fprintf(stdout, "valid %d, %d, %d\n", reqAddrIndex, i, valid[reqAddrIndex][i]);
      if (getAddressTag(cache[reqAddrIndex][i], tagSize) == reqAddrTag) {
        // If the requested block is in the cache, return its 
        // location in the set.
        return i; 
      } else if (i == (ways - 1)) {
        // If the tag isn't in the set, replacement is necessary.
        // Choose a block to evict and then evict it
        i = queueFirstInstanceOfIndex(replacementQueue, reqAddrIndex);
        queueRemoveBlock(replacementQueue, i, reqAddrIndex);
        i = locateCacheBlock(ways, cache, valid, tagSize, replacementQueue, i, reqAddrIndex);
        cache[reqAddrIndex][i] = 0;
        return i; 
      }
    } else {
      // If an invalid cell is found in the set, mark it as valid and then 
      // return its location in the set.
      valid[reqAddrIndex][i] = 1;
      return i;
    }
  }
  fprintf(stderr, "locateCacheBlock likely messed up somewhere\n");
  return 0;
}

/*
   Main function. Feed to options to set the cache

Options:
-h : print out help message
-s : set L1 cache Size (KB)
-w : set L1 cache ways
-l : set L1 cache line size
*/
int main(int argc, char **argv)
{
  int i;
  uint32_t size = 32; //total size of L1$ (KB)
  uint32_t ways = 1; //# of ways in L1. Default to direct-mapped
  uint32_t line = 32; //line size (B)
  char repPolicy = 'f'; //replacement policy: f for FIFO, l for LRU
  // hit and miss counts
  int totalHits = 0;
  int totalMisses = 0;

  char * filename;

  //strings to compare
  const char helpString[] = "-h";
  const char sizeString[] = "-s";
  const char waysString[] = "-w";
  const char lineString[] = "-l";
  const char traceString[] = "-t";
  const char lruString[] = "-lru";

  if (argc == 1) {
    // No arguments passed, show help
    printHelp(argv[0]);
    return 1;
  }

  //parse command line
  for(i = 1; i < argc; i++)
  {
    //check for help
    if(!strcmp(helpString, argv[i]))
    {
      //print out help text and terminate
      printHelp(argv[0]);
      return 1; //return 1 for help termination
    }
    //check for size
    else if(!strcmp(sizeString, argv[i]))
    {
      //take next string and convert to int
      i++; //increment i so that it skips data string in the next loop iteration
      //check next string's first char. If not digit, fail
      if(isdigit(argv[i][0]))
      {
        size = atoi(argv[i]);
      }
      else
      {
        printf("Incorrect formatting of size value\n");
        return -1; //input failure
      }
    }
    //check for ways
    else if(!strcmp(waysString, argv[i]))
    {
      //take next string and convert to int
      i++; //increment i so that it skips data string in the next loop iteration
      //check next string's first char. If not digit, fail
      // TODO: check if log \in N
      if(isdigit(argv[i][0]))
      {
        ways = atoi(argv[i]);
      }
      else
      {
        printf("Incorrect formatting of ways value\n");
        return -1; //input failure
      }
    }
    //check for line size
    else if(!strcmp(lineString, argv[i]))
    {
      //take next string and convert to int
      i++; //increment i so that it skips data string in the next loop iteration
      //check next string's first char. If not digit, fail
      // TODO: check log \in N
      if(isdigit(argv[i][0]))
      {
        line = atoi(argv[i]);
      }
      else
      {
        printf("Incorrect formatting of line size value\n");
        return -1; //input failure
      }
    }
    else if (!strcmp(traceString, argv[i])) {
      filename = argv[++i];
    }
    else if (!strcmp(lruString, argv[i])) {
      repPolicy = 'l';
      return -1;
    }
    //unrecognized input
    else{
      printf("Unrecognized argument. Exiting.\n");
      return -1;
    }
  }

  /* TODO: Probably should intitalize the cache */

  unsigned long indexBits, offsetBits, tagBits;
  // before determining number of bits for things,
  // determine number of rows in array
  indexBits = (size*0x400) / (line * ways);
  uint32_t cache[indexBits][ways];
  uint32_t fullAssCache[1][indexBits*ways];
  memset(cache, 0, sizeof cache);
  memset(fullAssCache, 0, sizeof fullAssCache);
  
  // initialize valid, dirty bit arrays
  int valid[indexBits][ways];
  int faValid[1][indexBits*ways]; 
  int dirty[indexBits][ways];
  memset(valid, 0, sizeof valid);
  memset(faValid, 0, sizeof faValid);
  memset(dirty, 0, sizeof dirty);

  // determine bits breakup of address
  indexBits = log2l(indexBits);
  offsetBits = log2l(line);
  tagBits = 0x20 - (indexBits + offsetBits);

  // construct queues 
  Queue_t *seenBlocks;
  Queue_t *replacementQueue;
  Queue_t *faReplacementQueue;
  seenBlocks = queueConstructor();
  replacementQueue = queueConstructor();
  faReplacementQueue = queueConstructor();

  indexBits = pow(2, indexBits);
  printf("Ways: %u; Sets: %u; Line Size: %uB\n", (unsigned int) ways, (unsigned int) indexBits, line);
  indexBits = log2l(indexBits);
  printf("Tag: %d bits; Index: %d bits; Offset: %d bits\n", (int) tagBits, 
      (int) indexBits, (int) offsetBits);

  /* TODO: Now we read the trace file line by line */
  /* TODO: Now we simulate the cache */  
  /* TODO: Now we output the file */
  FILE *traceFP;
  traceFP = fopen(filename, "r");
  if (traceFP == NULL) {
    fprintf(stderr, "Invalid filename: %s\n", filename);
    return -1;
  }

  FILE *simFP;
  simFP = fopen(strcat(filename,".simulated"), "w");

  char requestType = 0;
  uint32_t requestAddress = 0;

  char *inString = malloc(sizeof(char)*16);

  while (fgets(inString, 16, traceFP) != NULL) {

    requestType = *inString;
    inString += sizeof(char) * 4;
    requestAddress = strtol(inString, NULL, 16);
    inString -= sizeof(char) * 4;

    uint32_t tag = getAddressTag(requestAddress, tagBits); 
    uint32_t index = getAddressIndex(requestAddress, tagBits, indexBits);

    fprintf(simFP, "%c 0x%X ", requestType, requestAddress);

    int seenFlag;
    seenFlag = queueContainsBlock(seenBlocks, tag, index);

    if (!seenFlag) {
      enqueue(seenBlocks, tag, index);
      fprintf(simFP, "compulsory\n");
    }

    int h, fah;

    switch (requestType) {
      case 'l': 
        // Read

        // Check for hit
        h = cacheLookup(ways, cache, valid, tagBits, tag, index);
        fah = cacheLookup((indexBits*ways), fullAssCache, faValid, (tagBits + indexBits), (tag<<(0x20-tagBits)|(index<<(offsetBits))), 0);
        if (h) {
          // Hit!
          totalHits++;
          fprintf(simFP, "hit\n");

          // Refresh requested thing in the queue
          if (repPolicy == 'l') {
            queueRemoveBlock(replacementQueue, tag, index);
            enqueue(replacementQueue, tag, index);
            if (fah) {
              queueRemoveBlock(faReplacementQueue, (tag<<(0x20-tagBits)|(index<<(offsetBits))), index);
              enqueue(faReplacementQueue, (tag<<(0x20-tagBits)|(index<<(offsetBits))), index);
            }
          }

          // return data.
          // done.
          break;
        } else {
          // Miss!
          totalMisses++;
          if (seenFlag) {
            if (fah) {
              fprintf(simFP, "conflict\n");
            } else {
              fprintf(simFP, "capacity\n");
            }
          }

          // Locate cache block to use
          uint32_t indexToRead, faIndexToRead;
          indexToRead = locateCacheBlock(ways, cache, valid, tagBits, replacementQueue, tag, index);
          if (!fah) {
            faIndexToRead = locateCacheBlock((indexBits * ways), fullAssCache, faValid, (tagBits + indexBits), faReplacementQueue, (tag<<(0x20-tagBits)|(index<<(offsetBits))), 0);
          }

          // Add requested thing to the queue
          enqueue(replacementQueue, tag, index);
          if (!fah) {
            enqueue(faReplacementQueue, (tag<<(0x20-tagBits)|(index<<(offsetBits))), 0);
          }

          if (dirty[index][indexToRead]) {
            // Writeback if dirty block
            write_xactions++;
          }

          // Read data from memory into cache block
          read_xactions++;

          // Mark cache block as not dirty
          dirty[index][indexToRead] = 0;

          // return data.
          // done.
          break;
        }
      case 's':
        // Write
        // Check for hit
        h = cacheLookup(ways, cache, valid, tagBits, tag, index);
        fah = cacheLookup((indexBits*ways), fullAssCache, faValid, (tagBits + indexBits), (tag<<(0x20-tagBits)|(index<<(offsetBits))), 0);
        if (h) {
          // Hit!
          totalHits++;
          fprintf(simFP, "hit\n");

          // Refresh requested thing in the queue
          if (repPolicy == 'l') {
            queueRemoveBlock(replacementQueue, tag, index);
            enqueue(replacementQueue, tag, index);
            if (fah) {
              queueRemoveBlock(faReplacementQueue, (tag<<(0x20-tagBits)|(index<<(offsetBits))), index);
              enqueue(faReplacementQueue, (tag<<(0x20-tagBits)|(index<<(offsetBits))), index);
            }
          }

          // Write new data to cache block
          uint32_t indexToWrite, faIndexToWrite;
          indexToWrite = locateCacheBlock(ways, cache, valid, tagBits, replacementQueue, tag, index);
          cache[index][indexToWrite] = requestAddress;

          // Add requested thing to queue
          enqueue(replacementQueue, tag, index);

          if (fah) {
            faIndexToWrite = locateCacheBlock((indexBits * ways), fullAssCache, faValid, (tagBits + indexBits), faReplacementQueue, (tag<<(0x20-tagBits)|(index<<(offsetBits))), 0);
            fullAssCache[0][faIndexToWrite] = requestAddress;
            enqueue(faReplacementQueue, (tag<<(0x20-tagBits)|(index<<(offsetBits))), 0);
          }

          // done.
          break;
        } else {
          // Miss!
          totalMisses++;
          if (seenFlag) {
            if (fah) {
              fprintf(simFP, "conflict\n");
            } else {
              fprintf(simFP, "capacity\n");
            }
          }

          // Locate cache block to use
          uint32_t indexToWrite, faIndexToWrite;
          indexToWrite = locateCacheBlock(ways, cache, valid, tagBits, replacementQueue, tag, index);

          // Add requested thing to the queue
          enqueue(replacementQueue, tag, index);

          if (!fah) {
            faIndexToWrite = locateCacheBlock((indexBits * ways), fullAssCache, faValid, (tagBits + indexBits), faReplacementQueue, (tag<<(0x20-tagBits)|(index<<(offsetBits))), 0);
            fullAssCache[0][faIndexToWrite] = requestAddress;
            enqueue(faReplacementQueue, (tag<<(0x20-tagBits)|(index<<(offsetBits))), 0);
          }

          if (dirty[index][indexToWrite]) {
            // Writeback if dirty block
            write_xactions++;
          }

          // Read data from memory into cache block
          read_xactions++;

          // Write new data to cache block
          cache[index][indexToWrite] = requestAddress;

          // Mark cache block as dirty
          dirty[index][indexToWrite] = 1;

          // done.
          break;
        }
          default:
          fprintf(stderr, "Error, unexpected accessType: %c\n", requestType);
          return -1;
    }

    fclose(traceFP);
    fclose(simFP);

    /* Print results */
    printf("Miss Rate: %8lf%%\n", ((double) totalMisses) / ((double)
          totalMisses + (double) totalHits) * 100.0);
    printf("Read Transactions: %d\n", read_xactions);
    printf("Write Transactions: %d\n", write_xactions);


    /* TODO: Cleanup */
  }
}
