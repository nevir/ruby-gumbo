# Copyright (c) 2013 Nicolas Martyanoff
#
# Permission to use, copy, modify, and distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
# WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
# ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
# WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
# ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
# OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

require 'gumbo'

class Gumbo::Element
  def to_s
    if original_tag
      open_tag = original_tag
      end_tag  = original_end_tag || ''
    else
      tag_name = original_tag_name || tag
      open_tag = "<#{tag_name}>"
      end_tag  = "</#{tag_name}>"
    end

    open_tag + (children || []).map(&:to_s).join + end_tag
  end
  alias_method :inspect, :to_s

  # The *byte* offset range where this element was extracted from, or nil if it
  # was inserted algorithmically.
  def offset_range
    return nil unless original_tag
    if original_end_tag
      end_offset = end_pos.offset + original_end_tag.bytesize
    else
      end_offset = start_pos.offset + original_tag.bytesize
    end

    start_pos.offset...end_offset
  end

  # The *byte* offset range where the content inside this node exists, or nil if
  # the node was inserted algorithmically, or has no content.
  def content_range
    return nil unless original_tag && original_end_tag

    (start_pos.offset + original_tag.bytesize)...end_pos.offset
  end
end
