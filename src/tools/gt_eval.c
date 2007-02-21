/*
  Copyright (c) 2006-2007 Gordon Gremme <gremme@zbh.uni-hamburg.de>
  Copyright (c) 2006-2007 Center for Bioinformatics, University of Hamburg
  See LICENSE file or http://genometools.org/license.html for license details.
*/

#include "gt.h"

typedef struct {
  bool verbose,
       exondiff;
} EvalArguments;

static OPrval parse_options(int *parsed_args, EvalArguments *arguments,
                            int argc, char **argv, Error *err)
{
  OptionParser *op;
  Option *option;
  OPrval oprval;
  error_check(err);
  op = option_parser_new("reality_file prediction_file ",
                         "Evaluate a gene prediction against a given "
                         "``reality'' file (both in GFF3).");

  /* -v */
  option = option_new_verbose(&arguments->verbose);
  option_parser_add_option(op, option);

  /* -exondiff */
  option = option_new_bool("exondiff", "show a diff for the exons",
                           &arguments->exondiff, false);
  option_is_development_option(option);
  option_parser_add_option(op, option);

  /* parse */
  oprval = option_parser_parse_min_max_args(op, parsed_args, argc, argv,
                                            versionfunc, 2, 2, err);
  option_parser_delete(op);
  return oprval;
}

int gt_eval(int argc, char *argv[], Error *err)
{
  GenomeStream *reality_stream,
                *prediction_stream;
  StreamEvaluator *evaluator;
  EvalArguments arguments;
  int has_err, parsed_args;
  error_check(err);

  /* option parsing */
  switch (parse_options(&parsed_args, &arguments, argc, argv, err)) {
    case OPTIONPARSER_OK: break;
    case OPTIONPARSER_ERROR: return -1;
    case OPTIONPARSER_REQUESTS_EXIT: return 0;
  }

  /* create the reality stream */
  reality_stream = gff3_in_stream_new_sorted(argv[parsed_args],
                                             arguments.verbose);

  /* create the prediction stream */
  prediction_stream = gff3_in_stream_new_sorted(argv[parsed_args + 1],
                                                arguments.verbose);

  /* create the stream evaluator */
  evaluator = stream_evaluator_new(reality_stream, prediction_stream);

  /* compute the evaluation */
  has_err = stream_evaluator_evaluate(evaluator, arguments.verbose,
                                      arguments.exondiff, err);

  /* show the evaluation */
  if (!has_err)
    stream_evaluator_show(evaluator, stdout);

  /* free */
  stream_evaluator_delete(evaluator);

  return has_err;
}
