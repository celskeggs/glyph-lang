#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "glyph.h"

FILE *_glyph_metacode_temp_output = NULL;
FILE *_glyph_metacode_log_output = NULL;
yaml_emitter_t *_glyph_render_output = NULL;
struct glyph_type *_glyph_type_list = NULL;
struct glyph_instance *_glyph_current = NULL;

static __attribute__((noreturn)) void fatal(const char *info) {
    fprintf(stderr, "GLYPH API FATAL: %s\n", info);
    exit(1);
}

void atom_define_glyph(const char *name, void (*interp)(void)) {
    struct glyph_type *glyph =
            malloc(sizeof(struct glyph_type) + strlen(name) + 1);
    if (glyph == NULL) {
        fatal("OOM in atom_define_glyph");
    }
    strcpy(glyph->name, name);
    glyph->interp = interp;
    glyph->next = _glyph_type_list;
    _glyph_type_list = glyph;
}

static void emit_event(yaml_event_t *event, int ok) {
    if (!ok) {
        fatal("could not initialize event");
    }
    if (_glyph_render_output == NULL) {
        fatal("no render output (set to null)");
    }
    if (!yaml_emitter_emit(_glyph_render_output, event)) {
        fatal("could not emit rendering event");
    }
//    yaml_event_delete(event);
}

static void start_stream(void) {
    yaml_event_t event;
    emit_event(&event,
        yaml_stream_start_event_initialize(&event, YAML_UTF8_ENCODING));
}

static void end_stream(void) {
    yaml_event_t event;
    emit_event(&event, yaml_stream_end_event_initialize(&event));
}

static void start_document(void) {
    yaml_event_t event;
    emit_event(&event,
        yaml_document_start_event_initialize(&event, NULL, NULL, NULL, 1));
}

static void end_document(void) {
    yaml_event_t event;
    emit_event(&event, yaml_document_end_event_initialize(&event, 1));
}

static void start_sequence(void) {
    yaml_event_t event;
    emit_event(&event,
        yaml_sequence_start_event_initialize(&event, NULL, NULL, 1,
                                             YAML_BLOCK_SEQUENCE_STYLE));
}

static void end_sequence(void) {
    yaml_event_t event;
    emit_event(&event, yaml_sequence_end_event_initialize(&event));
}

static void start_mapping(void) {
    yaml_event_t event;
    emit_event(&event,
        yaml_mapping_start_event_initialize(&event, NULL, NULL, 1,
                                            YAML_BLOCK_MAPPING_STYLE));
}

static void end_mapping(void) {
    yaml_event_t event;
    emit_event(&event, yaml_mapping_end_event_initialize(&event));
}

static void scalar(const char *string) {
    yaml_event_t event;
    emit_event(&event,
        yaml_scalar_event_initialize(&event, NULL, NULL,
                                     (yaml_char_t *) string, strlen(string),
                                     1, 1, YAML_ANY_SCALAR_STYLE));
}

static void scalar_int(uint8_t value) {
    char temp[32];
    sprintf(temp, "%u", value);
    scalar(temp);
}

void atom_set_color(uint8_t red, uint8_t green, uint8_t blue) {
    start_mapping();
    scalar("op");
    scalar("set_color");
    scalar("red");
    scalar_int(red);
    scalar("green");
    scalar_int(green);
    scalar("blue");
    scalar_int(blue);
    end_mapping();
}

const char *atom_fetch_text(void) {
    if (_glyph_current == NULL) {
        fatal("atom_fetch_text: no current glyph");
    }
    if (_glyph_current->text_body == NULL) {
        fatal("atom_fetch_text: glyph body missing");
    }
    return _glyph_current->text_body;
}

void atom_render_text(const char *text) {
    start_mapping();
    scalar("op");
    scalar("render_text");
    scalar("text");
    scalar(text);
    end_mapping();
}

void _glyph_begin_render(void) {
    start_mapping();
    scalar("op");
    scalar("glyph_bubble");
    scalar("content");
    start_sequence();
}

extern void _glyph_end_render(void) {
    end_sequence();
    end_mapping();
}

extern void _glyph_outer_begin_render(void) {
    start_stream();
    start_document();
    start_sequence();
}

extern void _glyph_outer_end_render(void) {
    end_sequence();
    end_document();
    end_stream();
}

void atom_emit_metacode(const char *code) {
    if (_glyph_metacode_temp_output == NULL) {
        fatal("null metacode output (temp)");
    }
    if (_glyph_metacode_log_output == NULL) {
        fatal("null metacode output (log)");
    }
    if (fputs(code, _glyph_metacode_temp_output) < 0) {
        perror("fputs");
        fatal("metacode emit error (temp)");
    }
    if (fputs(code, _glyph_metacode_log_output) < 0) {
        perror("fputs");
        fatal("metacode emit error (log)");
    }
}
