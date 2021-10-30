#ifndef GLYPH_H
#define GLYPH_H

#include <stdio.h>
#include <stdint.h>
#include <yaml.h>

struct glyph_type {
    void (*interp)(void);
    struct glyph_type *next;

    char name[];
};

struct glyph_instance {
    struct glyph_type *type;
    char *text_body;

    // TODO: glyph trees
};

extern FILE *_glyph_metacode_temp_output;
extern FILE *_glyph_metacode_log_output;
extern struct glyph_type *_glyph_type_list;
extern yaml_emitter_t *_glyph_render_output;
extern struct glyph_instance *_glyph_current;

extern void atom_define_glyph(const char *name, void (*interp)(void));
extern void atom_set_color(uint8_t red, uint8_t green, uint8_t blue);
extern const char *atom_fetch_text(void);
extern void atom_render_text(const char *text);
extern void atom_emit_metacode(const char *text);

extern void _glyph_begin_render(const char *name);
extern void _glyph_end_render(void);
extern void _glyph_outer_begin_render(void);
extern void _glyph_outer_end_render(void);

#endif
