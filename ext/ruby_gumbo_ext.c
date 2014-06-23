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
#include <ruby/encoding.h>

#include <gumbo.h>

void Init_gumbo(void);

VALUE r_gumbo_parse(VALUE module, VALUE input);
VALUE r_document_has_doctype(VALUE self);
VALUE r_element_attribute(VALUE self, VALUE name);
VALUE r_element_has_attribute(VALUE self, VALUE name);


static VALUE r_bool_new(bool val);
static VALUE r_sym_new(const char *str);
static VALUE r_str_new(const char *str, long len);
static VALUE r_tainted_str_new(const char *str, long len);
static VALUE r_cstr_new(const char *str);
static VALUE r_tainted_cstr_new(const char *str);

static VALUE r_gumbo_destroy_output(VALUE value);

static VALUE r_gumbo_source_position_to_value(GumboSourcePosition position);
static VALUE r_gumbo_node_type_to_symbol(GumboNodeType type);
static VALUE r_gumbo_parse_flags_to_symbol_array(GumboParseFlags flags);
static VALUE r_gumbo_quirks_mode_to_symbol(GumboQuirksModeEnum mode);
static VALUE r_gumbo_namespace_to_symbol(GumboNamespaceEnum ns);
static VALUE r_gumbo_tag_to_symbol(GumboTag tag);
static VALUE r_gumbo_node_to_value(GumboNode *node);
static VALUE r_gumbo_stringpiece_to_str(const GumboStringPiece* string);

static VALUE r_gumbo_attribute_namespace_to_symbol(GumboAttributeNamespaceEnum ns);
static VALUE r_gumbo_attribute_to_value(GumboAttribute *attribute);

static VALUE m_gumbo;
static VALUE c_node, c_document, c_element;
static VALUE c_text, c_cdata, c_comment, c_whitespace;
static VALUE c_attribute;
static VALUE c_source_position;


void
Init_gumbo_ext(void) {
    m_gumbo = rb_define_module("Gumbo");
    rb_define_module_function(m_gumbo, "parse", r_gumbo_parse, 1);

    c_node = rb_define_class_under(m_gumbo, "Node", rb_cObject);
    rb_define_attr(c_node, "type", 1, 0);
    rb_define_attr(c_node, "parent", 1, 0);
    rb_define_attr(c_node, "parse_flags", 1, 0);

    c_document = rb_define_class_under(m_gumbo, "Document", c_node);
    rb_define_attr(c_document, "name", 1, 0);
    rb_define_attr(c_document, "public_identifier", 1, 0);
    rb_define_attr(c_document, "system_identifier", 1, 0);
    rb_define_attr(c_document, "quirks_mode", 1, 0);
    rb_define_attr(c_document, "children", 1, 0);
    rb_define_method(c_document, "has_doctype?", r_document_has_doctype, 0);

    c_element = rb_define_class_under(m_gumbo, "Element", c_node);
    rb_define_attr(c_element, "tag", 1, 0);
    rb_define_attr(c_element, "original_tag", 1, 0);
    rb_define_attr(c_element, "original_tag_name", 1, 0);
    rb_define_attr(c_element, "original_end_tag", 1, 0);
    rb_define_attr(c_element, "original_end_tag_name", 1, 0);
    rb_define_attr(c_element, "tag_namespace", 1, 0);
    rb_define_attr(c_element, "attributes", 1, 0);
    rb_define_attr(c_element, "children", 1, 0);
    rb_define_attr(c_element, "start_pos", 1, 0);
    rb_define_attr(c_element, "end_pos", 1, 0);
    rb_define_method(c_element, "attribute", r_element_attribute, 1);
    rb_define_method(c_element, "has_attribute?", r_element_has_attribute, 1);

    c_text = rb_define_class_under(m_gumbo, "Text", c_node);
    rb_define_attr(c_text, "text", 1, 0);
    rb_define_attr(c_text, "original_text", 1, 0);
    rb_define_attr(c_text, "start_pos", 1, 0);

    c_cdata = rb_define_class_under(m_gumbo, "CData", c_text);
    c_comment = rb_define_class_under(m_gumbo, "Comment", c_text);
    c_whitespace = rb_define_class_under(m_gumbo, "Whitespace", c_text);

    c_attribute = rb_define_class_under(m_gumbo, "Attribute", rb_cObject);
    rb_define_attr(c_attribute, "namespace", 1, 0);
    rb_define_attr(c_attribute, "name", 1, 0);
    rb_define_attr(c_attribute, "original_name", 1, 0);
    rb_define_attr(c_attribute, "value", 1, 0);
    rb_define_attr(c_attribute, "original_value", 1, 0);
    rb_define_attr(c_attribute, "name_start", 1, 0);
    rb_define_attr(c_attribute, "name_end", 1, 0);
    rb_define_attr(c_attribute, "value_start", 1, 0);
    rb_define_attr(c_attribute, "value_end", 1, 0);

    c_source_position = rb_define_class_under(m_gumbo, "SourcePosition",
                                              rb_cObject);
    rb_define_attr(c_source_position, "line", 1, 0);
    rb_define_attr(c_source_position, "column", 1, 0);
    rb_define_attr(c_source_position, "offset", 1, 0);
}

/*
 * call-seq:
 *   Gumbo::parse(input) {|document| ...}
 *   Gumbo::parse(input)                -> document
 *
 * Parse a HTML document from a string. If the document cannot be created, a
 * runtime error is raised.
 *
 * The input string must be UTF-8 encoded.
 */
VALUE
r_gumbo_parse(VALUE module, VALUE input) {
    GumboOutput *output;
    GumboDocument *document;
    VALUE r_document, r_root;
    VALUE result;

    rb_check_type(input, T_STRING);

    if (rb_enc_get_index(input) != rb_utf8_encindex())
        rb_raise(rb_eArgError, "input is not UTF-8 encoded");

    output = gumbo_parse_with_options(&kGumboDefaultOptions,
                                      StringValueCStr(input),
                                      RSTRING_LEN(input));
    if (!output)
        rb_raise(rb_eRuntimeError, "cannot parse input");

    r_document = rb_ensure(r_gumbo_node_to_value, (VALUE)output->document,
                           r_gumbo_destroy_output, (VALUE)output);

    if (rb_block_given_p()) {
        result = rb_yield(r_document);
    } else {
        result = r_document;
    }

    return result;
}

/*
 * call-seq:
 *   document.has_doctype? -> boolean
 *
 * Return +true+ if the document has a doctype or +false+ else.
 */
VALUE
r_document_has_doctype(VALUE self) {
    return rb_iv_get(self, "@has_doctype");
}

/*
 * call-seq:
 *   element.attribute(name) -> attribute
 *
 * If +element+ has an attribute with the name +name+, return it. If not,
 * return +nil+.
 */
VALUE
r_element_attribute(VALUE self, VALUE name) {
    VALUE attributes;
    const char *name_str;

    name_str = StringValueCStr(name);

    attributes = rb_iv_get(self, "@attributes");
    for (long i = 0; i < RARRAY_LEN(attributes); i++) {
        VALUE attribute;
        VALUE r_attr_name;
        const char *attr_name;

        attribute = rb_ary_entry(attributes, i);
        r_attr_name = rb_iv_get(attribute, "@name");
        attr_name = StringValueCStr(r_attr_name);

        if (strcasecmp(attr_name, name_str) == 0)
            return attribute;
    }

    return Qnil;
}

/*
 * call-seq:
 *   element.has_attribute?(name) -> boolean
 *
 * Return +true+ if +element+ has an attribute with the name +name+ or
 * +false+ else.
 */
VALUE
r_element_has_attribute(VALUE self, VALUE name) {
    VALUE attribute;

    attribute = r_element_attribute(self, name);
    return (attribute == Qnil) ? Qfalse : Qtrue;
}

static VALUE
r_bool_new(bool val) {
    return val ? Qtrue : Qfalse;
}

static VALUE
r_sym_new(const char *str) {
    return ID2SYM(rb_intern(str));
}

static VALUE
r_str_new(const char *str, long len) {
    return str ? rb_enc_str_new(str, len, rb_utf8_encoding()) : Qnil;
}

static VALUE
r_tainted_str_new(const char *str, long len) {
    VALUE val;

    if (str) {
        val = rb_enc_str_new(str, len, rb_utf8_encoding());
        OBJ_TAINT(val);
    } else {
        val = Qnil;
    }

    return val;
}

static VALUE
r_cstr_new(const char *str) {
    return r_str_new(str, strlen(str));
}

static VALUE
r_tainted_cstr_new(const char *str) {
    return r_tainted_str_new(str, strlen(str));
}

static VALUE
r_gumbo_stringpiece_to_str(const GumboStringPiece* string) {
    return r_tainted_str_new(string->data, string->length);
}

static VALUE
r_gumbo_destroy_output(VALUE value) {
    GumboOutput *output;

    output = (GumboOutput*)value;
    gumbo_destroy_output(&kGumboDefaultOptions, output);

    return Qnil;
}

static VALUE
r_gumbo_source_position_to_value(GumboSourcePosition position) {
    VALUE r_position;

    r_position = rb_class_new_instance(0, NULL, c_source_position);

    rb_iv_set(r_position, "@line", UINT2NUM(position.line));
    rb_iv_set(r_position, "@column", UINT2NUM(position.column));
    rb_iv_set(r_position, "@offset", UINT2NUM(position.offset));

    return r_position;
}

static VALUE
r_gumbo_node_type_to_symbol(GumboNodeType type) {
    switch (type) {
        case GUMBO_NODE_DOCUMENT:
            return r_sym_new("document");
        case GUMBO_NODE_ELEMENT:
            return r_sym_new("element");
        case GUMBO_NODE_TEXT:
            return r_sym_new("text");
        case GUMBO_NODE_CDATA:
            return r_sym_new("cdata");
        case GUMBO_NODE_COMMENT:
            return r_sym_new("comment");
        case GUMBO_NODE_WHITESPACE:
            return r_sym_new("whitespace");
        default:
            rb_raise(rb_eArgError, "unknown node type %d", type);
    }
}

static VALUE
r_gumbo_parse_flags_to_symbol_array(GumboParseFlags flags) {
    VALUE array;

    array = rb_ary_new();

    if (flags & GUMBO_INSERTION_NORMAL)
        rb_ary_push(array, r_sym_new("insertion_normal"));
    if (flags & GUMBO_INSERTION_BY_PARSER)
        rb_ary_push(array, r_sym_new("insertion_by_parser"));
    if (flags & GUMBO_INSERTION_IMPLICIT_END_TAG)
        rb_ary_push(array, r_sym_new("insertion_implicit_end_tag"));
    if (flags & GUMBO_INSERTION_IMPLIED)
        rb_ary_push(array, r_sym_new("insertion_implied"));
    if (flags & GUMBO_INSERTION_CONVERTED_FROM_END_TAG)
        rb_ary_push(array, r_sym_new("insertion_converted_from_end_tag"));
    if (flags & GUMBO_INSERTION_FROM_ISINDEX)
        rb_ary_push(array, r_sym_new("insertion_from_isindex"));
    if (flags & GUMBO_INSERTION_FROM_IMAGE)
        rb_ary_push(array, r_sym_new("insertion_from_image"));
    if (flags & GUMBO_INSERTION_RECONSTRUCTED_FORMATTING_ELEMENT)
        rb_ary_push(array, r_sym_new("insertion_reconstructed_formatting_element"));
    if (flags & GUMBO_INSERTION_ADOPTION_AGENCY_CLONED)
        rb_ary_push(array, r_sym_new("insertion_adoption_agency_cloned"));
    if (flags & GUMBO_INSERTION_ADOPTION_AGENCY_MOVED)
        rb_ary_push(array, r_sym_new("insertion_adoption_agency_moved"));
    if (flags & GUMBO_INSERTION_FOSTER_PARENTED)
        rb_ary_push(array, r_sym_new("insertion_foster_parented"));

    return array;
}

static VALUE
r_gumbo_quirks_mode_to_symbol(GumboQuirksModeEnum mode) {
    switch (mode) {
        case GUMBO_DOCTYPE_NO_QUIRKS:
            return r_sym_new("no_quirks");
        case GUMBO_DOCTYPE_QUIRKS:
            return r_sym_new("quirks");
        case GUMBO_DOCTYPE_LIMITED_QUIRKS:
            return r_sym_new("limited_quirks");
        default:
            rb_raise(rb_eArgError, "unknown quirks mode %d", mode);
    }
}

static VALUE
r_gumbo_namespace_to_symbol(GumboNamespaceEnum ns) {
    switch (ns) {
        case GUMBO_NAMESPACE_HTML:
            return r_sym_new("html");
        case GUMBO_NAMESPACE_SVG:
            return r_sym_new("svg");
        case GUMBO_NAMESPACE_MATHML:
            return r_sym_new("mathml");
        default:
            rb_raise(rb_eArgError, "unknown namespace %d", ns);
    }
}

static VALUE
r_gumbo_tag_to_symbol(GumboTag tag) {
    const char *name;

    if (tag < 0 || tag >= GUMBO_TAG_LAST)
        rb_raise(rb_eArgError, "unknown tag %d", tag);

    if (tag == GUMBO_TAG_UNKNOWN) {
        name = "unknown";
    } else {
        name = gumbo_normalized_tagname(tag);
    }

    return r_sym_new(name);
}

static VALUE
r_gumbo_node_to_value(GumboNode *node) {
    VALUE class;
    VALUE r_node;
    GumboVector *children;

    if (node->type == GUMBO_NODE_DOCUMENT) {
        class = c_document;
    } else if (node->type == GUMBO_NODE_ELEMENT) {
        class = c_element;
    } else if (node->type == GUMBO_NODE_TEXT) {
        class = c_text;
    } else if (node->type == GUMBO_NODE_CDATA) {
        class = c_cdata;
    } else if (node->type == GUMBO_NODE_COMMENT) {
        class = c_comment;
    } else if (node->type == GUMBO_NODE_WHITESPACE) {
        class = c_whitespace;
    } else {
        rb_raise(rb_eArgError, "unknown node type %d", node->type);
    }

    r_node = rb_class_new_instance(0, NULL, class);
    rb_iv_set(r_node, "@type", r_gumbo_node_type_to_symbol(node->type));
    rb_iv_set(r_node, "@parent", Qnil);
    rb_iv_set(r_node, "@parse_flags",
              r_gumbo_parse_flags_to_symbol_array(node->parse_flags));

    children = NULL;

    if (node->type == GUMBO_NODE_DOCUMENT) {
        GumboDocument *document;

        document = &node->v.document;
        children = &document->children;

        rb_iv_set(r_node, "@name", r_tainted_cstr_new(document->name));
        rb_iv_set(r_node, "@public_identifier",
                  r_tainted_cstr_new(document->public_identifier));
        rb_iv_set(r_node, "@system_identifier",
                  r_tainted_cstr_new(document->system_identifier));
        rb_iv_set(r_node, "@quirks_mode",
                  r_gumbo_quirks_mode_to_symbol(document->doc_type_quirks_mode));
        rb_iv_set(r_node, "@has_doctype", r_bool_new(document->has_doctype));
    } else if (node->type == GUMBO_NODE_ELEMENT) {
        GumboElement *element;
        VALUE r_attributes;

        element = &node->v.element;
        children = &element->children;

        rb_iv_set(r_node, "@tag",
                  r_gumbo_tag_to_symbol(element->tag));
        rb_iv_set(r_node, "@original_tag",
                  r_gumbo_stringpiece_to_str(&element->original_tag));
        rb_iv_set(r_node, "@original_end_tag",
                  r_gumbo_stringpiece_to_str(&element->original_end_tag));
        rb_iv_set(r_node, "@tag_namespace",
                  r_gumbo_namespace_to_symbol(element->tag_namespace));
        rb_iv_set(r_node, "@start_pos",
                  r_gumbo_source_position_to_value(element->start_pos));
        rb_iv_set(r_node, "@end_pos",
                  r_gumbo_source_position_to_value(element->end_pos));

        GumboStringPiece original_tag_name = element->original_tag;
        gumbo_tag_from_original_text(&original_tag_name);
        rb_iv_set(r_node, "@original_tag_name",
                  r_gumbo_stringpiece_to_str(&original_tag_name));

        GumboStringPiece original_end_tag_name = element->original_end_tag;
        gumbo_tag_from_original_text(&original_end_tag_name);
        rb_iv_set(r_node, "@original_end_tag_name",
                  r_gumbo_stringpiece_to_str(&original_end_tag_name));

        r_attributes = rb_ary_new2(element->attributes.length);
        rb_iv_set(r_node, "@attributes", r_attributes);

        for (unsigned int i = 0; i < element->attributes.length; i++) {
            GumboAttribute *attribute;
            VALUE r_attribute;

            attribute = element->attributes.data[i];
            r_attribute = r_gumbo_attribute_to_value(attribute);

            rb_ary_store(r_attributes, i, r_attribute);
        }
    } else if (node->type == GUMBO_NODE_TEXT
            || node->type == GUMBO_NODE_CDATA
            || node->type == GUMBO_NODE_COMMENT
            || node->type == GUMBO_NODE_WHITESPACE) {
        GumboText *text;

        text = &node->v.text;

        rb_iv_set(r_node, "@text", r_tainted_cstr_new(text->text));
        rb_iv_set(r_node, "@original_text",
                  r_gumbo_stringpiece_to_str(&text->original_text));
        rb_iv_set(r_node, "@start_pos",
                  r_gumbo_source_position_to_value(text->start_pos));
    }

    if (children) {
        VALUE r_children;

        r_children = rb_ary_new2(children->length);
        rb_iv_set(r_node, "@children", r_children);

        for (unsigned int i = 0; i < children->length; i++) {
            GumboNode *child;
            VALUE r_child;

            child = children->data[i];
            r_child = r_gumbo_node_to_value(child);

            rb_iv_set(r_child, "@parent", r_node);

            rb_ary_store(r_children, i, r_child);
        }
    }

    return r_node;
}

static VALUE
r_gumbo_attribute_namespace_to_symbol(GumboAttributeNamespaceEnum ns) {
    switch (ns) {
        case GUMBO_ATTR_NAMESPACE_NONE:
            return Qnil;
        case GUMBO_ATTR_NAMESPACE_XLINK:
            return r_sym_new("xlink");
        case GUMBO_ATTR_NAMESPACE_XML:
            return r_sym_new("xml");
        case GUMBO_ATTR_NAMESPACE_XMLNS:
            return r_sym_new("xmlns");
        default:
            rb_raise(rb_eArgError, "unknown namespace %d", ns);
    }
}

static VALUE
r_gumbo_attribute_to_value(GumboAttribute *attribute) {
    VALUE r_attribute;

    r_attribute = rb_class_new_instance(0, NULL, c_attribute);

    rb_iv_set(r_attribute, "@namespace",
              r_gumbo_attribute_namespace_to_symbol(attribute->attr_namespace));
    rb_iv_set(r_attribute, "@name", r_tainted_cstr_new(attribute->name));
    rb_iv_set(r_attribute, "@original_name",
              r_gumbo_stringpiece_to_str(&attribute->original_name));
    rb_iv_set(r_attribute, "@value", r_tainted_cstr_new(attribute->value));
    rb_iv_set(r_attribute, "@original_value",
              r_gumbo_stringpiece_to_str(&attribute->original_value));
    rb_iv_set(r_attribute, "@name_start",
              r_gumbo_source_position_to_value(attribute->name_start));
    rb_iv_set(r_attribute, "@name_end",
              r_gumbo_source_position_to_value(attribute->name_end));
    rb_iv_set(r_attribute, "@value_start",
              r_gumbo_source_position_to_value(attribute->value_start));
    rb_iv_set(r_attribute, "@value_end",
              r_gumbo_source_position_to_value(attribute->value_end));

    return r_attribute;
}
