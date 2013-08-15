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
static VALUE r_sym_new(const char *str);
static VALUE r_str_new(const char *str, long len);
static VALUE r_tainted_str_new(const char *str, long len);
static VALUE r_cstr_new(const char *str);
static VALUE r_tainted_cstr_new(const char *str);

static VALUE r_gumbo_node_type_to_symbol(GumboNodeType type);
static VALUE r_gumbo_quirks_mode_to_symbol(GumboQuirksModeEnum mode);
static VALUE r_gumbo_namespace_to_symbol(GumboNamespaceEnum ns);
static VALUE r_gumbo_tag_to_symbol(GumboTag tag);
static VALUE r_gumbo_node_to_value(GumboNode *node);

static VALUE r_gumbo_attribute_namespace_to_symbol(GumboAttributeNamespaceEnum ns);
static VALUE r_gumbo_attribute_to_value(GumboAttribute *attribute);

static VALUE m_gumbo;
static VALUE c_node, c_document, c_element;
static VALUE c_attribute;


void
Init_gumbo(void) {
    m_gumbo = rb_define_module("Gumbo");
    rb_define_module_function(m_gumbo, "parse", r_gumbo_parse, 1);

    c_node = rb_define_class_under(m_gumbo, "Node", rb_cObject);
    rb_define_attr(c_node, "type", 1, 0);

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
    rb_define_attr(c_element, "tag_namespace", 1, 0);
    rb_define_attr(c_element, "attributes", 1, 0);
    rb_define_attr(c_element, "children", 1, 0);

    c_attribute = rb_define_class_under(m_gumbo, "Attribute", rb_cObject);
    rb_define_attr(c_attribute, "namespace", 1, 0);
    rb_define_attr(c_attribute, "name", 1, 0);
    rb_define_attr(c_attribute, "original_name", 1, 0);
    rb_define_attr(c_attribute, "value", 1, 0);
    rb_define_attr(c_attribute, "original_value", 1, 0);
}

VALUE
r_gumbo_parse(VALUE module, VALUE input) {
    GumboOutput *output;
    GumboDocument *document;
    VALUE r_document, r_root;
    VALUE result;

    rb_check_type(input, T_STRING);

    if (!rb_block_given_p())
        rb_raise(rb_eLocalJumpError, "no block given");

    output = gumbo_parse_with_options(&kGumboDefaultOptions,
                                      StringValueCStr(input),
                                      RSTRING_LEN(input));
    if (!output)
        rb_raise(rb_eRuntimeError, "cannot parse input");

    r_document = r_gumbo_node_to_value(output->document);

    result = rb_yield_values(1, r_document);

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
r_sym_new(const char *str) {
    return ID2SYM(rb_intern(str));
}

static VALUE
r_str_new(const char *str, long len) {
    return str ? rb_str_new(str, len) : Qnil;
}

static VALUE
r_tainted_str_new(const char *str, long len) {
    VALUE val;

    if (str) {
        val = rb_str_new(str, len);
        OBJ_TAINT(str);
    } else {
        val = Qnil;
    }

    return val;
}

static VALUE
r_cstr_new(const char *str) {
    return str ? rb_str_new2(str) : Qnil;
}

static VALUE
r_tainted_cstr_new(const char *str) {
    VALUE val;

    if (str) {
        val = rb_str_new2(str);
        OBJ_TAINT(str);
    } else {
        val = Qnil;
    }

    return val;
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
    switch (tag) {
        case GUMBO_TAG_HTML:
            return r_sym_new("html");
        case GUMBO_TAG_HEAD:
            return r_sym_new("head");
        case GUMBO_TAG_TITLE:
            return r_sym_new("title");
        case GUMBO_TAG_BASE:
            return r_sym_new("base");
        case GUMBO_TAG_LINK:
            return r_sym_new("link");
        case GUMBO_TAG_META:
            return r_sym_new("meta");
        case GUMBO_TAG_STYLE:
            return r_sym_new("style");
        case GUMBO_TAG_SCRIPT:
            return r_sym_new("script");
        case GUMBO_TAG_NOSCRIPT:
            return r_sym_new("noscript");
        case GUMBO_TAG_BODY:
            return r_sym_new("body");
        case GUMBO_TAG_SECTION:
            return r_sym_new("section");
        case GUMBO_TAG_NAV:
            return r_sym_new("nav");
        case GUMBO_TAG_ARTICLE:
            return r_sym_new("article");
        case GUMBO_TAG_ASIDE:
            return r_sym_new("aside");
        case GUMBO_TAG_H1:
            return r_sym_new("h1");
        case GUMBO_TAG_H2:
            return r_sym_new("h2");
        case GUMBO_TAG_H3:
            return r_sym_new("h3");
        case GUMBO_TAG_H4:
            return r_sym_new("h4");
        case GUMBO_TAG_H5:
            return r_sym_new("h5");
        case GUMBO_TAG_H6:
            return r_sym_new("h6");
        case GUMBO_TAG_HGROUP:
            return r_sym_new("hgroup");
        case GUMBO_TAG_HEADER:
            return r_sym_new("header");
        case GUMBO_TAG_FOOTER:
            return r_sym_new("footer");
        case GUMBO_TAG_ADDRESS:
            return r_sym_new("address");
        case GUMBO_TAG_P:
            return r_sym_new("p");
        case GUMBO_TAG_HR:
            return r_sym_new("hr");
        case GUMBO_TAG_PRE:
            return r_sym_new("pre");
        case GUMBO_TAG_BLOCKQUOTE:
            return r_sym_new("blockquote");
        case GUMBO_TAG_OL:
            return r_sym_new("ol");
        case GUMBO_TAG_UL:
            return r_sym_new("ul");
        case GUMBO_TAG_LI:
            return r_sym_new("li");
        case GUMBO_TAG_DL:
            return r_sym_new("dl");
        case GUMBO_TAG_DT:
            return r_sym_new("dt");
        case GUMBO_TAG_DD:
            return r_sym_new("dd");
        case GUMBO_TAG_FIGURE:
            return r_sym_new("figure");
        case GUMBO_TAG_FIGCAPTION:
            return r_sym_new("figcaption");
        case GUMBO_TAG_DIV:
            return r_sym_new("div");
        case GUMBO_TAG_A:
            return r_sym_new("a");
        case GUMBO_TAG_EM:
            return r_sym_new("em");
        case GUMBO_TAG_STRONG:
            return r_sym_new("strong");
        case GUMBO_TAG_SMALL:
            return r_sym_new("small");
        case GUMBO_TAG_S:
            return r_sym_new("s");
        case GUMBO_TAG_CITE:
            return r_sym_new("cite");
        case GUMBO_TAG_Q:
            return r_sym_new("q");
        case GUMBO_TAG_DFN:
            return r_sym_new("dfn");
        case GUMBO_TAG_ABBR:
            return r_sym_new("abbr");
        case GUMBO_TAG_TIME:
            return r_sym_new("time");
        case GUMBO_TAG_CODE:
            return r_sym_new("code");
        case GUMBO_TAG_VAR:
            return r_sym_new("var");
        case GUMBO_TAG_SAMP:
            return r_sym_new("samp");
        case GUMBO_TAG_KBD:
            return r_sym_new("kbd");
        case GUMBO_TAG_SUB:
            return r_sym_new("sub");
        case GUMBO_TAG_SUP:
            return r_sym_new("sup");
        case GUMBO_TAG_I:
            return r_sym_new("i");
        case GUMBO_TAG_B:
            return r_sym_new("b");
        case GUMBO_TAG_MARK:
            return r_sym_new("mark");
        case GUMBO_TAG_RUBY:
            return r_sym_new("ruby");
        case GUMBO_TAG_RT:
            return r_sym_new("rt");
        case GUMBO_TAG_RP:
            return r_sym_new("rp");
        case GUMBO_TAG_BDI:
            return r_sym_new("bdi");
        case GUMBO_TAG_BDO:
            return r_sym_new("bdo");
        case GUMBO_TAG_SPAN:
            return r_sym_new("span");
        case GUMBO_TAG_BR:
            return r_sym_new("br");
        case GUMBO_TAG_WBR:
            return r_sym_new("wbr");
        case GUMBO_TAG_INS:
            return r_sym_new("ins");
        case GUMBO_TAG_DEL:
            return r_sym_new("del");
        case GUMBO_TAG_IMAGE:
            return r_sym_new("image");
        case GUMBO_TAG_IMG:
            return r_sym_new("img");
        case GUMBO_TAG_IFRAME:
            return r_sym_new("iframe");
        case GUMBO_TAG_EMBED:
            return r_sym_new("embed");
        case GUMBO_TAG_OBJECT:
            return r_sym_new("object");
        case GUMBO_TAG_PARAM:
            return r_sym_new("param");
        case GUMBO_TAG_VIDEO:
            return r_sym_new("video");
        case GUMBO_TAG_AUDIO:
            return r_sym_new("audio");
        case GUMBO_TAG_SOURCE:
            return r_sym_new("source");
        case GUMBO_TAG_TRACK:
            return r_sym_new("track");
        case GUMBO_TAG_CANVAS:
            return r_sym_new("canvas");
        case GUMBO_TAG_MAP:
            return r_sym_new("map");
        case GUMBO_TAG_AREA:
            return r_sym_new("area");
        case GUMBO_TAG_MATH:
            return r_sym_new("math");
        case GUMBO_TAG_MI:
            return r_sym_new("mi");
        case GUMBO_TAG_MO:
            return r_sym_new("mo");
        case GUMBO_TAG_MN:
            return r_sym_new("mn");
        case GUMBO_TAG_MS:
            return r_sym_new("ms");
        case GUMBO_TAG_MTEXT:
            return r_sym_new("mtext");
        case GUMBO_TAG_MGLYPH:
            return r_sym_new("mglyph");
        case GUMBO_TAG_MALIGNMARK:
            return r_sym_new("malignmark");
        case GUMBO_TAG_ANNOTATION_XML:
            return r_sym_new("annotation_xml");
        case GUMBO_TAG_SVG:
            return r_sym_new("svg");
        case GUMBO_TAG_FOREIGNOBJECT:
            return r_sym_new("foreignobject");
        case GUMBO_TAG_DESC:
            return r_sym_new("desc");
        case GUMBO_TAG_TABLE:
            return r_sym_new("table");
        case GUMBO_TAG_CAPTION:
            return r_sym_new("caption");
        case GUMBO_TAG_COLGROUP:
            return r_sym_new("colgroup");
        case GUMBO_TAG_COL:
            return r_sym_new("col");
        case GUMBO_TAG_TBODY:
            return r_sym_new("tbody");
        case GUMBO_TAG_THEAD:
            return r_sym_new("thead");
        case GUMBO_TAG_TFOOT:
            return r_sym_new("tfoot");
        case GUMBO_TAG_TR:
            return r_sym_new("tr");
        case GUMBO_TAG_TD:
            return r_sym_new("td");
        case GUMBO_TAG_TH:
            return r_sym_new("th");
        case GUMBO_TAG_FORM:
            return r_sym_new("form");
        case GUMBO_TAG_FIELDSET:
            return r_sym_new("fieldset");
        case GUMBO_TAG_LEGEND:
            return r_sym_new("legend");
        case GUMBO_TAG_LABEL:
            return r_sym_new("label");
        case GUMBO_TAG_INPUT:
            return r_sym_new("input");
        case GUMBO_TAG_BUTTON:
            return r_sym_new("button");
        case GUMBO_TAG_SELECT:
            return r_sym_new("select");
        case GUMBO_TAG_DATALIST:
            return r_sym_new("datalist");
        case GUMBO_TAG_OPTGROUP:
            return r_sym_new("optgroup");
        case GUMBO_TAG_OPTION:
            return r_sym_new("option");
        case GUMBO_TAG_TEXTAREA:
            return r_sym_new("textarea");
        case GUMBO_TAG_KEYGEN:
            return r_sym_new("keygen");
        case GUMBO_TAG_OUTPUT:
            return r_sym_new("output");
        case GUMBO_TAG_PROGRESS:
            return r_sym_new("progress");
        case GUMBO_TAG_METER:
            return r_sym_new("meter");
        case GUMBO_TAG_DETAILS:
            return r_sym_new("details");
        case GUMBO_TAG_SUMMARY:
            return r_sym_new("summary");
        case GUMBO_TAG_COMMAND:
            return r_sym_new("command");
        case GUMBO_TAG_MENU:
            return r_sym_new("menu");
        case GUMBO_TAG_APPLET:
            return r_sym_new("applet");
        case GUMBO_TAG_ACRONYM:
            return r_sym_new("acronym");
        case GUMBO_TAG_BGSOUND:
            return r_sym_new("bgsound");
        case GUMBO_TAG_DIR:
            return r_sym_new("dir");
        case GUMBO_TAG_FRAME:
            return r_sym_new("frame");
        case GUMBO_TAG_FRAMESET:
            return r_sym_new("frameset");
        case GUMBO_TAG_NOFRAMES:
            return r_sym_new("noframes");
        case GUMBO_TAG_ISINDEX:
            return r_sym_new("isindex");
        case GUMBO_TAG_LISTING:
            return r_sym_new("listing");
        case GUMBO_TAG_XMP:
            return r_sym_new("xmp");
        case GUMBO_TAG_NEXTID:
            return r_sym_new("nextid");
        case GUMBO_TAG_NOEMBED:
            return r_sym_new("noembed");
        case GUMBO_TAG_PLAINTEXT:
            return r_sym_new("plaintext");
        case GUMBO_TAG_RB:
            return r_sym_new("rb");
        case GUMBO_TAG_STRIKE:
            return r_sym_new("strike");
        case GUMBO_TAG_BASEFONT:
            return r_sym_new("basefont");
        case GUMBO_TAG_BIG:
            return r_sym_new("big");
        case GUMBO_TAG_BLINK:
            return r_sym_new("blink");
        case GUMBO_TAG_CENTER:
            return r_sym_new("center");
        case GUMBO_TAG_FONT:
            return r_sym_new("font");
        case GUMBO_TAG_MARQUEE:
            return r_sym_new("marquee");
        case GUMBO_TAG_MULTICOL:
            return r_sym_new("multicol");
        case GUMBO_TAG_NOBR:
            return r_sym_new("nobr");
        case GUMBO_TAG_SPACER:
            return r_sym_new("spacer");
        case GUMBO_TAG_TT:
            return r_sym_new("tt");
        case GUMBO_TAG_U:
            return r_sym_new("u");
        case GUMBO_TAG_UNKNOWN:
            return r_sym_new("unknown");

        default:
            rb_raise(rb_eArgError, "unknown tag %d", tag);
    }
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
    } else {
        //rb_raise(rb_eArgError, "unknown node type %d", node->type);
        return Qnil;
    }

    r_node = rb_class_new_instance(0, NULL, class);
    rb_iv_set(r_node, "@type", r_gumbo_node_type_to_symbol(node->type));

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
                  r_tainted_str_new(element->original_tag.data,
                                    element->original_tag.length));
        rb_iv_set(r_node, "@tag_namespace",
                  r_gumbo_namespace_to_symbol(element->tag_namespace));

        r_attributes = rb_ary_new2(element->attributes.length);
        rb_iv_set(r_node, "@attributes", r_attributes);

        for (unsigned int i = 0; i < element->attributes.length; i++) {
            GumboAttribute *attribute;
            VALUE r_attribute;

            attribute = element->attributes.data[i];
            r_attribute = r_gumbo_attribute_to_value(attribute);

            rb_ary_store(r_attributes, i, r_attribute);
        }
    }

    if (children && children->length > 0) {
        VALUE r_children;

        r_children = rb_ary_new2(children->length);
        rb_iv_set(r_node, "@children", r_children);

        for (unsigned int i = 0; i < children->length; i++) {
            GumboNode *child;
            VALUE r_child;

            child = children->data[i];
            r_child = r_gumbo_node_to_value(child);

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
              r_tainted_str_new(attribute->original_name.data,
                                attribute->original_name.length));
    rb_iv_set(r_attribute, "@value", r_tainted_cstr_new(attribute->value));
    rb_iv_set(r_attribute, "@original_value",
              r_tainted_str_new(attribute->original_value.data,
                                attribute->original_value.length));

    return r_attribute;
}
