/*
  Copyright (c) 2007 Gordon Gremme <gremme@zbh.uni-hamburg.de>
  Copyright (c) 2007 Center for Bioinformatics, University of Hamburg
  See LICENSE file or http://genometools.org/license.html for license details.
*/

#include "gt.h"

static OPrval parse_options(int *parsed_args, int argc, char **argv, Error *err)
{
  OptionParser *op;
  OPrval oprval;
  error_check(err);
  op = option_parser_new("file [...]", "Map the supplied files into memory and "
                         "read them once.");
  oprval = option_parser_parse_min_args(op, parsed_args, argc, argv,
                                        versionfunc, 1, err);
  option_parser_delete(op);
  return oprval;
}

int gt_mmapandread(int argc, char *argv[], Error *err)
{
  int i, fd, parsed_args;
  void *map;
  struct stat sb;
  unsigned long j;
  char byte = 0;
  error_check(err);

  /* option parsing */
  switch (parse_options(&parsed_args, argc, argv, err)) {
    case OPTIONPARSER_OK: break;
    case OPTIONPARSER_ERROR: return -1;
    case OPTIONPARSER_REQUESTS_EXIT: return 0;
  }

  /* iterate over all files */
  for (i = parsed_args; i < argc; i++) {
    /* open file */
    fd = xopen(argv[i], O_RDONLY, 0);

    /* get file statistics */
    xfstat(fd, &sb);

    if (sb.st_size == 0)
      printf("file \"%s\" is empty\n", argv[i]);
    else if (!(sb.st_mode & S_IFREG))
      printf("\"%s\" is not a regular file\n", argv[i]);
    else {
      /* map file */
      map = xmmap(0, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);

      /* read file */
      printf("reading file \"%s\"\n", argv[i]);
      j = 0;
      progressbar_start(&j, (unsigned long) sb.st_size);
      for (; j < (unsigned long) sb.st_size; j++)
        byte |= ((char*) map)[j];
      progressbar_stop();

      /* unmap file */
      xmunmap(map, sb.st_size);
    }

    /* close file */
    xclose(fd);
  }

  if (!byte)
    printf("all read files contained only null characters\n");

  return 0;
}
