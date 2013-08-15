#!/usr/bin/env ruby

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


if (ARGV.length < 1)
  puts "Usage: #{$0} <file>"
  exit 1
end

file = ARGV[0]
html = File.read file

Gumbo::parse(html) do |doc|
  root = doc.children[0]

  head = root.children.find do |node|
    node.type == :element && node.tag == :head
  end
  raise "<head> element not found" unless head

  title = head.children.find do |node|
    node.type == :element && node.tag == :title
  end
  raise "<title> element not found" unless title
  raise "empty <title> element" unless title.children.length > 0

  text = title.children[0]
  raise "invalid <title> element" unless text.type == :text

  puts text.text
end
