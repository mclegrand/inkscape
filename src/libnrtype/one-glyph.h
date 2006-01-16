/** \file Definition of struct one_glyph. */

/*
 * License: May be redistributed with or without modifications under the terms of the Gnu General
 * Public License as published by the Free Software Foundation, version 2 or (at your option) any
 * later version.
 */

#ifndef LIBNRTYPE_ONE_GLYPH_H_INKSCAPE
#define LIBNRTYPE_ONE_GLYPH_H_INKSCAPE

#include <pango/pango-types.h>

/**
 * Information for a single glyph.
 *
 * Pango converts the text into glyphs, but scatters the info for a given glyph; here is a
 * structure holding what inkscape needs to know.
 */
struct one_glyph {
    int gl;  ///< glyph_id
    double x, y; ///< glyph position in the layout (nominal sizes, in the [0..1] range).
    bool char_start; /**< Whether this glyph is the beginning of a letter. (RTL is taken in
                      * account.) */
    bool word_start; ///< Whether this glyph is the beginning of a word.
    bool para_start; ///< Whether this glyph is the beginning of a paragraph (for indentation).
    char uni_dir;    ///< BiDi orientation of the run containing this glyph.
    int uni_st, uni_en; /**< Start and end positions of the text corresponding to this glyph.
                         * You always have uni_st < uni_en. */
    PangoFont *font;  /**< Font this glyph uses.  (For bidi text, you need several fonts.)
                       * When rendering glyphs, check if this font is the one you're using. */
};


#endif /* !LIBNRTYPE_ONE_GLYPH_H_INKSCAPE */

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=99 :
