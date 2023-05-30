#include <iostream>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <cassert>
#include <SDL.h>
#include <SDL_ttf.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef float f32;
typedef double f64;

#include "Colours.h"

#define WIDTH 10
#define HEIGHT 24
#define V_HEIGHT 20
#define GRID_SIZE 30
#define ARRAY_COUNT(x) (sizeof(x) / sizeof((x)[0]))

const u8 FRAMES_PER_CELL[] = {
	48,
	43,
	38,
	33,
	28,
	23,
	18,
	13,
	8,
	6,
	5,
	5,
	5,
	4,
	4,
	4,
	3,
	3,
	3,
	2,
	2,
	2,
	2,
	2,
	2,
	2,
	2,
	2,
	2,
	1
};

const f32 SECONDS_PER_FRAME = 1.f / 60.f;

struct Tetrad
{
	const u8 *data;
	const s32 side;
};

inline Tetrad
tetrad(const u8 *data, s32 side)
{
	return { data, side };
}

const u8 TETRAD_1[] = {
	0, 0, 0, 0,
	1, 1, 1, 1,
	0, 0, 0, 0,
	0, 0, 0, 0,
};

const u8 TETRAD_2[] = {
	2, 2,
	2, 2,
};

const u8 TETRAD_3[] = {
	0, 0, 0,
	3, 3, 3,
	0, 3, 0,
};

const u8 TETRAD_4[] = {
	0, 0, 0,
	0, 4, 4,
	4, 4, 0,
};

const u8 TETRAD_5[] = {
	0, 0, 0,
	5, 5, 0,
	0, 5, 5,
};


const u8 TETRAD_6[] = {
	0, 0, 0,
	6, 0, 0,
	6, 6, 6,
};

const u8 TETRAD_7[] = {
	0, 0, 0,
	0, 0, 7,
	7, 7, 7,
};


const Tetrad TETRADS[] = {
	tetrad(TETRAD_1, 4),
	tetrad(TETRAD_2, 2),
	tetrad(TETRAD_3, 3),
	tetrad(TETRAD_4, 3),
	tetrad(TETRAD_5, 3),
	tetrad(TETRAD_6, 3),
	tetrad(TETRAD_7, 3),
};

enum Game_Phase
{
	GAME_PHASE_PLAY,
	GAME_PHASE_ROW,
	GAME_PHASE_LOSE,
};

struct Block_State
{
	u8 tetrad_index;
	s32 current_row;
	s32 current_col;
	s32 rotation;
};

struct Input_State
{
	u8 left;
	u8 right;
	u8 up;
	u8 down;
	u8 space;

	s8 dleft;
	s8 dright;
	s8 dup;
	s8 ddown;
	s8 dspace;
};

struct Game_State
{
	u8 playarea[WIDTH * HEIGHT];
	u8 rows[HEIGHT];
	s32 cleared_rows;
	Block_State block;

	Game_Phase phase;
	s32 level;
	s32 start_level;
	s32 row_count;
	s32 points;

	f32 drop_time;
	f32 clear_time;
	f32 time;
};

enum Text
{
	TEXT_LEFT,
	TEXT_CENTER,
	TEXT_RIGHT,
};

inline u8
matrix_get(const u8 *values, s32 width, s32 row, s32 col)
{
	s32 index = width * row + col;
	return values[index];
}

inline void
matrix_set(u8 *values, s32 width, s32 row, s32 col, u8 value)
{
	s32 index = width * row + col;
	values[index] = value;
}

inline u8
tetrad_get(const Tetrad* tetrad, s32 row, s32 col, s32 rotation)
{
	s32 side = tetrad->side;
	switch (rotation)
	{
	case 0:
		return tetrad->data[row * side + col];
	case 1:
		return tetrad->data[(side - col - 1) * side + row];
	case 2:
		return tetrad->data[(side - row - 1) * side + (side - col - 1)];
	case 3:
		return tetrad->data[col * side + (side - row - 1)];
	}
	return 0;
}

inline u8
check_row_complete(const u8 *values, s32 width, s32 row)
{
	for (s32 col = 0;
		col < width;
		++col)
	{
		if (!matrix_get(values, width, row, col))
		{
			return 0;
		}
	}
	return 1;
}

inline  u8
check_row_lose(const u8 *values, s32 width, s32 row)
{
	for (s32 col = 0;
		col < width;
		++col)
	{
		if (matrix_get(values, width, row, col))
		{
			return 0;
		}
	}
	return 1;
}

s32 find_rows(const u8* values, s32 width, s32 height, u8 *rows_full)
{
	s32 count = 0;
	for (s32 row = 0;
		row < height;
		++row)
	{
		u8 complete = check_row_complete(values, width, row);
		rows_full[row] = complete;
		count += complete;
	}
	return count;
}

void 
clear_rows(u8* values, s32 width, s32 height, const u8 *rows)
{
	s32 src_row = height - 1;
	for (s32 dst_row = height - 1;
		dst_row >= 0;
		--dst_row)
	{
		while (src_row > 0 && rows[src_row])
		{
			--src_row;
		}
		if (src_row < 0) {
			memset(values + dst_row * width, 0, width);
		}
		else
		{
			memcpy(values + dst_row * width, values + src_row * width, width);
			--src_row;
		}
	}
}

bool check_block_valid(const Block_State* block,
					   const u8* playarea, s32 width, s32 height)
{
	const Tetrad *tetrad = TETRADS + block->tetrad_index;
	assert(tetrad);

	for (s32 row = 0;
		row < tetrad->side;
		++row)
	{
		for (s32 col = 0;
			col < tetrad->side;
			++col)
		{
			u8 value = tetrad_get(tetrad, row, col, block->rotation);
			{
				if (value > 0)
				{
					s32 grid_row = block->current_row + row;
					s32 grid_col = block->current_col + col;
					if (grid_row < 0)
					{
						return false;
					}
					if (grid_row >= height)
					{
						return false;
					}
					if (grid_col < 0)
					{
						return false;
					}
					if (grid_col >= width)
					{
						return false;
					}
					if (matrix_get(playarea, width, grid_row, grid_col))
					{
						return false;
					}
				}
			}
		}
	}
	return true;
}

void combine_block(Game_State* game)
{
	const Tetrad* tetrad = TETRADS + game->block.tetrad_index;
	for (s32 row = 0;
		row < tetrad->side;
		++row)
	{
		for (s32 col = 0;
			col < tetrad->side;
			++col)
		{
			u8 value = tetrad_get(tetrad, row, col, game->block.rotation);
			if (value) {
				s32 playarea_row = game->block.current_row + row;
				s32 playarea_col = game->block.current_col + col;

				matrix_set(game->playarea, WIDTH, playarea_row, playarea_col, value);
			}
		}
	}
}

inline s32
random_block(s32 minimum, s32 maximum)
{
	s32 range = maximum - minimum;
	return minimum + rand() % range;
}

void spawn_block(Game_State *game)
{
	game->block = {};
	game->block.tetrad_index = random_block(0, ARRAY_COUNT(TETRADS));
	game->block.current_col = WIDTH / 2;
}

inline f32
get_time_drop(s32 level)
{
	if (level > 29)
	{
		level = 29;
	}
	return FRAMES_PER_CELL[level] * SECONDS_PER_FRAME;
}

inline bool
soft_drop(Game_State *game)
{
	++game->block.current_row;
	if (!check_block_valid(&game->block, game->playarea, WIDTH, HEIGHT))
	{
		--game->block.current_row;
		combine_block(game);
		spawn_block(game);
		return false;
	}

	game->drop_time = game->time + get_time_drop(game->level);
	return true;
}

inline s32
scoring_system(s32 level, s32 row_count)
{
	switch (row_count)
	{
	case 1:
		return 40 * (level + 1);
	case 2:
		return 100 * (level + 1);
	case 3:
		return 300 * (level + 1);
	case 4:
		return 1200 + (level + 1);
	}
	return 0;
}

inline s32
minimum(s32 x, s32 y)
{
	return x < y ? x : y;
}
inline s32
maximum(s32 x, s32 y)
{
	return x > y ? x : y;
}

inline s32
get_rows_next_level(s32 start_level, s32 level)
{
	s32 level_up_limit = minimum(
		(start_level * 10 + 10),
		maximum(100, (start_level * 10 - 50)));
	if (level == start_level)
	{
		return level_up_limit;
	}
	s32 delta = level - start_level;
	return level_up_limit + delta * 10;
}

void update_rows(Game_State  *game)
{
	if (game->time >= game->clear_time)
	{
		clear_rows(game->playarea, WIDTH, HEIGHT, game->rows);
		game->row_count += game->cleared_rows;
		game->points += scoring_system(game->level, game->cleared_rows);

		s32 next_level = get_rows_next_level(game->start_level,
											 game-> level);

		if (game->row_count >= next_level) {
			++game->level;
		}
		game->phase = GAME_PHASE_PLAY;
	}
}

void update_gameplay(Game_State *game, const Input_State *input)
{
	Block_State block = game->block;
	if (input->dleft > 0)
	{
		--block.current_col;
	}
	if (input->dright > 0)
	{
		++block.current_col;
	}
	if (input->dup > 0)
	{
		block.rotation = (block.rotation + 1) % 4;
	}
	if (check_block_valid(&block, game->playarea, WIDTH, HEIGHT))
	{
		game->block = block;
	}
	if (input->ddown > 0)
	{
		soft_drop(game);
	}
	if (input->dspace > 0)
	{
		while (soft_drop(game));
	}
	while (game->time >= game->drop_time)
	{
		soft_drop(game);
	}
	game->cleared_rows = find_rows(game->playarea, WIDTH, HEIGHT, game->rows);
	if (game->cleared_rows > 0)
		{
		game->phase = GAME_PHASE_ROW;
		game->clear_time = game->time + 0.2f;
		}
	s32 lose_row = 0;
	if (!check_row_lose(game->playarea, WIDTH, lose_row)) 
	{
		game->phase = GAME_PHASE_LOSE;
	}
}

void update_game(Game_State *game, const Input_State *input)
{
	switch (game->phase)
	{
	case GAME_PHASE_PLAY:
		return update_gameplay(game, input);
		break;
	case GAME_PHASE_ROW:
		update_rows(game);
		break;
	}
}

void fill_rect(SDL_Renderer *renderer, s32 x,
			   s32 y, s32 width, s32 height, Colour colour)
{
	SDL_Rect rect = {};
	rect.x = x;
	rect.y = y;
	rect.w = width;
	rect.h = height;
	SDL_SetRenderDrawColor(renderer, colour.r, colour.g, colour.b, colour.a);
	SDL_RenderFillRect(renderer, &rect);
}

void draw_rect(SDL_Renderer* renderer, s32 x,
	s32 y, s32 width, s32 height, Colour colour)
{
	SDL_Rect rect = {};
	rect.x = x;
	rect.y = y;
	rect.w = width;
	rect.h = height;
	SDL_SetRenderDrawColor(renderer, colour.r, colour.g, colour.b, colour.a);
	SDL_RenderDrawRect(renderer, &rect);
}

void draw_string(SDL_Renderer* renderer, TTF_Font* font, const char* text,
				 s32 x, s32 y, Text postition, Colour colour)
{
	SDL_Color text_colour = SDL_Color{ colour.r, colour.g, colour.b, colour.a };
	SDL_Surface *surface = TTF_RenderText_Solid(font, text, text_colour);
	SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);

	SDL_Rect rect;
	rect.y = y;
	rect.w = surface->w;
	rect.h = surface->h;
	switch (postition)
	{
	case TEXT_LEFT:
		rect.x = x;
		break;
	case TEXT_CENTER:
		rect.x = x - surface->w / 2;
		break;
	case TEXT_RIGHT:
		rect.x = x - surface->w;
		break;
	}

	SDL_RenderCopy(renderer, texture, 0, &rect);
	SDL_FreeSurface(surface);
	SDL_DestroyTexture(texture);

}

void draw_cell(SDL_Renderer *renderer,
			   s32 row, s32 col, u8 value,
			   s32 offset_x, s32 offset_y)
{
	Colour basic_colour = BASIC_COLOURS[value];
	Colour smooth_colour = SMOOTH_COLOURS[value];
	Colour dark_colour = DARK_COLOURS[value];
	
	s32 x = col * GRID_SIZE + offset_x;
	s32 y = row * GRID_SIZE + offset_y;

	s32 edge = GRID_SIZE / 8;

	fill_rect(renderer, x, y, GRID_SIZE, GRID_SIZE, dark_colour);
	fill_rect(renderer, x + edge, y, GRID_SIZE - edge, GRID_SIZE - edge, smooth_colour);
	fill_rect(renderer, x + edge, y + edge, GRID_SIZE - edge * 2, GRID_SIZE - edge * 2, basic_colour);
}

void draw_block(SDL_Renderer* renderer, const Block_State* block,
				s32 offset_x, s32 offset_y)
{	
	const Tetrad* tetrad = TETRADS + block->tetrad_index;
	for (s32 row = 0;
		row < tetrad->side;
		++row)
	{
		for (s32 col = 0;
			col < tetrad->side;
			++col)
		{
			u8 value = tetrad_get(tetrad, row, col, block->rotation);
			if (value)
			{
				draw_cell(renderer,
					row + block->current_row,
					col + block->current_col,
					value, offset_x, offset_y);
			}
		}
	}
}

void draw_playarea(SDL_Renderer* renderer, const u8* playarea,
				  s32 width, s32 height, s32 offset_x, s32 offset_y)
{
	for (s32 row = 0;
		row < height;
		++row)
	{
		for (s32 col = 0;
			col < width;
			++col)
		{
			u8 value = matrix_get(playarea, width, row, col);
			if (value)
			{
				draw_cell(renderer, row, col, value, offset_x, offset_y);
			}
		}
	}
}

void render_game(const Game_State *game, SDL_Renderer *renderer, TTF_Font *font)
{
	Colour main_colour = colour(0xFF, 0xFF, 0xFF, 0xFF);

	draw_playarea(renderer, game->playarea, WIDTH, HEIGHT, 0, 0);

	if (game->phase == GAME_PHASE_PLAY)
	{
		draw_block(renderer, &game->block, 0, 0);

		Block_State block = game->block;
		while (check_block_valid(&block, game->playarea, WIDTH, HEIGHT))
		{
			block.current_row++;
		}
		--block.current_row;

		draw_block(renderer, &block, 0, true);
	}
	
	if (game->phase == GAME_PHASE_ROW)
	{
		for (s32 row = 0;
			row < HEIGHT;
			++row)
		{
			if (game->rows[row])
			{
				s32 x = 0;
				s32 y = row * GRID_SIZE;
			}
		}
	}
	else if (game->phase == GAME_PHASE_LOSE)
	{
		s32 x = WIDTH * GRID_SIZE / 2;
		s32 y = WIDTH * GRID_SIZE / 2;
		draw_string(renderer, font, "GAME OVER", x, y, TEXT_CENTER, main_colour);
	}

	else if (game->phase == GAME_PHASE_LOSE)
	draw_string(renderer, font, "TETRIS", 0, 0, TEXT_LEFT, main_colour);
}

int main(int argc, char* argv[])
{
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		return 1;
	}
	if (TTF_Init() < 0)
	{
		return 1;
	}

	SDL_Window* window = SDL_CreateWindow(
		"Tetris",
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		400,
		720,
		SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
	SDL_Renderer* renderer = SDL_CreateRenderer(
		window,
		-1,
		SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

	const char* font_name = "FreeSans.ttf";
	TTF_Font* font = TTF_OpenFont(font_name, 22);

	Game_State game = {};
	Input_State input = {};
	spawn_block(&game);

	game.block.tetrad_index = 2;

	bool quit = false;
	while (!quit)
	{
		game.time = SDL_GetTicks() / 1000.0f;

		SDL_Event e;
		while (SDL_PollEvent(&e) != 0)
		{
			if (e.type == SDL_QUIT)
			{
				quit = true;
			}
		}

		s32 numkeys;
		const u8 *key_states = SDL_GetKeyboardState(&numkeys);

		if (key_states[SDL_SCANCODE_ESCAPE])
		{
			quit = true;
		}

		Input_State prior_input = input;
		
		input.left = key_states[SDL_SCANCODE_LEFT];
		input.right = key_states[SDL_SCANCODE_RIGHT];
		input.up = key_states[SDL_SCANCODE_UP];
		input.down = key_states[SDL_SCANCODE_DOWN];
		input.space = key_states[SDL_SCANCODE_SPACE];

		input.dleft = input.left - prior_input.left;
		input.dright = input.right - prior_input.right;
		input.dup = input.up - prior_input.up;
		input.ddown = input.down - prior_input.down;
		input.dspace = input.space - prior_input.space;

		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
		SDL_RenderClear(renderer);

		update_game(&game, &input);
		render_game(&game, renderer, font);

		SDL_RenderPresent(renderer);
	}

	TTF_CloseFont(font);
	SDL_DestroyRenderer(renderer);
	SDL_Quit();

	return 0;
}