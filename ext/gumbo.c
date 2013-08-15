/*
 * Copyright (c) 2013 Nicolas Martyanoff
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <ruby.h>

#include <gumbo.h>

void Init_gumbo(void);

VALUE r_gumbo_parse(VALUE module, VALUE input);
VALUE r_document_has_doctype(VALUE self);


static VALUE r_bool_new(bool val);
static VALUE r_cstr_new(const char *str);
static VALUE r_gumbo_quirks_mode_to_symbol(GumboQuirksModeEnum mode);
static VALUE r_gumbo_document_to_value(GumboDocument *document);

static VALUE m_gumbo;
static VALUE c_document, c_element;


void
Init_gumbo(void) {
    m_gumbo = rb_define_module("Gumbo");
    rb_define_module_function(m_gumbo, "parse", r_gumbo_parse, 1);

    c_document = rb_define_class_under(m_gumbo, "Document", rb_cObject);
    rb_define_attr(c_document, "name", 1, 0);
    rb_define_attr(c_document, "public_identifier", 1, 0);
    rb_define_attr(c_document, "system_identifier", 1, 0);
    rb_define_attr(c_document, "quirks_mode", 1, 0);
    rb_define_method(c_document, "has_doctype?", r_document_has_doctype, 0);

    c_element = rb_define_class_under(m_gumbo, "Element", rb_cObject);
}

VALUE
r_gumbo_parse(VALUE module, VALUE input) {
    GumboOutput *output;
    GumboDocument *document;
    VALUE r_document;
    VALUE result;

    rb_check_type(input, T_STRING);

    if (!rb_block_given_p())
        rb_raise(rb_eLocalJumpError, "no block given");

    output = gumbo_parse_with_options(&kGumboDefaultOptions,
                                      StringValueCStr(input),
                                      RSTRING_LEN(input));
    if (!output)
        rb_raise(rb_eRuntimeError, "cannot parse input");

    r_document = r_gumbo_document_to_value(&output->document->v.document);

    result = rb_yield(r_document);

    gumbo_destroy_output(&kGumboDefaultOptions, output);
    return result;
}

VALUE
r_document_has_doctype(VALUE self) {
    return rb_iv_get(self, "@has_doctype");
}

static VALUE
r_bool_new(bool val) {
    return val ? Qtrue : Qfalse;
}

static VALUE
r_cstr_new(const char *str) {
    return str ? rb_str_new2(str) : Qnil;
}

static VALUE
r_gumbo_quirks_mode_to_symbol(GumboQuirksModeEnum mode) {
    switch (mode) {
#define DEFINE_MODE(value_, sym_) \
        case value_: \
            return ID2SYM(rb_intern(#sym_))

        DEFINE_MODE(GUMBO_DOCTYPE_NO_QUIRKS, no_quirks);
        DEFINE_MODE(GUMBO_DOCTYPE_QUIRKS, quirks);
        DEFINE_MODE(GUMBO_DOCTYPE_LIMITED_QUIRKS, limited_quirks);

#undef DEFINE_MODE

        default:
            return Qnil;
    }
}

static VALUE
r_gumbo_document_to_value(GumboDocument *document) {
    VALUE r_document;

    r_document = rb_class_new_instance(0, NULL, c_document);

    rb_iv_set(r_document, "@name",
              r_cstr_new(document->name));
    rb_iv_set(r_document, "@public_identifier",
              r_cstr_new(document->public_identifier));
    rb_iv_set(r_document, "@system_identifier",
              r_cstr_new(document->system_identifier));
    rb_iv_set(r_document, "@quirks_mode",
              r_gumbo_quirks_mode_to_symbol(document->doc_type_quirks_mode));
    rb_iv_set(r_document, "@has_doctype",
              r_bool_new(document->has_doctype));

    return r_document;
}
