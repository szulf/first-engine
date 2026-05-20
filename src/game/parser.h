#ifndef GAME_PARSER_H
#define GAME_PARSER_H

#include <string_view>

#include "base/base.h"

struct Parser_Pos
{
  std::string_view line;
  usize pos;
};

void parser_skip_whitespace(Parser_Pos& pos);
void parser_expect_and_skip(Parser_Pos& pos, char c);
std::string_view parser_word(Parser_Pos& pos);
f32 parser_number_f32(Parser_Pos& pos);
u32 parser_number_u32(Parser_Pos& pos);
bool parser_boolean(Parser_Pos& pos);

inline char parser_curr_char(Parser_Pos& pos)
{
  return pos.line[pos.pos];
}

inline bool parser_size_ok(Parser_Pos& pos)
{
  return pos.pos < pos.line.size();
}

#endif
