/*
  Copyright (c) 2008 Stefan Kurtz <kurtz@zbh.uni-hamburg.de>
  Copyright (c) 2008 Center for Bioinformatics, University of Hamburg

  Permission to use, copy, modify, and distribute this software for any
  purpose with or without fee is hereby granted, provided that the above
  copyright notice and this permission notice appear in all copies.

  THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

#include "core/str.h"
#include "core/unused_api.h"
#include "core/arraydef.h"
#include "esa-seqread.h"
#include "alphadef.h"
#include "verbose-def.h"
#include "spacedef.h"
#include "format64.h"
#include "echoseq.pr"
#include "esa-mmsearch-def.h"
#include "tyr-mkindex.h"

struct Dfsinfo /* information stored for each node of the lcp interval tree */
{
  Seqpos leftmostleaf,
         rightmostleaf,
         suftabrightmostleaf,
         lcptabrightmostleafplus1;
};

typedef int (*Processoccurrencecount)(unsigned long,Seqpos,void *,GtError *);

typedef struct _listSeqpos
{
  Seqpos position;
  struct _listSeqpos *nextptr;
} ListSeqpos;

typedef struct
{
  unsigned long occcount;
  ListSeqpos *positionlist;
} Countwithpositions;

DECLAREARRAYSTRUCT(Countwithpositions);

struct Dfsstate /* global information */
{
  Seqpos searchlength,
         totallength;
  unsigned long minocc,
                maxocc;
  const Encodedsequence *encseq;
  Readmode readmode;
  const Alphabet *alpha;
  Processoccurrencecount processoccurrencecount;
  ArrayCountwithpositions occdistribution;
  FILE *merindexfpout;
  bool moveforward;
  Encodedsequencescanstate *esrspace;
  bool performtest;
  const Seqpos *suftab; /* only necessary for performtest */
  Uchar *currentmer;    /* only necessary for performtest */
};

#include "esa-dfs.h"

static uint64_t bruteforcecountnumofmers(const Dfsstate *state)
{
  Seqpos idx;
  uint64_t numofmers = 0;

  for (idx=0; idx <= state->totallength - state->searchlength; idx++)
  {
    if (!containsspecial(state->encseq,
                         state->moveforward,
                         state->esrspace,
                         idx,
                         state->searchlength))
    {
      numofmers++;
    }
  }
  return numofmers;
}

static void checknumofmers(const Dfsstate *state,
                           uint64_t dnumofmers)
{
  uint64_t bfnumofmers = bruteforcecountnumofmers(state);

  if (dnumofmers != bfnumofmers)
  {
    fprintf(stderr,"numofmers(distribution) = " Formatuint64_t " != "
                   Formatuint64_t,
                   PRINTuint64_tcast(dnumofmers),
                   PRINTuint64_tcast(bfnumofmers));
    exit(EXIT_FAILURE);
  }
}

static void checknumberofoccurrences(const Dfsstate *dfsstate,
                                     unsigned long countocc,
                                     Seqpos position)
{
  MMsearchiterator *mmsi;
  Seqpos idx, bfcount;

  for (idx = 0; idx < dfsstate->searchlength; idx++)
  {
    dfsstate->currentmer[idx] = getencodedchar(dfsstate->encseq,position+idx,
                                               dfsstate->readmode);
  }
  mmsi = newmmsearchiterator(dfsstate->encseq,
                             dfsstate->suftab,
                             0,
                             dfsstate->totallength,
                             0,
                             dfsstate->readmode,
                             dfsstate->currentmer,
                             (unsigned long) dfsstate->searchlength);
  bfcount = countmmsearchiterator(mmsi);
  if (bfcount != (Seqpos) countocc)
  {
    fprintf(stderr,"bfcount = " FormatSeqpos " != %lu = countocc\n",
                   PRINTSeqposcast(bfcount),
                   countocc);
    exit(EXIT_FAILURE);
  }
  freemmsearchiterator(&mmsi);
}

static /*@null@*/ ListSeqpos *insertListSeqpos(ListSeqpos *liststart,
                                               Seqpos position)
{
  ListSeqpos *newnode;

  ALLOCASSIGNSPACE(newnode,NULL,ListSeqpos,1);
  newnode->position = position;
  newnode->nextptr = liststart;
  return newnode;
}

static void wrapListSeqpos(ListSeqpos *node)
{
  ListSeqpos *tmpnext;

  if (node != NULL)
  {
    while (true)
    {
      if (node->nextptr == NULL)
      {
        FREESPACE(node);
        break;
      }
      tmpnext = node->nextptr;
      FREESPACE(node);
      node = tmpnext;
    }
  }
}

static void showListSeqpos(const Encodedsequence *encseq,
                           const Alphabet *alpha,
                           Seqpos searchlength,
                           const ListSeqpos *node)
{
  const ListSeqpos *tmp;

  for (tmp = node; tmp != NULL; tmp = tmp->nextptr)
  {
    fprintfencseq(stdout,alpha,encseq,tmp->position,searchlength);
    (void) putchar((int) '\n');
  }
}

static bool decideifocc(const Dfsstate *state,unsigned long countocc)
{
  if (state->minocc > 0)
  {
    if (state->maxocc > 0)
    {
      if (countocc >= state->minocc && countocc <= state->maxocc)
      {
        return true;
      }
    } else
    {
      if (countocc >= state->minocc)
      {
        return true;
      }
    }
  } else
  {
    if (state->maxocc > 0)
    {
      if (countocc <= state->maxocc)
      {
        return true;
      }
    }
  }
  return false;
}

static uint64_t addupdistribution(const ArrayCountwithpositions *distribution)
{
  unsigned long idx;
  uint64_t addcount = 0;

  for (idx=0; idx < distribution->nextfreeCountwithpositions; idx++)
  {
    if (distribution->spaceCountwithpositions[idx].occcount > 0)
    {
      addcount += (uint64_t)
                  (idx * distribution->spaceCountwithpositions[idx].occcount);
    }
  }
  return addcount;
}

static void showmerdistribution(const Dfsstate *state)
{
  unsigned long countocc;

  for (countocc = 0;
      countocc < state->occdistribution.nextfreeCountwithpositions;
      countocc++)
  {
    if (state->occdistribution.spaceCountwithpositions[countocc].occcount > 0)
    {
      printf("%lu %lu\n",countocc,
                         state->occdistribution.
                                spaceCountwithpositions[countocc].occcount);
      if (decideifocc(state,countocc))
      {
        showListSeqpos(state->encseq,
                       state->alpha,
                       state->searchlength,
                       state->occdistribution.spaceCountwithpositions[countocc].
                                              positionlist);
        wrapListSeqpos(state->occdistribution.spaceCountwithpositions[countocc].
                                              positionlist);
      }
    }
  }
}

static void showfinalstatistics(const Dfsstate *state,
                                const GtStr *inputindex,
                                Verboseinfo *verboseinfo)
{
  uint64_t dnumofmers = addupdistribution(&state->occdistribution);

  if (state->performtest)
  {
    checknumofmers(state,dnumofmers);
  }
  showverbose(verboseinfo,
              "the following output refers to the set of all sequences");
  showverbose(verboseinfo,
              "represented by the index \"%s\"",gt_str_get(inputindex));
  showverbose(verboseinfo,
              "number of %lu-mers in the sequences not containing a "
              "wildcard: " Formatuint64_t,
              (unsigned long) state->searchlength,
              PRINTuint64_tcast(dnumofmers));
  showverbose(verboseinfo,
              "show the distribution of the number of occurrences of %lu-mers",
               (unsigned long) state->searchlength);
  showverbose(verboseinfo,"not containing a wildcard as rows of the form "
              "i d where");
  showverbose(verboseinfo,
              "d is the number of events that a %lu-mer occurs exactly i times",
              (unsigned long) state->searchlength);
  showmerdistribution(state);
}

static void incrementdistribcounts(ArrayCountwithpositions *occdistribution,
                                   unsigned long countocc,unsigned long value)
{
  if (countocc >= occdistribution->allocatedCountwithpositions)
  {
    const Seqpos addamount = (Seqpos) 128;
    unsigned long idx;

    ALLOCASSIGNSPACE(occdistribution->spaceCountwithpositions,
                     occdistribution->spaceCountwithpositions,
                     Countwithpositions,countocc+addamount);
    for (idx=occdistribution->allocatedCountwithpositions;
         idx<countocc+addamount; idx++)
    {
      occdistribution->spaceCountwithpositions[idx].occcount = 0;
      occdistribution->spaceCountwithpositions[idx].positionlist = NULL;
    }
    occdistribution->allocatedCountwithpositions = countocc+addamount;
  }
  if (countocc + 1 > occdistribution->nextfreeCountwithpositions)
  {
    occdistribution->nextfreeCountwithpositions = countocc+1;
  }
  occdistribution->spaceCountwithpositions[countocc].occcount += value;
}

static int adddistpos2distribution(unsigned long countocc,
                                   Seqpos position,
                                   void *adddistposinfo,
                                   GT_UNUSED GtError *err)
{
  Dfsstate *state = (Dfsstate *) adddistposinfo;

  incrementdistribcounts(&state->occdistribution,countocc,1UL);
  if (decideifocc(state,countocc))
  {
    state->occdistribution.spaceCountwithpositions[countocc].positionlist
      = insertListSeqpos(state->occdistribution.
                              spaceCountwithpositions[countocc].positionlist,
                       position);
  }
  if (state->performtest)
  {
    checknumberofoccurrences(state,countocc,position);
  }
  return 0;
}

static Dfsinfo *allocateDfsinfo(GT_UNUSED Dfsstate *state)
{
  Dfsinfo *dfsinfo;

  ALLOCASSIGNSPACE(dfsinfo,NULL,Dfsinfo,1);
  return dfsinfo;
}

static void freeDfsinfo(Dfsinfo *dfsinfo, GT_UNUSED Dfsstate *state)
{
  FREESPACE(dfsinfo);
}

/*
static bool containsspecial2(const Encodedsequence *encseq,
                     GT_UNUSED bool moveforward,
                     GT_UNUSED Encodedsequencescanstate *esrspace,
                     Seqpos startpos,
                     Seqpos len)
{
  Seqpos pos;
  bool result = false, result2;

  for (pos=startpos; pos<startpos+len; pos++)
  {
    if (ISSPECIAL(getencodedchar(encseq,pos,Forwardmode)))
    {
      result = true;
      break;
    }
  }
  result2 = containsspecial(encseq,
                            moveforward,
                            esrspace,
                            startpos,len);
  if ((result && !result2) || (!result && result2))
  {
    fprintf(stderr,"pos = %lu, len = %lu: result = %s != %s = result2\n",
                    (unsigned long) startpos,
                    (unsigned long) len,
                    result ? "true" : "false",
                    result2 ? "true" : "false");
    exit(EXIT_FAILURE);
  }
  return result;
}
*/

static int processleafedge(GT_UNUSED bool firstsucc,
                           Seqpos fatherdepth,
                           GT_UNUSED Dfsinfo *father,
                           Seqpos leafnumber,
                           Dfsstate *state,
                           GT_UNUSED GtError *err)
{
  gt_error_check(err);
  if (fatherdepth < state->searchlength &&
      leafnumber + state->searchlength <= state->totallength &&
      !containsspecial(state->encseq,
                       state->moveforward,
                       state->esrspace,
                       leafnumber + fatherdepth,
                       state->searchlength - fatherdepth))
  {
    if (state->processoccurrencecount(1UL,leafnumber,state,err) != 0)
    {
      return -1;
    }
  }
  return 0;
}

static int processcompletenode(Seqpos nodeptrdepth,
                               Dfsinfo *nodeptr,
                               Seqpos nodeptrminusonedepth,
                               Dfsstate *state,
                               GT_UNUSED GtError *err)
{
  gt_error_check(err);
  if (state->searchlength <= nodeptrdepth)
  {
    Seqpos fatherdepth;

    fatherdepth = nodeptr->lcptabrightmostleafplus1;
    if (fatherdepth < nodeptrminusonedepth)
    {
      fatherdepth = nodeptrminusonedepth;
    }
    if (fatherdepth < state->searchlength)
    {
      if (state->processoccurrencecount((unsigned long)
                                        (nodeptr->rightmostleaf -
                                         nodeptr->leftmostleaf + 1),
                                        nodeptr->suftabrightmostleaf,
                                        state,
                                        err) != 0)
      {
        return -1;
      }
    }
  }
  return 0;
}

static void assignleftmostleaf(Dfsinfo *dfsinfo,Seqpos leftmostleaf,
                               GT_UNUSED Dfsstate *dfsstate)
{
  dfsinfo->leftmostleaf = leftmostleaf;
}

static void assignrightmostleaf(Dfsinfo *dfsinfo,Seqpos currentindex,
                                Seqpos previoussuffix,Seqpos currentlcp,
                                GT_UNUSED Dfsstate *dfsstate)
{
  dfsinfo->rightmostleaf = currentindex;
  dfsinfo->suftabrightmostleaf = previoussuffix;
  dfsinfo->lcptabrightmostleafplus1 = currentlcp;
}

static int enumeratelcpintervals(const GtStr *str_inputindex,
                                 Sequentialsuffixarrayreader *ssar,
                                 const GtStr *str_storeindex,
                                 unsigned long searchlength,
                                 unsigned long minocc,
                                 unsigned long maxocc,
                                 bool performtest,
                                 Verboseinfo *verboseinfo,
                                 GtError *err)
{
  Dfsstate state;
  bool haserr = false;
  char *merindexoutfilename = NULL;

  gt_error_check(err);
  INITARRAY(&state.occdistribution,Countwithpositions);
  state.esrspace = newEncodedsequencescanstate();
  state.searchlength = (Seqpos) searchlength;
  state.alpha = alphabetSequentialsuffixarrayreader(ssar);
  state.readmode = readmodeSequentialsuffixarrayreader(ssar);
  state.minocc = minocc;
  state.maxocc = maxocc;
  state.moveforward = ISDIRREVERSE(state.readmode) ? false : true;
  state.encseq = encseqSequentialsuffixarrayreader(ssar);
  state.totallength = getencseqtotallength(state.encseq);
  state.performtest = performtest;
  if (performtest)
  {
    ALLOCASSIGNSPACE(state.currentmer,NULL,Uchar,state.searchlength);
    state.suftab = suftabSequentialsuffixarrayreader(ssar);
  } else
  {
    state.currentmer = NULL;
    state.suftab = NULL;
  }
  if (state.searchlength > state.totallength)
  {
    gt_error_set(err,"searchlength " FormatSeqpos " > " FormatSeqpos
                     " = totallength not allowed",
                 PRINTSeqposcast(state.searchlength),
                 PRINTSeqposcast(state.totallength));
    haserr = true;
  } else
  {
    if (gt_str_length(str_storeindex) == 0)
    {
      state.merindexfpout = NULL;
      state.processoccurrencecount = adddistpos2distribution;
    } else
    {
      state.merindexfpout = NULL;
      assert(false);
    }
    if (depthfirstesa(ssar,
                      allocateDfsinfo,
                      freeDfsinfo,
                      processleafedge,
                      NULL,
                      processcompletenode,
                      assignleftmostleaf,
                      assignrightmostleaf,
                      &state,
                      verboseinfo,
                      err) != 0)
    {
      haserr = true;
    }
    if (gt_str_length(str_storeindex) == 0)
    {
      showfinalstatistics(&state,str_inputindex,verboseinfo);
    } else
    {
      assert(false);
    }
  }
  FREESPACE(merindexoutfilename);
  FREEARRAY(&state.occdistribution,Countwithpositions);
  FREESPACE(state.currentmer);
  freeEncodedsequencescanstate(&state.esrspace);
  return haserr ? -1 : 0;
}

int merstatistics(const GtStr *str_inputindex,
                  unsigned long searchlength,
                  unsigned long minocc,
                  unsigned long maxocc,
                  const GtStr *str_storeindex,
                  GT_UNUSED bool storecounts,
                  bool scanfile,
                  bool performtest,
                  Verboseinfo *verboseinfo,
                  GtError *err)
{
  bool haserr = false;
  Sequentialsuffixarrayreader *ssar;

  gt_error_check(err);
  ssar = newSequentialsuffixarrayreaderfromfile(str_inputindex,
                                                SARR_LCPTAB |
                                                SARR_SUFTAB |
                                                SARR_ESQTAB,
                                                (scanfile && !performtest)
                                                  ? SEQ_scan : SEQ_mappedboth,
                                                err);
  if (ssar == NULL)
  {
    haserr = true;
  }
  if (!haserr)
  {
    if (enumeratelcpintervals(str_inputindex,
                              ssar,
                              str_storeindex,
                              searchlength,
                              minocc,
                              maxocc,
                              performtest,
                              verboseinfo,
                              err) != 0)
    {
      haserr = true;
    }
  }
  if (ssar != NULL)
  {
    freeSequentialsuffixarrayreader(&ssar);
  }
  return haserr ? -1 : 0;
}
