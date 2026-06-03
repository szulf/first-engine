#include "parser.h"

#include <cctype>
#include <format>
#include <charconv>

void parser_skip_whitespace(Parser_Pos& pos)
{
  while (parser_size_ok(pos) && std::isspace(parser_curr_char(pos)))
  {
    ++pos.pos;
  }
}

void parser_expect_and_skip(Parser_Pos& pos, char c)
{
  ASSERT(parser_curr_char(pos) == c, "Expected '{}', found '{}'.", c, parser_curr_char(pos));
  ++pos.pos;
  parser_skip_whitespace(pos);
}

void parser_expect_and_skip(Parser_Pos& pos, std::string_view c)
{
  for (usize i = 0; i < c.size(); ++i)
  {
    parser_expect_and_skip(pos, c[i]);
  }
}

std::string_view parser_word(Parser_Pos& pos)
{
  parser_skip_whitespace(pos);
  usize word_length{};
  while (parser_size_ok(pos) &&
         (std::isalnum(parser_curr_char(pos)) || std::ispunct(parser_curr_char(pos))))
  {
    ++word_length;
    ++pos.pos;
  }

  auto word = pos.line.substr(pos.pos - word_length, word_length);
  parser_skip_whitespace(pos);
  return word;
}

f32 parser_number_f32(Parser_Pos& pos)
{
  parser_skip_whitespace(pos);
  usize num_length{};
  while (parser_size_ok(pos) &&
         (std::isdigit(parser_curr_char(pos)) || parser_curr_char(pos) == '.' ||
          parser_curr_char(pos) == 'e' || parser_curr_char(pos) == 'E' ||
          parser_curr_char(pos) == '+' || parser_curr_char(pos) == '-'))
  {
    ++num_length;
    ++pos.pos;
  }

  f32 result{};
  auto ec =
    std::from_chars(pos.line.data() + (pos.pos - num_length), pos.line.data() + pos.pos, result).ec;
  ASSERT(ec == std::errc{}, "Invalid f32.");
  parser_skip_whitespace(pos);
  return result;
}

u32 parser_number_u32(Parser_Pos& pos)
{
  parser_skip_whitespace(pos);
  usize num_length{};
  while (parser_size_ok(pos) && std::isdigit(parser_curr_char(pos)))
  {
    ++num_length;
    ++pos.pos;
  }

  u32 result{};
  auto ec =
    std::from_chars(pos.line.data() + (pos.pos - num_length), pos.line.data() + pos.pos, result).ec;
  ASSERT(ec == std::errc{}, "Invalid u32.");
  parser_skip_whitespace(pos);
  return result;
}

i32 parser_number_i32(Parser_Pos& pos)
{
  parser_skip_whitespace(pos);
  usize num_length{};
  while (parser_size_ok(pos) &&
         (std::isdigit(parser_curr_char(pos)) || parser_curr_char(pos) == '-'))
  {
    ++num_length;
    ++pos.pos;
  }

  i32 result{};
  auto ec =
    std::from_chars(pos.line.data() + (pos.pos - num_length), pos.line.data() + pos.pos, result).ec;
  ASSERT(ec == std::errc{}, "Invalid i32.");
  parser_skip_whitespace(pos);
  return result;
}

bool parser_boolean(Parser_Pos& pos)
{
  parser_skip_whitespace(pos);
  if (pos.line.size() - pos.pos >= 4 && pos.line.substr(pos.pos, 4) == "true")
  {
    pos.pos += 4;
    return true;
  }
  else if (pos.line.size() - pos.pos >= 5 && pos.line.substr(pos.pos, 5) == "false")
  {
    pos.pos += 5;
    return false;
  }
  ASSERT(false, "Invalid boolean.");
}
