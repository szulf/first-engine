#ifndef GAME_PARSER_H
#define GAME_PARSER_H

#include <string_view>
#include <expected>

#include "base/base.h"
#include "base/errors.h"

struct Parser_Pos
{
  std::string_view line;
  usize pos;
};

void parser_skip_whitespace(Parser_Pos& pos);
std::expected<void, Error> parser_expect_and_skip(Parser_Pos& pos, char c);
std::expected<void, Error> parser_expect_and_skip(Parser_Pos& pos, std::string_view c);
std::string_view parser_word(Parser_Pos& pos);
std::expected<f32, Error> parser_number_f32(Parser_Pos& pos);
std::expected<u32, Error> parser_number_u32(Parser_Pos& pos);
std::expected<i32, Error> parser_number_i32(Parser_Pos& pos);
std::expected<bool, Error> parser_boolean(Parser_Pos& pos);

inline char parser_curr_char(Parser_Pos& pos)
{
  return pos.line[pos.pos];
}

inline bool parser_size_ok(Parser_Pos& pos)
{
  return pos.pos < pos.line.size();
}

#endif
