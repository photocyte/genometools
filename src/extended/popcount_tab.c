/*
  Copyright (c) 2012 Dirk Willrodt <willrodt@zbh.uni-hamburg.de>
  Copyright (c) 2012 Center for Bioinformatics, University of Hamburg

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

#include "core/bitpackarray.h"
#include "core/ma.h"
#include "core/unused_api.h"
#include "extended/popcount_tab.h"

struct GtPopcountTab
{
  BitPackArray *class_start,
               *bits;
};

GtPopcountTab *gt_popcount_tab_new(GT_UNUSED unsigned char blocksize)
{
  GtPopcountTab *popcount_tab;
  popcount_tab = gt_malloc(sizeof (GtPopcountTab));
  return popcount_tab;
}

void gt_popcount_tab_delete(GtPopcountTab *popcount_tab)
{
  gt_free(popcount_tab);
}
