#include <dlfcn.h>

#include "glyph.h"

static yaml_parser_t glyph_parser;
static yaml_emitter_t render_emitter;
static const char *metacode_temp_path = NULL;

static __attribute__((noreturn)) void fatal(const char *info) {
    fprintf(stderr, "GLYPH CORE FATAL: %s\n", info);
    exit(1);
}

static void
interp_metacode(void)
{
    const char *code = atom_fetch_text();
    atom_render_text(code);
    atom_emit_metacode(code);
}

static void
interp_metaexec(void)
{
    const char *symbol_name = atom_fetch_text();
    atom_set_color(0xD0, 0x30, 0x30);
    atom_render_text(symbol_name);
    if (fprintf(_glyph_metacode_log_output,
                "; metaexec request here for: %s\n", symbol_name) < 0) {
        perror("fprintf");
        fatal("fprintf for metaexec");
    }
    if (fflush(_glyph_metacode_log_output) < 0) {
        perror("fflush");
        fatal("flush for metaexec");
    }
    if (fclose(_glyph_metacode_temp_output) < 0) {
        perror("fclose");
        fatal("close for metaexec");
    }
    if (metacode_temp_path == NULL) {
        fatal("null metacode temp path");
    }
    static int next_plugin_id = 0;
    char filename[128];
    snprintf(filename, sizeof(filename), "./plugin%u.so", next_plugin_id++);
    char buffer[512];
    snprintf(buffer, sizeof(buffer), "clang -Wno-override-module -fPIC -shared %s -o %s",
                                     metacode_temp_path, filename);
    if (system(buffer) != 0) {
        fatal("compilation failed");
    }
    void *handle = dlopen(filename, RTLD_LAZY | RTLD_GLOBAL);
    if (handle == NULL) {
        fprintf(stderr, "dlopen error: %s\n", dlerror());
        fatal("dlopen failed");
    }
    void (*entry_func)(void) = dlsym(handle, symbol_name);
    if (entry_func == NULL) {
        fprintf(stderr, "dlsym error: %s\n", dlerror());
        fatal("dlsym failed");
    }
    entry_func();

    _glyph_metacode_temp_output = fopen(metacode_temp_path, "w");
    if (_glyph_metacode_temp_output == NULL) {
        perror(metacode_temp_path);
        fatal("cannot reopen metacode temp output");
    }
}

static void
define_atomic_glyphs(void)
{
    atom_define_glyph("metacode", interp_metacode);
    atom_define_glyph("metaexec", interp_metaexec);
}

static void
configure_io(char **argv)
{
    if (!yaml_parser_initialize(&glyph_parser)) {
        fatal("cannot initialize parser");
    }
    if (!yaml_emitter_initialize(&render_emitter)) {
        fatal("cannot initialize emitter");
    }

    FILE *input = fopen(argv[1], "rb");
    if (input == NULL) {
        perror(argv[1]);
        fatal("cannot open input source");
    }
    yaml_parser_set_input_file(&glyph_parser, input);

    _glyph_metacode_temp_output = fopen(argv[2], "w");
    if (_glyph_metacode_temp_output == NULL) {
        perror(argv[2]);
        fatal("cannot open metacode temp output");
    }
    metacode_temp_path = argv[2];

    _glyph_metacode_log_output = fopen(argv[3], "w");
    if (_glyph_metacode_log_output == NULL) {
        perror(argv[3]);
        fatal("cannot open metacode log output");
    }

    FILE *render = fopen(argv[4], "wb");
    if (render == NULL) {
        perror(argv[3]);
        fatal("cannot open render yaml output");
    }
    yaml_emitter_set_output_file(&render_emitter, render);
    _glyph_render_output = &render_emitter;
}

static void
deconfigure_io(void)
{
    yaml_parser_delete(&glyph_parser);
    yaml_emitter_delete(&render_emitter);
    if (fclose(_glyph_metacode_temp_output) < 0) {
        perror("fclose");
        fatal("deconfigure i/o temp fclose");
    }
    if (fclose(_glyph_metacode_log_output) < 0) {
        perror("fclose");
        fatal("deconfigure i/o log fclose");
    }
}

static void
parse(yaml_event_t *event)
{
    if (!yaml_parser_parse(&glyph_parser, event)) {
        fatal("parse failure");
    }
    if (event->type == YAML_ALIAS_EVENT) {
        fatal("unhandled yaml event: alias");
    }
}

static struct glyph_type *
find_glyph_type(const yaml_char_t *name, size_t len)
{
    for (struct glyph_type *type = _glyph_type_list; type != NULL; type = type->next) {
        if (len == strlen(type->name) && memcmp(type->name, name, len) == 0) {
            return type;
        }
    }
    fputs("unknown glyph: ", stderr);
    fwrite(name, 1, len, stderr);
    fputc('\n', stderr);
    fatal("attempt to use undefined glyph");
}

void
parse_glyph(struct glyph_instance *glyph_out)
{
    yaml_event_t ev;
    glyph_out->type = NULL;
    glyph_out->text_body = NULL;

    parse(&ev);
    if (ev.type != YAML_MAPPING_START_EVENT) {
        printf("event: %d\n", ev.type);
        fatal("glyph must start with a mapping event");
    }
    yaml_event_delete(&ev);

    parse(&ev);
    while (ev.type != YAML_MAPPING_END_EVENT) {
        if (ev.type != YAML_SCALAR_EVENT) {
            fatal("glyph must have scalar keys");
        }
        switch (ev.data.scalar.length) {
        case 5:
            if (memcmp(ev.data.scalar.value, "glyph", 5) == 0) {
                yaml_event_delete(&ev);
                parse(&ev);
                if (ev.type != YAML_SCALAR_EVENT) {
                    fatal("glyph field 'glyph' must have scalar value");
                }
                if (glyph_out->type != NULL) {
                    fatal("duplicate glyph field 'glyph'");
                }
                glyph_out->type = find_glyph_type(ev.data.scalar.value, ev.data.scalar.length);
                break;
            }
            goto bad_key;
        case 4:
            if (memcmp(ev.data.scalar.value, "text", 4) == 0) {
                yaml_event_delete(&ev);
                parse(&ev);
                if (ev.type != YAML_SCALAR_EVENT) {
                    fatal("glyph field 'text' must have scalar value");
                }
                if (glyph_out->text_body != NULL) {
                    fatal("duplicate glyph fild 'text'");
                }
                glyph_out->text_body = malloc(ev.data.scalar.length + 1);
                memcpy(glyph_out->text_body, ev.data.scalar.value, ev.data.scalar.length);
                glyph_out->text_body[ev.data.scalar.length] = '\0';
                break;
            }
            goto bad_key;
        default:
        bad_key:
            fatal("unexpected key in glyph");
        }
        yaml_event_delete(&ev);

        parse(&ev);
    }
    yaml_event_delete(&ev);

    if (glyph_out->type == NULL || glyph_out->text_body == NULL) {
        fatal("incomplete parsed glyph");
    }
}

static void
interpret_glyph(struct glyph_instance *glyph)
{
    _glyph_current = glyph;
    _glyph_begin_render();
    glyph->type->interp();
    _glyph_end_render();
    _glyph_current = NULL;
}

void
process_source(void)
{
    yaml_event_t ev;
    struct glyph_instance glyph;
    parse(&ev);

    if (ev.type != YAML_STREAM_START_EVENT) {
        fatal("invalid yaml: no stream start event");
    }

    yaml_event_delete(&ev);

    _glyph_outer_begin_render();

    parse(&ev);
    while (ev.type != YAML_STREAM_END_EVENT) {
        if (ev.type == YAML_STREAM_END_EVENT) {
            break;
        } else if (ev.type != YAML_DOCUMENT_START_EVENT) {
            fatal("invalid yaml: no document start event");
        }
        yaml_event_delete(&ev);

        parse_glyph(&glyph);

        parse(&ev);
        if (ev.type != YAML_DOCUMENT_END_EVENT) {
            fatal("invalid yaml: no document start event");
        }
        yaml_event_delete(&ev);

        interpret_glyph(&glyph);

        parse(&ev);
    }
    yaml_event_delete(&ev);

    _glyph_outer_end_render();
}

int
main(int argc, char *argv[])
{
    if (argc != 5) {
        fprintf(stderr, "usage: %s <input.gf> <metacode-tmp.ll> <metacode-log.ll> <render.yaml>\n", argv[0]);
        return 1;
    }

    configure_io(argv);
    define_atomic_glyphs();
    process_source();
    deconfigure_io();

    return 0;
}
