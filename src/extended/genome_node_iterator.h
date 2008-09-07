/*
  Copyright (c) 2007-2008 Gordon Gremme <gremme@zbh.uni-hamburg.de>
  Copyright (c) 2007-2008 Center for Bioinformatics, University of Hamburg

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

#ifndef GENOME_NODE_ITERATOR_H
#define GENOME_NODE_ITERATOR_H

#include "extended/genome_node.h"

typedef struct GT_GenomeNodeIterator GT_GenomeNodeIterator;

/* Return a new genome node iterator which performs a depth-first traversal of
   <genome_node> (including <genome_node> itself). */
GT_GenomeNodeIterator* genome_node_iterator_new(GT_GenomeNode *genome_node);
/* Return a new genome node iterator which iterates over all direct children of
   <genome_node> (without <genome_node> itself). */
GT_GenomeNodeIterator* genome_node_iterator_new_direct(GT_GenomeNode *genome_node);
GT_GenomeNode*         genome_node_iterator_next(GT_GenomeNodeIterator*);
int                 genome_node_iterator_example(GT_Error *);
void                genome_node_iterator_delete(GT_GenomeNodeIterator*);

#endif
