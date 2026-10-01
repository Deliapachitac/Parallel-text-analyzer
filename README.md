# ⚡ Multi-Process Text Analyzer (`lexan`)

An asynchronous, multi-process text processing pipeline built in C for Linux environments. The system utilizes a **Divide & Conquer** architecture to perform high-performance lexical analysis, text normalization, filtering, and word frequency evaluation across massive text files.

---

## 🏛️ System Architecture

The application constructs a 3-tier hierarchical process tree comprising a **Root Coordinator**, multiple **Splitter** nodes, and **Builder** nodes. Inter-Process Communication (IPC) is achieved via **anonymous/named pipes** and asynchronous **POSIX signals** (`SIGUSR1`, `SIGUSR2`).

```
                               ┌──────────────────────────────────┐
                               │           Root Process           │
                               │            (lexan.c)             │
                               └──────┬────────────────────┬──────┘
                                      │                    │
              ┌───────────────────────┘                    └──────────────────────┐
              │ Spawns & Sends Line Offsets                                       │ Spawns Builders &
              ▼                                                                   │ Passes Pipes
  ┌──────────────────────┐                                                        ▼
  │   Splitter Process   │                                            ┌──────────────────────┐
  │     (splitter.c)     │                                            │    Builder Process   │
  └──────────┬───────────┘                                            │      (builder.c)     │
             │                                                        └──────────▲───────────┘
             │                                                                   │
             │  1. Cleans text & removes digits/punctuation                      │
             │  2. Filters out words via Exclusion Hash Table                    │
             │  3. Routes word w via Hash Function: h(w) % n ────────────────────┘
             │     (Streamed through IPC Pipes)
             │
             │
             │  Completion Signals:
             ├─────────────────────────────────────────► [ SIGUSR1 ] ─────────► To Root Process
             │                                                                        ▲
             │                                                                        │
             └─────────────────────────────────────────► [ SIGUSR2 ] ─────────────────┘
                                                          (Sent by Builders upon sending
                                                           final metrics & frequencies)
```


## 📁 Source Code & Component Breakdown

### 🔹 `lexan.c` (Root Coordinator)
* **CLI Parameter Handling**: Parses command-line flags (`-i`, `-I`, `-m`, `-t`, `-e`, `-o`) in arbitrary order[cite: 3, 4].
* **File Offset Mapping**: Counts total lines in the input document and constructs an index of line starting byte positions to divide workloads[cite: 2].
* **Splitter Process Setup**: Calculates line quotas per splitter, passes offsets via setup channels, builds argument matrices, and spawns `./splitter` processes using `execvp()`[cite: 2].
* **Builder Process Setup**: Initializes communication buffers and worker pipes, passes metadata to builder child processes, and launches `./builder` binaries[cite: 2].
* **Result Aggregation & Output**: Dynamically reallocates an array of word-frequency structs to gather output from all builders, sorts entries using `qsort()`, and logs execution statistics, signals received, runtimes, and the top-$k$ tokens to the output file[cite: 2, 3].

### 🔹 `splitter.c` (Token Extractor & Filter)
* **Exclusion List Initialization**: Constructs an internal hash set containing words loaded from the exclusion file for constant-time lookup[cite: 3].
* **Text Sanitization**: Reads designated line boundaries from the source text, strips punctuation, removes numeric digits, converts upper-case characters to lower-case, and filters out excluded terms[cite: 3].
* **Token Dispatching**: Computes target builder IDs using a custom distribution hash function (`hash_for_builders`) to route matching words to specific builder pipes[cite: 3].
* **Stream Termination**: Continuously transfers sanitized tokens until the pre-allocated line quota defined by the root process is fully processed[cite: 3].

### 🔹 `builder.c` (Frequency Accumulator)
* **Pipe Ingestion**: Reads byte chunks from incoming splitter channels and tokenizes streams using whitespace separators[cite: 2, 3].
* **Hash Table Storage**: Clones unique word tokens using `strdup()` and stores them inside a local hash table alongside their occurrence counters[cite: 3].
* **Data Transmission**: Traverses the accumulated hash structure and streams word length, string data, and frequency metrics back to the root process via dedicated pipes[cite: 3].
* **Final Signal & Metrics**: Appends a termination delimiter (`0`) followed by total CPU and real-time execution benchmarks measured prior to process exit[cite: 3, 5].

### 🔹 `Hash.c` / `Hash.h` (Dynamic Hash Table)
* **Data Structure Design**: Implements a generic Hash Table leveraging Separate Chaining for $O(1)$ average-time insertion and retrieval operations[cite: 3].
* **Exclusion Filtering Utility**: Stores ignored words without tracking occurrences, providing fast lookup speed compared to linear list scans[cite: 3].
* **Frequency Tracking Utility**: Manages word frequency counters by incrementing existing bucket structs upon duplicate token insertion rather than storing redundant entries[cite: 3].
* **API Functions**: Provides core operations (`create_hash_table`, `hash_string`, `hash_add`, `hash_find`), data accessors (`get_counter`, `hash_get_value`), and traversal utilities (`hash_first`, `hash_next`).




## 🔗 Inter-Process Communication & Data Flow

The program uses Unix pipes and POSIX signals for communication across its process hierarchy:

* **Setup & Task Assignment (Root ➔ Splitters)**
  * The root process computes starting byte offsets and line allocations for each splitter[cite: 2].
  * These execution parameters are passed directly via pipes prior to invoking `execvp()`[cite: 2].

* **Token Streaming (Splitters ➔ Builders)**
  * Each splitter opens dedicated pipes connected to all builder processes[cite: 3].
  * Cleaned tokens are passed through a routing hash function (`hash_for_builders`)[cite: 3].
  * The function deterministically maps each word to a specific builder, ensuring all instances of the same word land in the same builder's hash table[cite: 3].

* **Result Aggregation & Metrics (Builders ➔ Root)**
  * Builders process incoming token streams, accumulate word frequencies in their local hash tables, and stream the formatted output back to the root process via dedicated pipes[cite: 2, 3].
  * Each record transmits the token length, string, and counter[cite: 3].
  * A termination flag (`0`) signals the end of dataset streaming, followed immediately by process performance metrics (CPU and real-time execution duration)[cite: 3, 5].

* **Asynchronous Signal Handling (Workers ➔ Root)**
  * `SIGUSR1`: Sent by splitter processes to notify the root process upon completing their text scanning workload[cite: 3].
  * `SIGUSR2`: Sent by builder processes upon finalizing result transmission and execution timing delivery[cite: 3].
  * The root process tallies incoming signals to verify process synchronization before final data output[cite: 3].




## How to run 

### Compile

```bash
make
```

### Run 
The `TestFiles/` directory contains sample input files:
`ExclusionList.txt`
`GreatExpectations.txt`

You can run this sample through:

```bash
make run
```

To run manually:
```bash
./lexan -i <input file> -l <num of splitters> -m <num of builders> -t <top words> -e < exclusion list> -o <output file>
```