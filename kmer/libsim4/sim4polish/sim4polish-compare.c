#include "sim4polish.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

//
//  Routines for comparing sim4polish structures.
//
//  These routines assume that the iid's are consistent for the pair
//  of polishes.  In particular, that they are mapped to the same set
//  of genomic sequences.



//  Return false if not from the same EST/GEN pair, or mapped to
//  different strands, true otherwise.
//
int
s4p_compatable(sim4polish *A, sim4polish *B) {
  if ((A->estID != B->estID) ||
      (A->genID != B->genID) ||
      (A->matchOrientation != B->matchOrientation))
    return(0);
  else
    return(1);
}




//  Returns true if the two polishes are on about the same genomic
//  region
//
int
s4p_IsSameRegion(sim4polish *A, sim4polish *B, u32bit tolerance) {
  int Alo=0, Ahi=0;
  int Blo=0, Bhi=0;
  int Dlo=0, Dhi=0;

  if (A->numExons > 0) {
    Alo = A->genLo + A->exons[0].genFrom;
    Ahi = A->genLo + A->exons[A->numExons-1].genTo;
  }

  if (B->numExons > 0) {
    Blo = B->genLo + B->exons[0].genFrom;
    Bhi = B->genLo + B->exons[B->numExons-1].genTo;
  }

  Dlo = Blo - Alo;
  Dhi = Bhi - Ahi;

  if ((Dlo < -tolerance) || (Dlo > tolerance) ||
      (Dhi < -tolerance) || (Dhi > tolerance))
    return(0);

  return(1);
}



//  Returns true if the two polishes have the same number of exons,
//  and each exon is mapped to about the same genomic region.
//
int
s4p_IsSameExonModel(sim4polish *A, sim4polish *B, u32bit tolerance) {
  int Alo=0, Ahi=0;
  int Blo=0, Bhi=0;
  int Dlo=0, Dhi=0;

  if (A->numExons != B->numExons)
    return(0);

  for (int i=0; i<A->numExons; i++) {
    Alo = A->genLo + A->exons[i].genFrom;
    Ahi = A->genLo + A->exons[i].genTo;

    Blo = B->genLo + B->exons[i].genFrom;
    Bhi = B->genLo + B->exons[i].genTo;

    Dlo = Blo - Alo;
    Dhi = Bhi - Ahi;

    if ((Dlo < -tolerance) || (Dlo > tolerance) ||
        (Dhi < -tolerance) || (Dhi > tolerance)) {
      return(0);
    }
  }
}




void
s4p_compareExons_Overlap(sim4polish *A,
                         sim4polish *B,
                         int        *numSame,
                         int        *numMissing,
                         int        *numExtra) {
  int       i, j;
  int       Dlo=0, Dhi=0;
  int       al=0, ah=0, bl=0, bh=0;
  int      *foundA = 0L;
  int      *foundB = 0L;

  *numSame    = 0;
  *numMissing = 0;
  *numExtra   = 0;

  foundA = (int *)malloc(sizeof(int) * (A->numExons + B->numExons));
  foundB = foundA + A->numExons;

  if (errno) {
    fprintf(stderr, "s4p_compareExons()-- Can't allocate %u + %u int's for counting exons.\n%s\n", A->numExons, B->numExons, strerror(errno));
    exit(1);
  }

  for (i=0; i<A->numExons; i++)
    foundA[i] = 0;

  for (i=0; i<B->numExons; i++)
    foundB[i] = 0;

  //  If they overlap, declare a match
  //
  for (i=0; i<A->numExons; i++) {
    for (j=0; j<B->numExons; j++) {
      al = A->genLo + A->exons[i].genFrom;
      ah = A->genLo + A->exons[i].genTo;
      bl = B->genLo + B->exons[j].genFrom;
      bh = B->genLo + B->exons[j].genTo;

      if (((al <= bl) && (bl <= ah) && (ah <= bh)) ||
          ((bl <= al) && (al <= bh) && (bh <= ah)) ||
          ((al <= bl) && (bh <= ah)) ||
          ((bl <= al) && (ah <= bh))) {
        foundA[i]++;
        foundB[j]++;
        numSame++;
      }
    }
  }

  for (i=0; i<A->numExons; i++) {
    if (foundA[i] == 0)
      numExtra++;
    if (foundA[i] > 1)
      fprintf(stderr, "WARNING: Found exon %d %d times in A!\n", i, foundA[i]);
  }

  for (i=0; i<B->numExons; i++) {
    if (foundB[i] == 0)
      numMissing++;
    if (foundB[i] > 1)
      fprintf(stderr, "WARNING: Found exon %d %d times in B!\n", i, foundB[i]);
  }

  free(foundA);
}





void
s4p_compareExons_Ends(sim4polish *A,
                      sim4polish *B,
                      int         tolerance,
                      int        *numSame,
                      int        *numMissing,
                      int        *numExtra) {
  int       i, j;
  int       Dlo=0, Dhi=0;
  int       al=0, ah=0, bl=0, bh=0;
  int      *foundA = 0L;
  int      *foundB = 0L;

  *numSame    = 0;
  *numMissing = 0;
  *numExtra   = 0;

  foundA = (int *)malloc(sizeof(int) * (A->numExons + B->numExons));
  foundB = foundA + A->numExons;

  if (errno) {
    fprintf(stderr, "s4p_compareExons()-- Can't allocate %u + %u int's for counting exons.\n%s\n", A->numExons, B->numExons, strerror(errno));
    exit(1);
  }

  for (i=0; i<A->numExons; i++)
    foundA[i] = 0;

  for (i=0; i<B->numExons; i++)
    foundB[i] = 0;

  //  If they have similar end points, declare a match
  //
  for (i=0; i<A->numExons; i++) {
    for (j=0; j<B->numExons; j++) {
      Dlo = (int)(B->genLo + B->exons[j].genFrom) - (int)(A->genLo + A->exons[i].genFrom);
      Dhi = (int)(B->genLo + B->exons[j].genTo)   - (int)(A->genLo + A->exons[i].genTo);

      if ((Dlo > -tolerance) && (Dlo < tolerance) &&
          (Dhi > -tolerance) && (Dhi < tolerance)) {
        foundA[i]++;
        foundB[j]++;
        numSame++;
      }
    }
  }

  for (i=0; i<A->numExons; i++) {
    if (foundA[i] == 0)
      numExtra++;
    if (foundA[i] > 1)
      fprintf(stderr, "WARNING: Found exon %d %d times in A!\n", i, foundA[i]);
  }

  for (i=0; i<B->numExons; i++) {
    if (foundB[i] == 0)
      numMissing++;
    if (foundB[i] > 1)
      fprintf(stderr, "WARNING: Found exon %d %d times in B!\n", i, foundB[i]);
  }

  free(foundA);
}




