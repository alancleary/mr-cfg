# MR-CFG

This repository contains an implementation of the MR-CFG (Maximal Repeat Context-Free Grammar) construction algorithm presented in [1].
Specifically, the algorithm computes an MR-CFG (a straight-line grammar) for a string using its LCP-intervals.

The algorithm described in [1] is a generic algorithm that allows LCP-intervals to be computed from a variety of different data-structures so long as the intervals are iterated in shortest-LCP-value-first order.
This implementation uses a Compressed Suffix Array (CSA) to compute LCP-intervals using the algorithm of [2].
Both the "optimal" and "online" versions of the algorithm described in [1] are implemented.
Additionally, a third "fast" implementation based on compressed bitmaps has been implemented.

This implementation is not particularly fast or memory efficient.
And the generated grammar is not encoded or output.
Instead, statistics about the grammar are reported and then the input string is reconstructed from the grammar and output for validation.
Moreover, this implementation is intended to be used as a reference only.

All non-CLI code has been implemented as a header-only library so that it may easily be used and extended.


## Building

This project depends on [Succinct Data Structure Library 3.0](https://github.com/xxsds/sdsl-lite) and [Roaring Bitmap](https://github.com/RoaringBitmap/CRoaring).
You can install these on your system yourself before proceeding or let the build system install them in the repository's directory for you.

The project uses the [CMake](https://cmake.org/) meta-build system to generate build files specific to your environment.
Generate the build files as follows:
```bash
cmake -B build .
```
This will create a `build/` directory containing all the build files.
The first time you run this command it may take a while because it downloads dependencies from GitHub.

To actually build the code, run:
```bash
cmake --build build
```
This will generate an `MR-CFG` executable in the `build/` directory.
Similar to the previous command, the first time you run this command may take a while because it has to build the dependencies.
If you make changes to the code, you only have to run this command to recompile the code.


## Running

`MR-CFG` uses a command-line interface (CLI).
Its usage instructions are as follows:
```bash
Usage: MR-CFG {OPTIMAL|ONLINE|FAST} <FILE>
```
The first argument - `{OPTIMAL|ONLINE|FAST}` - specifies what interval stabbing algorithm to use.
`OPTIMAL` uses the theoretically optimal $\mathcal{O}(n)$ time algorithm, where $n$ is the length of the input text.
`ONLINE` uses the more space efficient but theoretically slower $\mathcal{O}(n\log{m})$ time algorithm based on binary search, where $m$ is the number of maximal repeats in the input text.
And `FAST` uses an algorithm based on compressed bitmaps that is relatively fast and space efficient.
The second argument - `<FILE>` - is a file containing text a straight-line grammar (SLG) will be built from.

After it computes the SLG, MR-CFG outputs the string the SLG produces to the standard error stream for validation.
This output can be captured in a file as follows:
```bash
./build/MR-CFG ONLINE my-input-file.txt 2> my-output-file.txt
```
Here `my-input-file.txt` is the input file and `my-output-file.txt` is the file that will capture the output to standard error, i.e. a copy of `my-input-file.txt` generated from the SLG.
These files can be compared using a tool like `diff`:
```bash
diff my-input-file.txt my-output-file.txt
```
Basic run-time info and statistics about the computed SLG will be output to the standard output.


## Results

The MR-CFG was first introduced in [3] as a CFG that can be derived from the Compact Directed Acyclic Word Graph (CDAWG) of a string.
In [1], we showed that its relationship to the CDAWG connects the MR-CFG to various string properties and data-structures.
These connections are not typical of SLGs and merit further investigation.
The primary contribution of [1] was to elucidate these connections and provide an algorithm for constructing the MR-CFG without the CDAWG, allowing it to be studied independently while further relating it to the existing stringology literature.
Additionally, these connections provide an opportunity for various string properties and data-structures to be better connected to _string attractors_ and dictionary compressors in general [4].

The actual run-time performance of our algorithm and compression power of the MR-CFG was not the purpose of our study, so experiments were not included in [1].
However, such experiments may provide insight into limitations of the MR-CFG and guide improvements to both the algorithm and the grammar.
Here we present experimental results on the data set from the MR-RePair manuscript [5].
We chose this data set because it contains a nice variety of data types and sizes.
MR-RePair is also a recent grammar-based compression algorithm based on maximal repeats, although it is not as strongly connected with other string properties and structures as MR-CFG.


|                                                           | **Data**                           | **MR-RePair**                       | **MR-CFG**                             |                                                                                           |
|:----------------------------------------------------------|:-----------------------------------|------------------------------------:|---------------------------------------:|:------------------------------------------------------------------------------------------|
| **Name**<br>**Number of Characters**<br>**Alphabet Size** | rand77.txt<br>65537<br>77          | 4492<br>46143<br>9<br>46152         | 123<br>54<br>65483<br>65537            | **Number of Rules**<br>**Non-Start Total Length**<br>**Start Length**<br>**Total Length** |
| **Name**<br>**Number of Characters**<br>**Alphabet Size** | world192.txt<br>2473401<br>94      | 48601<br>104060<br>212940<br>317000 | 96701<br>295586<br>693085<br>988671    | **Number of Rules**<br>**Non-Start Total Length**<br>**Start Length**<br>**Total Length** |
| **Name**<br>**Number of Characters**<br>**Alphabet Size** | bible.txt<br>4077776<br>63         | 72082<br>153266<br>386516<br>539782 | 216244<br>610301<br>1549683<br>2159984 | **Number of Rules**<br>**Non-Start Total Length**<br>**Start Length**<br>**Total Length** |
| **Name**<br>**Number of Characters**<br>**Alphabet Size** | E\_coli.txt<br>4638691<br>4        | 62363<br>129138<br>650174<br>779312 | 283701<br>680553<br>3725385<br>4405938 | **Number of Rules**<br>**Non-Start Total Length**<br>**Start Length**<br>**Total Length** |
| **Name**<br>**Number of Characters**<br>**Alphabet Size** | einstein.de.txt<br>92758442<br>117 | 21787<br>71709<br>12683<br>84392    | 26355<br>155697<br>29310<br>185007     | **Number of Rules**<br>**Non-Start Total Length**<br>**Start Length**<br>**Total Length** |
| **Name**<br>**Number of Characters**<br>**Alphabet Size** | fib41.txt<br>267914297<br>2        | 38<br>76<br>3<br>79                 | 41<br>74<br>24<br>98                   | **Number of Rules**<br>**Non-Start Total Length**<br>**Start Length**<br>**Total Length** |


## References

1. A. Cleary and J. Dood, "Constructing the CDAWG CFG using LCP-Intervals," _2023 Data Compression Conference (DCC)_. IEEE, Mar. 2023. doi: [10.1109/DCC55655.2023.00026](https://doi.org/10.1109/DCC55655.2023.00026)
2. T. Beller, K. Berger, and E. Ohlebusch, "Space-Efficient Computation of Maximal and Supermaximal Repeats in Genome Sequences," _String Processing and Information Retrieval_. Springer Berlin Heidelberg, pp. 99–110, 2012. doi: [10.1007/978-3-642-34109-0_11](https://doi.org/10.1007/978-3-642-34109-0_11).
3. D. Belazzougui and F. Cunial, "Representing the Suffix Tree with the CDAWG," _2019 Data Compression Conference (DCC)_. Schloss Dagstuhl - Leibniz-Zentrum fuer Informatik GmbH, Wadern/Saarbruecken, Germany, 2017, doi: [10.4230/LIPICS.CPM.2017.7](https://doi.org/10.4230/LIPICS.CPM.2017.7).
4. D. Kempa and N. Prezza, "At the roots of dictionary compression: string attractors," _Proceedings of the 50th Annual ACM SIGACT Symposium on Theory of Computing_. ACM, Jun. 20, 2018. doi: [10.1145/3188745.3188814](https://doi.org/10.1145/3188745.3188814).
5. I. Furuya, T. Takagi, Y. Nakashima, S. Inenaga, H. Bannai, and T. Kida, "MR-RePair: Grammar Compression Based on Maximal Repeats," _2019 Data Compression Conference (DCC)_. IEEE, Mar. 2019. doi: [10.1109/dcc.2019.00059](https://doi.org/10.1109/dcc.2019.00059).
