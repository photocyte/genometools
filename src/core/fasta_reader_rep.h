/*
  Copyright (c) 2008 Gordon Gremme <gremme@zbh.uni-hamburg.de>
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

#ifndef GT_FASTA_READER_REP_H
#define GT_FASTA_READER_REP_H

#include <stdio.h>
#include "core/fasta_reader.h"

struct GT_FastaReaderClass
{
  size_t size;
  int  (*run)(GT_FastaReader*, GT_FastaReaderProcDescription,
              GT_FastaReaderProcSequencePart, GT_FastaReaderProcSequenceLength,
              void *data, GT_Error*);
  void (*free)(GT_FastaReader*);
};

struct GT_FastaReader
{
  const GT_FastaReaderClass *c_class;
};

GT_FastaReader* gt_fasta_reader_create(const GT_FastaReaderClass*);
void*        gt_fasta_reader_cast(const GT_FastaReaderClass*, GT_FastaReader*);

#endif
