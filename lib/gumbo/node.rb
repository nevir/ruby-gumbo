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

module Gumbo
  class Node
    # Recursively dump an indented representation of a HTML tree to +output+.
    # Text nodes are not printed.
    def dump_tree(output = $stdout)
      process_node = lambda do |node, indent|
        return unless node.type == :document || node.type == :element

        output.write (" " * indent)

        if node.type == :element
          tag = (node.tag == :unknown) ? node.original_tag_name : node.tag.to_s
          attributes = node.attributes.map(&:name)
          output.write "<" + tag.upcase()
          output.write(" " + attributes.join(" ")) unless attributes.empty?
          output.puts ">"

          indent += 2
        end

        for child in node.children
          process_node.call(child, indent)
        end
      end

      process_node.call(self, 0)
    end
  end
end
