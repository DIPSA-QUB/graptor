// This code is part of the project "Ligra: A Lightweight Graph Processing
// Framework for Shared Memory", presented at Principles and Practice of
// Parallel Programming, 2013.
// Copyright (c) 2013 Julian Shun and Guy Blelloch
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights (to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#ifndef LIGRA_IO_H
#define LIGRA_IO_H

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <cassert>
#include <unistd.h>

#include "parallel.h"
// #include "quickSort.h"
using namespace std;

template <class E, class BinPred, class intT>
void quickSort(E* A, intT n, BinPred f) {
    std::sort( &A[0], &A[n], f );
}

typedef pair<intE,intE> intPair;
typedef pair<intE, pair<intE,intE> > intTriple;

template <class E>
struct pairFirstCmp
{
    bool operator() (pair<intE,E> a, pair<intE,E> b)
    {
        // We need to sort such that the destinations are also
        // sorted for best memory locality.
        return a.first < b.first
               || ( a.first == b.first && a.second < b.second );
    }
};

// A structure that keeps a sequence of strings all allocated from
// the same block of memory
struct words
{
    intT n; // total number of characters
    char* Chars;  // array storing all strings
    intT m; // number of substrings
    char** Strings; // pointers to strings (all should be null terminated)
    words() {}
    words(char* C, intT nn, char** S, intT mm)
        : n(nn), Chars(C), m(mm), Strings(S) {}
    void del()
    {
        delete [] Chars;
        delete [] Strings;
    }
};

inline bool isSpace(char c)
{
    switch (c)
    {
    case '\r':
    case '\t':
    case '\n':
    case 0:
    case ' ' :
        return true;
    default :
        return false;
    }
}

inline _seq<char> readStringFromFile(char *fileName)
{
    ifstream file (fileName, ios::in | ios::binary | ios::ate);
    if (!file.is_open())
    {
        std::cout << "Unable to open file: " << fileName << std::endl;
        abort();
    }
    intT end = file.tellg();
    file.seekg (0, ios::beg);
    intT n = end - file.tellg();
    char* bytes = new char [n+1];
    file.read (bytes,n);
    file.close();
    return _seq<char>(bytes,n);
}

// parallel code for converting a string to words
inline words stringToWords(char *Str, intT n)
{
    {
        parallel_for (intT i=0; i < n; i++)
        if (isSpace(Str[i])) Str[i] = 0;
    }

    // mark start of words
    bool *FL = new bool [n];
    FL[0] = Str[0];
    {
        parallel_for (intT i=1; i < n; i++) FL[i] = Str[i] && !Str[i-1];
    }

    // offset for each start of word
    _seq<intT> Off = sequence::packIndex<intT>(FL, n);
    intT m = Off.n;
    intT *offsets = Off.A;

    // pointer to each start of word
    char **SA = new char* [m];
    {
        parallel_for (intT j=0; j < m; j++) SA[j] = Str+offsets[j];
    }

    delete [] offsets;
    delete [] FL;
    return words(Str,n,SA,m);
}

template <class vertex>
wholeGraph<vertex> readGraphFromFile(char* fname, bool isSymmetric)
{
    _seq<char> S = readStringFromFile(fname);
    words W = stringToWords(S.A, S.n);
#ifndef WEIGHTED
    if (W.Strings[0] != (string) "AdjacencyGraph")
#else
    if (W.Strings[0] != (string) "WeightedAdjacencyGraph")
#endif
    {
        cout << "Bad input file" << endl;
        abort();
    }

    intT len = W.m -1;
    intT n = atol(W.Strings[1]);
    intT m = atol(W.Strings[2]);
#ifndef WEIGHTED
    if (len != n + m + 2)
#else
    if (len != n + 2*m + 2)
#endif
    {
        cout << "Bad input file (n,m)" << endl;
        abort();
    }

    // Change in constructor: it now allocates all data structures in-place
    // based on n and m.
    wholeGraph<vertex> WG(n, m, isSymmetric);
    //graph<vertex> G( n, m , isSymmetric, -1);

    intT* offsets = new intT [n];
    intE* edges = WG.allocatedInplace;
    {
        parallel_for(intT i=0; i < n; i++) offsets[i] = atol(W.Strings[i + 3]);
    }
    {
        parallel_for(intT i=0; i<m; i++)
        {
#ifndef WEIGHTED
            edges[i] = atol(W.Strings[i+n+3]);
#else
            edges[2*i] = atol(W.Strings[i+n+3]);
            edges[2*i+1] = atol(W.Strings[i+n+m+3]);
#endif
        }
    }
    W.del(); // to deal with performance bug in malloc
    vertex * V = WG.V;
    {
        parallel_for (intT i=0; i < n; i++)
        {
            uintT o = offsets[i];
            uintT l = ((i == n-1) ? m : offsets[i+1])-offsets[i];
            V[i].setOutDegree(l);
#ifndef WEIGHTED
            V[i].setOutNeighbors(edges+o);
#else
            V[i].setOutNeighbors(edges+2*o);
#endif
        }
    }

    if(!isSymmetric)
    {
        // Optimization on graph loading: if the transpose graph (reverse
        // edges) exists, then load this from file. Else, do the transpose
        // on the fly. The graph loads faster if the transpose exists.
        int fnl = strlen(fname);
        char t_fname[fnl+3];
        strcpy( t_fname, fname );
        strcpy( &t_fname[fnl], "_t" );
        struct stat buffer;
        if( stat( t_fname, &buffer ) == 0)
        {
            _seq<char> S = readStringFromFile(t_fname);
            words W = stringToWords(S.A, S.n);
#ifndef WEIGHTED
            if (W.Strings[0] != (string) "AdjacencyGraph")
#else
            if (W.Strings[0] != (string) "WeightedAdjacencyGraph")
#endif
            {
                cout << "Bad input file (header)" << endl;
                abort();
            }


            if( len != W.m-1 || atol(W.Strings[1]) != n
                    || atol(W.Strings[2]) !=m )
            {
                cout << "Transpose not matching input file (n, m)" << endl;
                abort();
            }

            intE* t_edges = WG.inEdges;

            {
                parallel_for(intT i=0; i < n; i++) offsets[i] = atol(W.Strings[i + 3]);
            }
            {
                parallel_for(intT i=0; i<m; i++)
                {
#ifndef WEIGHTED
                    t_edges[i] = atol(W.Strings[i+n+3]);
#else
                    t_edges[2*i] = atol(W.Strings[i+n+3]);
                    t_edges[2*i+1] = atol(W.Strings[i+n+m+3]);
#endif
                }
            }

            W.del(); // to deal with performance bug in malloc
            {
                parallel_for (intT i=0; i < n; i++)
                {
                    uintT o = offsets[i];
                    uintT l = ((i == n-1) ? m : offsets[i+1])-offsets[i];
                    V[i].setInDegree(l);
#ifndef WEIGHTED
                    V[i].setInNeighbors(t_edges+o);
#else
                    V[i].setInNeighbors(t_edges+2*o);
#endif
                }
            }

            delete [] offsets;
            return WG;
        }
        else
        {
            std::cerr << "Warning: transposed file '" << t_fname
                      << "' does not exist for asymmetric graph\n";
            std::cerr << "Warning: using the transposed file speeds up "
                      << "graph loading\n";

            intT* tOffsets = new intT [n];
            {
                parallel_for(intT i=0; i<n; i++) tOffsets[i] = INT_T_MAX;
            }

            intE* inEdges = WG.inEdges;

#ifndef WEIGHTED
            intPair* temp = new intPair [m];
#else
            intTriple* temp = new intTriple [m];
#endif
            {
                parallel_for(intT i=0; i<n; i++)
                {
                    uintT o = offsets[i];
                    for(intT j=0; j<V[i].getOutDegree(); j++)
                    {
#ifndef WEIGHTED
                        temp[o+j] = make_pair(V[i].getOutNeighbor(j),i);
#else
                        temp[o+j] = make_pair(V[i].getOutNeighbor(j),make_pair(i,V[i].getOutWeight(j)));
#endif
                    }
                }
            }
            delete [] offsets;

#ifndef WEIGHTED
            quickSort(temp,m,pairFirstCmp<intE>());
#else
            quickSort(temp,m,pairFirstCmp<intPair>());
#endif

            tOffsets[0] = 0;
    //        tOffsets[temp[0].first] = 0;
#ifndef WEIGHTED
            inEdges[0] = temp[0].second;
#else
            inEdges[0] = temp[0].second.first;
            inEdges[1] = temp[0].second.second;
#endif
            {
                parallel_for(intT i=1; i<m; i++)
                {
#ifndef WEIGHTED
                    inEdges[i] = temp[i].second;
#else
                    inEdges[2*i] = temp[i].second.first;
                    inEdges[2*i+1] = temp[i].second.second;
#endif
                    if(temp[i].first != temp[i-1].first)
                    {
                        tOffsets[temp[i].first] = i;
                    }
                }
            }

            delete [] temp;

            //fill in offsets of degree 0 vertices by taking closest non-zero
            //offset to the right
            sequence::scanIBack(tOffsets,tOffsets,n,minF<intT>(),(intT)m);

            {
                parallel_for(intT i=0; i<n; i++)
                {
                    uintT o = tOffsets[i];
                    uintT l = ((i == n-1) ? m : tOffsets[i+1])-tOffsets[i];
                    V[i].setInDegree(l);
#ifndef WEIGHTED
                    V[i].setInNeighbors(inEdges+o);
#else
                    V[i].setInNeighbors(inEdges+2*o);
#endif
                }
            }

            delete [] tOffsets;
            return WG;//graph<vertex>(v,n,m,edges,inEdges);
        }
    }
    else
    {
        delete [] offsets;
        return WG;//graph<vertex>(v,n,m,edges);
    }
}


template <class vertex>
wholeGraph<vertex> readGraphFromGalois(char* fname, bool isSymmetric)
{
    int fd;

    fd = open( fname, O_RDONLY );
    if( (fd = open( fname, O_RDONLY )) < 0 )
    {
        std::cerr << "Error in Galois input file: cannot open '"
                  << fname << "'\n";
        abort();
    }
    intT len = lseek( fd, 0, SEEK_END );
    if( len == (intT)-1 )
    {
        std::cerr << "Error in Galois input file in the sym: seek failed\n";
        abort();
    }

    // Could add MAP_POPULATE to preload all pages into memory
    // Could add MAP_HUGETLB to use huge pages
    const char * data = (const char *)mmap( 0, len, PROT_READ,
                                            MAP_SHARED, fd, 0 );
    if( data == (const char *)-1 )
    {
        std::cerr << "Cannot mmap input graph file\n";
        abort();
    }

    // 15/06/2016 Hans
    // The header is a sequence of 4 64-bit integers.
    // The best way to read 64-bit integers is to use a data type
    // that is guaranteed to take 64-bit. int64_t and uint64_t are
    // two portable integer data types defined in C for this purpose.
    //uint32 for 4 bytes, uint64 for 8 bytes
    // intT * header = (intT*)&data[0];
    size_t * header = (size_t*)&data[0];
    if( header[0] != 1 && header[0] != 2 )
    {
        std::cerr << "Error in Galois input file: version ("
                  << std::hex << header[0] << std::dec << ") != 1 or 2\n";
        abort();
    }

    intT n = (intT)header[2];
    intT m = (intT)header[3];
    bool wgh = ((intT)header[1]) == 4;
    size_t header_size = sizeof(header[0]) * 4;
    if( header[0] == 1 ) {
    	assert( sizeof(intT) == 8 && sizeof(intE) == 4
	    && "Version 1 input file" );
    } else if( header[0] == 2 ) {
        size_t * more_header = (size_t*)&data[header_size];
	header_size += sizeof(header[0]) * 4;
	std::cerr << "Version 2 binary file has sizeof(VID)=" << more_header[0]
	          << " and sizeof(EID)=" << more_header[1] << "\n";
	std::cerr << "Compiled with sizeof(intT)=" << sizeof(intT)
		  << " and sizeof(intE)=" << sizeof(intE) << "\n";
	std::cerr << "Compiled with sizeof(VID)=" << sizeof(VID)
		  << " and sizeof(EID)=" << sizeof(EID) << "\n";
    	assert( sizeof(intE) == more_header[0]
	    && sizeof(intT) == more_header[1]
	    && "Version 2 input file" );
    }
    //cerr<<"m="<<m<<" and n="<<n<<"wgh="<<wgh<<endl;
#ifndef WEIGHTED
    // We can ignore the weights in the file if we don't need them, but
    // we cannot continue without weights if we expect them.
    assert( !wgh );
#endif

    wholeGraph<vertex> G(n, m, isSymmetric);
    //graph<vertex> G( n, m , isSymmetric, -1);

    //uint32 for 4 bytes, uint64 for 8 bytes
    //size_t * offsets = (size_t*)(data + sizeof( header[0] ) * 4);
    intT * offsets = (intT*)(data + header_size);
    //uint32 for 4 bytes, uint64 for 8 bytes
    //uint32_t * edest
    intE * edest
        = (intE*)(data + header_size + sizeof( offsets[0] ) * n);
#ifdef WEIGHTED
    //uint32 for 4 bytes, uint64 for 8 bytes
    //uint32_t * ewght
    intE * ewght
        = (intE*)(data + header_size
                  + sizeof( offsets[0] ) * n
                  + sizeof( edest[0] ) * ( m + (m&1) ));
#endif

    intE * edges = G.allocatedInplace;

#ifndef WEIGHTED
    assert( header_size + sizeof(intT) * n + sizeof(intE) * m == len );

    parallel_for(intE i=0; i<m; i++)
	edges[i] = (intE)edest[i];
#else
    // Copy even though because we could re-use mmap data because we need
    // to avoid disk accesses.
    parallel_for(intE i=0; i<m; i++)
    {
        edges[2*i] = (intE)edest[i];
        edges[2*i+1] = (intE)ewght[i];
    }
#endif

//   vertex * v = G.V;

    parallel_for(intT i=0; i<n; i++)
    {
        uintT o = i == 0 ? 0 : offsets[i-1];
        //uintT o = offsets[i];
        //uintT l = ((i==n-1) ? m : offsets[i+1])-offsets[i];
        uintT l = offsets[i] - o;
        G.V[i].setOutDegree(l);
#ifndef WEIGHTED
        //G.V[i].setOutNeighbors(&edges[o]);
        G.V[i].setOutNeighbors(edges+o);
#else
        //G.V[i].setOutNeighbors(&edges[2*o]);
        G.V[i].setOutNeighbors(edges+2*o);
#endif
    }

    if( !isSymmetric )
    {
        int fnl = strlen(fname);
        char t_fname[fnl+3];
        strcpy( t_fname, fname );
        strcpy( &t_fname[fnl], "_t" );
        struct stat buffer;
        if( stat( t_fname, &buffer ) == 0)
        {
    	    munmap( (void*)data, len );
    	    close( fd );
            fd = open(t_fname, O_RDONLY);
            if( fd < 0 )
            {
                std::cerr << "Error in Galois t-input file: cannot open '"
                          << t_fname << "'\n";
                abort();
            }
            intT len = lseek( fd, 0, SEEK_END );
            if( len == (intT)-1 )
            {
                std::cerr << "Error in Galois input file: seek failed\n";
                abort();
            }

            // Could add MAP_POPULATE to preload all pages into memory
            // Could add MAP_HUGETLB to use huge pages
            const char * tdata
                = (const char *)mmap( 0, len, PROT_READ,
                                      MAP_SHARED, fd, 0 );
            if( tdata == (const char *)-1 )
            {
                std::cerr << "Cannot mmap input graph file\n";
                abort();
            }

            //uint32 for 4 bytes, uint64 for 8 bytes
            intT * theader = (intT*)&tdata[0];

            if( theader[0] != 1 )
            {
                std::cerr << "Error in Galois input file: version ("
                          << std::hex << theader[0] << std::dec << ") != 1\n";
                abort();
            }
            if( n != (intT)theader[2] || m != (intT)theader[3] )
            {
                std::cerr << "Mismatch in Galois input files on n/m\n";
                abort();
            }

            intT * t_offsets = (intT*)(tdata + sizeof( theader[0] ) * 4);
            //uint32 for 4 bytes, uint64 for 8 bytes
            //intE * edest
            intE * tedest
                = (intE*)(tdata + sizeof( theader[0] ) * 4
                              + sizeof( t_offsets[0] ) * n);
#ifdef WEIGHTED
            //uint32 for 4 bytes, uint64 for 8 bytes
            intE * tewght
                = (intE*)(tdata + sizeof( theader[0] ) * 4
                              + sizeof( t_offsets[0] ) * n
                              + sizeof( tedest[0] ) * ( m + (m&1) ));
#endif

            intE * t_edges = G.inEdges;

#ifndef WEIGHTED
            parallel_for(intT i=0; i<m; i++)
            t_edges[i] = (intE) tedest[i];
#else
            // Copy even though we could re-use mmap data because we need
            // to avoid disk accesses.
            parallel_for(intT i=0; i<m; i++)
            {
                t_edges[2*i] = (intE)tedest[i];
                t_edges[2*i+1] = (intE)tewght[i];
            }
#endif
            parallel_for(intT i=0; i<n; i++)
            {
                uintT o = i == 0 ? 0 : t_offsets[i-1];
                //uintT o = t_offsets[i];
                //uintT l = ((i==n-1) ? m : t_offsets[i+1])-t_offsets[i];
                uintT l = t_offsets[i] - o;
                G.V[i].setInDegree(l);
#ifndef WEIGHTED
                G.V[i].setInNeighbors(t_edges+o);
#else
                G.V[i].setInNeighbors(t_edges+2*o);
#endif
            }
            munmap( (void*)tdata, len );
            close(fd);
            return G;
        }
        else
        {
            std::cerr << "Warning: transposed file '" << t_fname
                      << "' does not exist for asymmetric graph\n";
            std::cerr << "Warning: using the transposed file speeds up "
                      << "graph loading\n";
            intT* tOffsets = new intT [n];
            {
                parallel_for(intT i=0; i<n; i++) tOffsets[i] = INT_T_MAX;
            }
            intE* inEdges = G.inEdges;

#ifndef WEIGHTED
            intPair* temp = new intPair [m];
#else
            intTriple* temp = new intTriple [m];
#endif
            {
                parallel_for(intT i=0; i<n; i++)
                {
                    uintT o = i == 0 ? 0 : offsets[i-1];
                    for(intT j=0; j<G.V[i].getOutDegree(); j++)
                    {
#ifndef WEIGHTED
                        temp[o+j] = make_pair(G.V[i].getOutNeighbor(j),i);
#else
                        temp[o+j] = make_pair(G.V[i].getOutNeighbor(j),make_pair(i,G.V[i].getOutWeight(j)));
#endif
                    }
                }
            }
#ifndef WEIGHTED
            quickSort(temp,m,pairFirstCmp<intE>());
#else
            quickSort(temp,m,pairFirstCmp<intPair>());
#endif
            //tOffsets[0] = 0;
            tOffsets[temp[0].first] = 0;
#ifndef WEIGHTED
            inEdges[0] = temp[0].second;
#else
            inEdges[0] = temp[0].second.first;
            inEdges[1] = temp[0].second.second;
#endif
            parallel_for(intT i=1; i<m; i++)
            {
#ifndef WEIGHTED
                inEdges[i] = temp[i].second;
#else
                inEdges[2*i] = temp[i].second.first;
                inEdges[2*i+1] = temp[i].second.second;
#endif
                if(temp[i].first != temp[i-1].first)
                {
                    tOffsets[temp[i].first] = i;
                }
            }

            delete [] temp;

            //fill in offsets of degree 0 vertices by taking closest non-zero
            //offset to the right
            sequence::scanIBack(tOffsets,tOffsets,n,minF<intT>(),(intT)m);

            {
                parallel_for(intT i=0; i<n; i++)
                {
                    uintT o = tOffsets[i];
                    uintT l = ((i == n-1) ? m : tOffsets[i+1])-tOffsets[i];
                    G.V[i].setInDegree(l);
#ifndef WEIGHTED
                    G.V[i].setInNeighbors(inEdges+o);
#else
                    G.V[i].setInNeighbors(inEdges+2*o);
#endif
                }
            }

            munmap( (void*)data, len );
            close (fd);
            delete [] tOffsets;
            return G;
        }
    }
    else
    {
        munmap( (void*)data, len );
        close (fd);
        return G;
    }
}

template <class vertex>
wholeGraph<vertex> readGraph(char* iFile, bool symmetric, bool binary)
{
    // if(binary) return readGraphFromBinary<vertex>(iFile,symmetric);
    if(binary) return readGraphFromGalois<vertex>(iFile,symmetric);
    else return readGraphFromFile<vertex>(iFile,symmetric);
}


#endif // LIGRA_IO_H
