/*
   Copyright (c) 2007 Christin Schaerfer <cschaerfer@stud.zbh.uni-hamburg.de>
   Copyright (c) 2007 Center for Bioinformatics, University of Hamburg
   See LICENSE file or http://genometools.org/license.html for license details.
*/

#ifndef TRACK_H
#define TRACK_H

#include <libgtcore/str.h>
#include <libgtcore/array.h>
#include <libgtext/genome_node.h>
#include <libgtview/config.h>
#include <libgtview/line.h>

/*!
Track interface
A track has a title and a type und contains
line objects
*/
typedef struct Track Track;

/*!
reates a new Track object.
\param title Title of the Track object.
\param env Pointer to Environment object.
\return Pointer to a new Track object.
*/
Track* track_new(Str* title,
                 Env* env);

/*!
Inserts an element into a Track object
\param track Track in which to insert element
\param gn Pointer to GenomeNode object
\param cfg Pointer to Config file
\param env Pointer to Environment object
*/
void track_insert_element(Track* track,
                          GenomeNode* gn,
			  Config* cfg,
			  GenomeNode* parent,
			  Env* env);

/*!
Returns Track title
\param track Pointer to Track object
\teturn Pointer to Title String object
*/
Str* track_get_title(Track* track);

/*!
Gets the next unoccupied Line object
\param lines Array with Line objects
\param gn Pointer to GenomeNode object
\return Pointer to unoccupied Line object
*/
Line* get_next_free_line(Track* track,
                         Range r,
			 Env* env);

/*!
Returns Array with Pointer to Line objects
\param track Pointer to Track object
\return Pointer to Array
*/
Array* track_get_lines(Track* track);

/*!
Returns Number of Lines of a Track object
\param track Pointer to Track object
*/
int track_get_number_of_lines(Track* track);

/*!
Sort all blocks into lines
\param track Pointer to Track object
\param env Pointer to Environment object
*/
void track_finish(Track* track,
                  Env* env);

/*!
Tests Track Class
\param env Pointer to Environment object
*/
int track_unit_test(Env* env);

/*!
Delets Track
\param Track Pointer to Track object to delete
\param env Pointer to Environment object
*/
void track_delete(Track* track,
                  Env* env);

#endif

