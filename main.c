/* =====================================================================================
 * Project: Minesweeper
 * Version: 1.0.2
 * Author: dualizm
 * License: Apache License 2.0
 * Homepage: https://github.com/dualizm/minesweeper.git
 ===================================================================================== */

#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

/* TODO
   - Сделать более приятный внешний вид
   - Сделать музыку?
   - Добавить статистику игры?
     -> Добавить сохранения в файл с пользователями и статистикой
   - Портировать под android
   - Портировать под linux
   - Портировать под windows
   - Портировать под macos
 */

#define DISPLAY_ERROR_SDL(INFO_TEXT)				\
  SDL_Log("%s! SDL_Error: %s\n", INFO_TEXT, SDL_GetError());

#define DISPLAY_ERROR_TTF(INFO_TEXT)				\
  SDL_Log("%s! SDL_Error: %s\n", INFO_TEXT, TTF_GetError());

#define BACKGROUND_COLOR 0x38, 0x38, 0x38
#define MAX_FIELD_WIDTH 16
#define MAX_FIELD_HEIGHT 30
#define BUTTON_SIZE 50
#define CELL_SIZE 24

#define LONG_PRESS_DURATION 200u // 0.2
#define ANIMATION_DURATION 88u

typedef enum {
  GAME_STATUS_LOSE,
  GAME_STATUS_START,
  GAME_STATUS_PROGRESS,
  GAME_STATUS_WIN,
} GameStatus;

typedef enum {
  CELL_BACKGROND_OPEN,
  CELL_BACKGROND_CLOSE,
  CELL_BACKGROND_DEAD,
} CellBackgroundState;

typedef enum {
  CELL_FOREGROUND_NONE,
  CELL_FOREGROUND_ONE,
  CELL_FOREGROUND_TWO,
  CELL_FOREGROUND_THREE,
  CELL_FOREGROUND_FOUR,
  CELL_FOREGROUND_FIVE,
  CELL_FOREGROUND_SIX,
  CELL_FOREGROUND_SEVEN,
  CELL_FOREGROUND_EIGHT,
  CELL_FOREGROUND_MINE,
  CELL_FOREGROUND_FLAG
} CellForegroundState;

typedef enum {
  GAME_MODE_BEGGINER,
  GAME_MODE_INTERMEDIATE,
  GAME_MODE_EXPERT
} GameMode;

// begginer - w = 550, h = 300
// intermediate - w = 550, h = 500
// expert - w = 550, h = 800
static Uint16 g_screen_width = 550;
static Uint16 g_screen_height = 300;
static Uint16 g_fps = 60;
static bool g_game_loop = false;
static GameStatus g_game_status = GAME_STATUS_START;
static GameMode g_game_mode = GAME_MODE_BEGGINER;
static SDL_Window *g_window = NULL;
static SDL_Renderer *g_renderer = NULL;
static TTF_Font *g_font = NULL;

static SDL_Texture *g_texture_button_status_mode = NULL;
static SDL_Texture *g_texture_button_game_mode = NULL;
static SDL_Texture *g_texture_cell_front = NULL;
static SDL_Texture *g_texture_cell_back = NULL;
static SDL_Texture *g_texture_timer = NULL;

#ifndef __EMSCRIPTEN__
static SDL_Texture *g_texture_quit = NULL;
#endif

SDL_Surface *load_surface(const char *path) {
  SDL_Surface *surf = SDL_LoadBMP(path);
  if (!surf) {
    SDL_Log("Unable to load image %s! SDL_Error: %s\n", path, SDL_GetError());
  }

  return surf;
}

SDL_Texture *load_texture(const char *path) {
  SDL_Surface *surf = load_surface(path);
  SDL_Texture *texture = SDL_CreateTextureFromSurface(g_renderer, surf);

  if (!texture) {
    SDL_Log("Unable to create texture from %s! SDL_Error: %s\n", path, SDL_GetError());
  }

  SDL_FreeSurface(surf);

  return texture;
}

TTF_Font *load_font(const char *path) {
  TTF_Font *font = TTF_OpenFont(path, 28);
  if (!font) {
    SDL_Log("Unable to load font from %s! SDL_Error: %s\n", path, SDL_GetError());
  }
  return font;
}

SDL_Texture *renderText(const char *message, TTF_Font *font, SDL_Color color,
                        SDL_Renderer *renderer) {
  SDL_Surface *surface = TTF_RenderText_Blended(font, message, color);
  if (!surface) {
    DISPLAY_ERROR_TTF("TTF_RenderText_Blended renderText error");
  }

  SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
  if (!texture) {
    DISPLAY_ERROR_SDL("SDL_CreateTextureFromSurface renderText error");
  }

  SDL_FreeSurface(surface);
  return texture;
}

typedef struct {
  SDL_Point position;
  Uint8 number;
  CellBackgroundState background;
  CellForegroundState foreground;
  bool is_mine;
  bool is_pressed;
  bool is_holding_press;
} Cell;

static inline void cell_init(Cell *cell, int x, int y) {
  cell->position = (SDL_Point){x, y};
  cell->number = 0;
  cell->background = CELL_BACKGROND_CLOSE;
  cell->foreground = CELL_FOREGROUND_NONE;
  cell->is_mine = false;
  cell->is_pressed = false;
  cell->is_holding_press = false;
}

static inline bool cell_is_close(Cell *cell);

void cell_draw(Cell *cell) {
  SDL_Rect dst_rect = {cell->position.x, cell->position.y, CELL_SIZE, CELL_SIZE};

  CellBackgroundState draw_background = cell->background;
  bool draw_as_pressed = false;

  if (cell_is_close(cell) && cell->is_pressed) {
    draw_background = CELL_BACKGROND_OPEN;
    draw_as_pressed = true;
  }

  SDL_Rect back_src_rect = {draw_background * CELL_SIZE, 0, CELL_SIZE,
                            CELL_SIZE};

  SDL_RenderCopy(g_renderer, g_texture_cell_back, &back_src_rect, &dst_rect);

  if (!draw_as_pressed || cell->background != CELL_BACKGROND_OPEN) {
    if (cell->is_holding_press) {
      SDL_Rect temp_flag_src_rect = {(CELL_FOREGROUND_FLAG - 1) * CELL_SIZE, 0,
				     CELL_SIZE, CELL_SIZE};
      SDL_SetTextureAlphaMod(g_texture_cell_front, 128); // Полупрозрачный флаг
      SDL_RenderCopy(g_renderer, g_texture_cell_front, &temp_flag_src_rect, &dst_rect);
      SDL_SetTextureAlphaMod(g_texture_cell_front, 255); // Возвращаем прозрачность
    } else if (cell->foreground != CELL_FOREGROUND_NONE) {
      SDL_Rect fore_src_rect = {(cell->foreground - 1) * CELL_SIZE, 0,
                                CELL_SIZE, CELL_SIZE};
      SDL_RenderCopy(g_renderer, g_texture_cell_front, &fore_src_rect, &dst_rect);
    }
  }
}

/*
 * Функции состояния
 */
static inline void cell_to_dead_mine(Cell *cell) {
  cell->background = CELL_BACKGROND_DEAD;
  cell->foreground = CELL_FOREGROUND_MINE;
}

static inline void cell_to_dead_flag(Cell *cell) {
  cell->background = CELL_BACKGROND_DEAD;
  cell->foreground = CELL_FOREGROUND_FLAG;
}

static inline void cell_to_number(Cell *cell) {
  cell->background = CELL_BACKGROND_OPEN;
  cell->foreground = cell->number;
}

static inline void cell_to_open(Cell *cell) {
  cell->background = CELL_BACKGROND_OPEN;
  cell->foreground = CELL_FOREGROUND_NONE;
}

static inline void cell_to_mine(Cell *cell) {
  cell->background = CELL_BACKGROND_CLOSE;
  cell->foreground = CELL_FOREGROUND_MINE;
}

static inline void cell_to_flag(Cell *cell) {
  cell->background = CELL_BACKGROND_CLOSE;
  cell->foreground = CELL_FOREGROUND_FLAG;
}

static inline void cell_to_close(Cell *cell) {
  cell->background = CELL_BACKGROND_CLOSE;
  cell->foreground = CELL_FOREGROUND_NONE;
}

/*
 * Функции предикаты
 */
static inline bool cell_is_number(Cell *cell) { return cell->number != 0; }

static inline bool cell_is_open(Cell *cell) {
  return cell->background != CELL_BACKGROND_CLOSE;
}

static inline bool cell_is_flag(Cell *cell) {
  return cell->background == CELL_BACKGROND_CLOSE &&
         cell->foreground == CELL_FOREGROUND_FLAG;
}

static inline bool cell_is_close(Cell *cell) {
  return cell->background == CELL_BACKGROND_CLOSE &&
         cell->foreground == CELL_FOREGROUND_NONE;
}

static inline bool cell_is_dead_mine(Cell *cell) {
  return cell->background == CELL_BACKGROND_DEAD &&
         cell->foreground == CELL_FOREGROUND_MINE;
}

static inline bool cell_is_empty(Cell *cell) {
  return cell->background == CELL_BACKGROND_CLOSE &&
         cell->foreground == CELL_FOREGROUND_NONE && cell->is_mine == false &&
         cell->number == 0;
}

typedef struct {
  SDL_Point pressed_cell_position;
  Uint32 pressed_start_time;
  bool is_pressed_active;
  bool is_pressed_complete;
  bool left_down;
  bool right_down;
} MouseState;

typedef struct {
  SDL_Point accord_center_position;
  Uint32 accord_animation_start_time;
  bool is_accord_animating;
} FieldState;

typedef struct field {
  MouseState mouse_state;
  FieldState field_state;
  SDL_Rect rect;
  Uint8 total_mines_count;
  Uint8 mines_count;
  Uint8 visited_to_win_count;
  Uint8 visited_cells;
  Uint32 current_game_time;
  Uint32 start_game_time;
  Cell cells[MAX_FIELD_HEIGHT][MAX_FIELD_WIDTH];
} Field;

static inline void field_init_mouse_state(Field *field) {
  field->mouse_state.left_down = false;
  field->mouse_state.right_down = false;
  field->mouse_state.pressed_cell_position = (SDL_Point){0, 0};
  field->mouse_state.is_pressed_active = false;
  field->mouse_state.is_pressed_complete = false;
  field->mouse_state.pressed_start_time = 0;
}

void field_init(Field *field) {
  // game mode field
  switch (g_game_mode) {
  case GAME_MODE_BEGGINER:
    field->rect.w = 9;
    field->rect.h = 9;
    field->total_mines_count = 10;
    break;
  case GAME_MODE_INTERMEDIATE:
    field->rect.w = 16;
    field->rect.h = 16;
    field->total_mines_count = 40;
    break;
  case GAME_MODE_EXPERT:
    field->rect.w = 16;
    field->rect.h = 30;
    field->total_mines_count = 99;
    break;
  }
  field->mines_count = field->total_mines_count;

  field->rect.x = ((g_screen_width - field->rect.w * CELL_SIZE) / 2);
  field->rect.y = ((g_screen_height - field->rect.h * CELL_SIZE) / 2) + 30;
  field->visited_to_win_count =
      field->rect.w * field->rect.h - field->total_mines_count;
  field->visited_cells = 0;
  field->current_game_time = 0;
  field->start_game_time = 0;

  // init cells
  for (int y = 0; y < field->rect.h; y++) {
    for (int x = 0; x < field->rect.w; x++) {
      cell_init(&field->cells[y][x], field->rect.x + x * CELL_SIZE,
		field->rect.y + y * CELL_SIZE);
    }
  }

  field_init_mouse_state(field);
  
  field->field_state.is_accord_animating = false;
  field->field_state.accord_center_position = (SDL_Point){0, 0};
  field->field_state.accord_animation_start_time = 0;
}

static inline bool field_check_borders(Field *field, int x, int y) {
  return x < 0 || x >= field->rect.w ||
         y < 0 || y >= field->rect.h;
}

void field_reset_mouse_state(Field *field) {
  field_init_mouse_state(field);
  
  if (!field->field_state.is_accord_animating) {
    for (int y = 0; y < field->rect.h; y++) {
      for (int x = 0; x < field->rect.w; x++) {
        field->cells[y][x].is_pressed = false;
	field->cells[y][x].is_holding_press = false;
      }
    }
  }
}

static inline void field_restart(Field *field) {
  field_init(field);
  g_game_status = GAME_STATUS_START;
}

void field_init_mines(Field *field, int start_x, int start_y) {
  Uint8 mines_places = 0;

  while (mines_places < field->mines_count) {
    Uint8 x = rand() % field->rect.w;
    Uint8 y = rand() % field->rect.h;

    if (abs(x - start_x) <= 1 && abs(y - start_y) <= 1) {
      continue;
    }

    if (!field->cells[y][x].is_mine) {
      field->cells[y][x].is_mine = true;
      mines_places += 1;
    }
  }
}

Uint8 field_count_mines_around(Field *field, int x, int y) {
  Uint8 count = 0;

  for (int i = y - 1; i < y + 2; i++) {
    for (int j = x - 1; j < x + 2; j++) {
      if (field_check_borders(field, j, i) ||
	  (i == y && j == x)) {
        continue;
      }

      Cell *cell = &field->cells[i][j];
      if (cell->is_mine) {
        count += 1;
      }
    }
  }

  return count;
}

void field_init_numbers(Field *field) {
  for (int y = 0; y < field->rect.h; y++) {
    for (int x = 0; x < field->rect.w; x++) {
      Cell *cell = &field->cells[y][x];
      if (!cell->is_mine) {
        Uint8 mines_count = field_count_mines_around(field, x, y);
	cell->number = mines_count;
      }
    }
  }
}

void field_draw(Field *field) {
  for (int y = 0; y < field->rect.h; y++) {
    for (int x = 0; x < field->rect.w; x++) {
      cell_draw(&field->cells[y][x]);
    }
  }
}

static inline void field_check_win(Field *field) {
  if (field->visited_cells == field->visited_to_win_count) {
    g_game_status = GAME_STATUS_WIN;

    for (int y = 0; y < field->rect.h; y++) {
      for (int x = 0; x < field->rect.w; x++) {
        Cell *cell = &field->cells[y][x];
        if (cell->is_mine && cell_is_close(cell)) {
          cell_to_flag(cell);
          field->mines_count -= 1;
        }
      }
    }
  }
}

static inline void field_show_all_mines(Field *field) {
  for (int y = 0; y < field->rect.h; y++) {
    for (int x = 0; x < field->rect.w; x++) {
      Cell *cell = &field->cells[y][x];
      if (cell->is_mine && !cell_is_flag(cell) && !cell_is_dead_mine(cell)) {
        cell_to_mine(cell);
      }
    }
  }
}

static inline void field_visit_to_number(Field *field, Cell *cell) {
  if (!cell_is_open(cell)) {
    field->visited_cells += 1;
  }
  cell_to_number(cell);
}

static inline void field_visit_to_open(Field *field, Cell *cell) {
  if (!cell_is_open(cell)) {
    field->visited_cells += 1;
  }
  cell_to_open(cell);
}

static inline Uint8 field_count_flags_around(Field *field, int x, int y) {
  Uint8 count = 0;

  for (int i = y - 1; i < y + 2; i++) {
    for (int j = x - 1; j < x + 2; j++) {
      if (field_check_borders(field, j, i) ||
	  (j == x && i == y)) {
        continue;
      }

      Cell *cell = &field->cells[i][j];
      if (cell_is_flag(cell)) {
        count += 1;
      }
    }
  }

  return count;
}

static inline bool field_is_dead_accord(Field *field, int x, int y) {
  for (int i = y - 1; i < y + 2; i++) {
    for (int j = x - 1; j < x + 2; j++) {
      if (field_check_borders(field, j, i) ||
	  (i == y && j == x)) {
        continue;
      }

      Cell *cell = &field->cells[i][j];
      if (cell->is_mine && !cell_is_flag(cell)) {
        return true;
      }
    }
  }

  return false;
}

static inline void field_reveal_around(Field *field, int x, int y) {
  int stack_x[256] = {0};
  int stack_y[256] = {0};
  int stack_top = 0;

  stack_x[stack_top] = x;
  stack_y[stack_top] = y;
  stack_top++;

  while (stack_top > 0) {
    stack_top--;
    int current_x = stack_x[stack_top];
    int current_y = stack_y[stack_top];

    for (int i = current_y - 1; i < current_y + 2; i++) {
      for (int j = current_x - 1; j < current_x + 2; j++) {
        if (field_check_borders(field, j, i)) {
	  // Проверять центральную клетку?
          continue;
        }

        Cell *cell = &field->cells[i][j];

        if (cell_is_open(cell) || cell_is_flag(cell)) {
          continue;
        }

        if (cell_is_number(cell)) {
          field_visit_to_number(field, cell);
        } else if (cell_is_empty(cell)) {
          field_visit_to_open(field, cell);
          stack_x[stack_top] = j;
          stack_y[stack_top] = i;
          stack_top++;
        }
      }
    }
  }
}

void field_open_around(Field *field, int x, int y) {
  bool is_dead = field_is_dead_accord(field, x, y);

  if (is_dead) {
    g_game_status = GAME_STATUS_LOSE;
    field_show_all_mines(field);
  }

  for (int i = y - 1; i < y + 2; i++) {
    for (int j = x - 1; j < x + 2; j++) {
      if (field_check_borders(field, j, i) ||
	  (i == y && j == x)) {
        continue;
      }

      Cell *cell = &field->cells[i][j];

      if (cell_is_flag(cell)) {
        if (is_dead && !cell->is_mine) {
          cell_to_dead_flag(cell);
        }
        continue;
      }

      if (is_dead && cell->is_mine) {
        cell_to_dead_mine(cell);
        continue;
      }

      if (!is_dead) {
        if (cell_is_number(cell)) {
          field_visit_to_number(field, cell);
        } else if (cell_is_close(cell)) {
          field_visit_to_open(field, cell);
          field_reveal_around(field, j, i);
        }

        field_check_win(field);
      }
    }
  }
}

static inline void field_reset_cells_pressed(Field *field) {
  for (int y = 0; y < field->rect.h; y++) {
    for (int x = 0; x < field->rect.w; x++) {
      field->cells[y][x].is_pressed = false;
    }
  }
}

void field_start_accord_animation(Field *field, int x, int y) {
  field_reset_cells_pressed(field);

  // is_pressed для всех клеток вокруг
  for (int i = y - 1; i < y + 2; i++) {
    for (int j = x - 1; j < x + 2; j++) {
      if (field_check_borders(field, j, i) ||
	  (i == y && j == x)) {
        continue;
      }

      Cell *cell = &field->cells[i][j];

      // Нажимаем только закрытые клетки без флагов
      if (!cell_is_open(cell) && !cell_is_flag(cell)) {
        cell->is_pressed = true;
      }
    }
  }
}

void field_complete_accord_animation(Field *field, int x, int y) {
  field_reset_cells_pressed(field);

  Uint8 flags_count = field_count_flags_around(field, x, y);
  Cell *cell = &field->cells[y][x];
  if (cell->number == flags_count) {
    field_open_around(field, x, y);
  }
}

void field_accord(Field *field, int x, int y) {
  Cell *cell = &field->cells[y][x];

  if (cell->number == 0) {
    return;
  }

  Uint8 flags_count = field_count_flags_around(field, x, y);
  if (flags_count > 0 && cell_is_open(cell)) {
    field_start_accord_animation(field, x, y);
    field->field_state.is_accord_animating = true;
    field->field_state.accord_center_position = (SDL_Point){x, y};
    field->field_state.accord_animation_start_time = SDL_GetTicks();
  }
}

void field_complete_pressed_cell(Field *field) {
  Cell *cell =
    &field->cells[field->mouse_state.pressed_cell_position.y]
                 [field->mouse_state.pressed_cell_position.x];
  if (cell_is_close(cell)) {
    cell_to_flag(cell);
  } else if (cell_is_flag(cell)) {
    cell_to_close(cell);
  }
}

void field_update(Field *field) {
  Uint32 current_time = SDL_GetTicks();
  
  if (field->mouse_state.is_pressed_active &&
      !field->mouse_state.is_pressed_complete &&
      g_game_status == GAME_STATUS_PROGRESS) {
    Uint32 elapsed =
      current_time - field->mouse_state.pressed_start_time;
    
    if (elapsed >= LONG_PRESS_DURATION) {
      field->mouse_state.is_pressed_complete = true;
      Cell *cell =
	&field->cells[field->mouse_state.pressed_cell_position.y]
	             [field->mouse_state.pressed_cell_position.x];
      cell->is_holding_press = true;
    }
  } else if (field->field_state.is_accord_animating) {
    Uint32 elapsed =
      current_time - field->field_state.accord_animation_start_time;
    
    if (elapsed >= ANIMATION_DURATION) {
      field_complete_accord_animation(field, field->field_state.accord_center_position.x,
                                      field->field_state.accord_center_position.y);
      field->field_state.is_accord_animating = false;
    }
  }
}

void field_handle_left_click(Field *field, int x, int y) {
  Cell *cell = &field->cells[y][x];
  if (cell_is_flag(cell)) {
    return;
  }

  if (cell->is_mine) {
    cell_to_dead_mine(cell);
    g_game_status = GAME_STATUS_LOSE;
    field_show_all_mines(field);
  } else if (cell_is_number(cell)) {
    Uint8 flags_count = field_count_flags_around(field, x, y);
    if (cell->number == flags_count) {
      field_visit_to_number(field, cell);
      field_accord(field, x, y);
    } else {
      field_accord(field, x, y);
      field_visit_to_number(field, cell);
    }
  } else if (cell_is_close(cell)) {
    field_visit_to_open(field, cell);
    field_reveal_around(field, x, y);
  }

  field_check_win(field);
}

void field_handle_right_click(Field *field, int x, int y) {
  Cell *cell = &field->cells[y][x];
  if (cell_is_close(cell)) {
    if (field->mines_count > 0) {
      cell_to_flag(cell);
      field->mines_count -= 1;
    }
  } else if (cell_is_flag(cell)) {
    cell_to_close(cell);
    field->mines_count += 1;
  }
}

bool field_cell_coords_from_mouse(Field *field, int *cell_x, int *cell_y) {
  int mx = 0;
  int my = 0;
  SDL_GetMouseState(&mx, &my);

  // Проверяю вхождение клика в поле
  if (mx < field->rect.x || mx > field->rect.x + field->rect.w * CELL_SIZE ||
      my < field->rect.y || my > field->rect.y + field->rect.h * CELL_SIZE) {
    return false;
  }

  *cell_x = floor((double)(mx - field->rect.x) / CELL_SIZE);
  *cell_y = floor((double)(my - field->rect.y) / CELL_SIZE);
  return true;
}

void field_handle_event(Field *field, SDL_Event *event) {
  int cx, cy;

  if (event->type == SDL_MOUSEBUTTONDOWN) {
    if (!field_cell_coords_from_mouse(field, &cx, &cy)) {
      return;
    }

    Cell *cell = &field->cells[cy][cx];
    field_reset_mouse_state(field);
    if (event->button.button == SDL_BUTTON_LEFT) {
      field->mouse_state.left_down = true;
      field->mouse_state.pressed_cell_position = (SDL_Point){cx, cy};
      if (cell_is_close(cell) || cell_is_flag(cell)) {
	field->mouse_state.pressed_start_time = SDL_GetTicks();
	field->mouse_state.is_pressed_active = true;
      }
      cell->is_pressed = true;
    } else if (event->button.button == SDL_BUTTON_RIGHT) {
      if (cell_is_close(cell) || cell_is_flag(cell)) {
        field->mouse_state.right_down = true;
        field->mouse_state.pressed_cell_position = (SDL_Point){cx, cy};
        cell->is_pressed = true;
      }
    }
  } else if (event->type == SDL_MOUSEBUTTONUP) {
    if (!field_cell_coords_from_mouse(field, &cx, &cy)) {
      field_reset_mouse_state(field);
      return;
    }

    // Проверяем, отпустили ли на той же клетке
    bool same_cell = cx == field->mouse_state.pressed_cell_position.x &&
                     cy == field->mouse_state.pressed_cell_position.y;

    if (g_game_status == GAME_STATUS_START) {
      if (same_cell && event->button.button == SDL_BUTTON_LEFT) {
        field_init_mines(field, cx, cy);
        field_init_numbers(field);
        g_game_status = GAME_STATUS_PROGRESS;
        field_handle_left_click(field, cx, cy);
        field->start_game_time = SDL_GetTicks();
      }
      field_reset_mouse_state(field);
      return;
    }

    if (g_game_status != GAME_STATUS_PROGRESS) {
      field_reset_mouse_state(field);
      return;
    }

    // Обрабатываем клик только если отпустили на той же клетке
    if (same_cell && event->button.button == SDL_BUTTON_LEFT) {
      // Проверяем не было ли это долгое нажатие для флага
      if (field->mouse_state.is_pressed_complete) {
	field_complete_pressed_cell(field);
	field_reset_mouse_state(field);
	return;
      }
      
      field_handle_left_click(field, cx, cy);
    } else if (event->button.button == SDL_BUTTON_RIGHT) {
      field_handle_right_click(field, cx, cy);
    }

    field_reset_mouse_state(field);
  } else if (event->type == SDL_MOUSEMOTION) {
    // Если кнопка зажата и мышка ушла с клетки, сбрасываем состояние
    if ((field->mouse_state.left_down || field->mouse_state.right_down) &&
        field->mouse_state.pressed_cell_position.x >= 0 &&
        field->mouse_state.pressed_cell_position.y >= 0) {

      int current_x, current_y;
      if (!field_cell_coords_from_mouse(field, &current_x, &current_y) ||
          current_x != field->mouse_state.pressed_cell_position.x ||
          current_y != field->mouse_state.pressed_cell_position.y) {
        field_reset_mouse_state(field);
      }
    }
  }
}

void button_game_status_handle_click(void *data) {
  Field *field = (Field *)data;
  field_restart(field);
}

void button_game_mode_handle_click(void *data) {
  Field *field = (Field *)data;
  switch (g_game_mode) {
  case GAME_MODE_BEGGINER:
    g_game_mode = GAME_MODE_INTERMEDIATE;
    g_screen_width = 550;
    g_screen_height = 500;
    break;
  case GAME_MODE_INTERMEDIATE:
    g_game_mode = GAME_MODE_EXPERT;
    g_screen_width = 550;
    g_screen_height = 800;
    break;
  case GAME_MODE_EXPERT:
    g_game_mode = GAME_MODE_BEGGINER;
    g_screen_width = 550;
    g_screen_height = 300;
    break;
  }
  field_init(field);
  g_game_status = GAME_STATUS_START;

  SDL_SetWindowSize(g_window, g_screen_width, g_screen_height);
}

#ifndef __EMSCRIPTEN__
void button_game_quit_handle_click(void *data) {
  (void)data;
  g_game_loop = false;
}
#endif

typedef struct {
  SDL_Rect rect;
  void (*on_click)(void *data);
  void *click_data;
  bool is_pressed;
  bool is_hover;
} Button;

void button_init(Button *btn, SDL_Rect rect, void (*handler_click)(void *data),
                 void *handler_data) {
  *btn = (Button){
      .rect = rect,
      .on_click = handler_click,
      .click_data = handler_data,
      .is_pressed = false,
      .is_hover = false,
  };
}

static inline void button_draw_hover(SDL_Rect *rect) {
  SDL_SetRenderDrawColor(g_renderer, BACKGROUND_COLOR, 0x50);
  SDL_RenderFillRect(g_renderer, rect);
}

static inline void button_draw_blocked(SDL_Rect *rect) {
  SDL_SetRenderDrawColor(g_renderer, BACKGROUND_COLOR, 0xA0);
  SDL_RenderFillRect(g_renderer, rect);
}

static inline void button_draw_pressed(SDL_Rect *rect) {
  rect->x += 2;
  rect->y += 2;
  rect->w -= 4;
  rect->h -= 4;
}

typedef struct {
  Button button_game_mode;
  Button button_game_status;
#ifndef __EMSCRIPTEN__
  Button button_quit;
#endif
  SDL_Point position;
} GameHeader;

void game_header_init(GameHeader *header, Field *field, int x, int y) {

  header->position = (SDL_Point){x, y - BUTTON_SIZE};

  button_init(&header->button_game_mode,
              (SDL_Rect){.x = header->position.x,
                         .y = header->position.y,
                         .h = BUTTON_SIZE,
                         .w = BUTTON_SIZE},
              button_game_mode_handle_click, field);

  button_init(
      &header->button_game_status,
      (SDL_Rect){.h = BUTTON_SIZE, .w = BUTTON_SIZE},
      button_game_status_handle_click, field);

#ifndef __EMSCRIPTEN__
  button_init(
      &header->button_quit,
      (SDL_Rect){.h = BUTTON_SIZE, .w = BUTTON_SIZE},
      button_game_quit_handle_click, NULL);
#endif
}

void game_header_draw(GameHeader *header, Field *field) {
  // Рисуем кнопку game_mode
  SDL_Rect game_mode_src_rect = {.x = g_game_mode * BUTTON_SIZE,
                                 .y = 0,
                                 .h = BUTTON_SIZE,
                                 .w = BUTTON_SIZE};

  SDL_Rect game_mode_dst_rect = header->button_game_mode.rect;

  if (header->button_game_mode.is_pressed) {
    button_draw_pressed(&game_mode_dst_rect);
  }

  SDL_RenderCopy(g_renderer, g_texture_button_game_mode, &game_mode_src_rect,
                 &game_mode_dst_rect);

  // Отображаем наведение на кнопку
  if (header->button_game_mode.is_hover) {
    button_draw_hover(&game_mode_dst_rect);
  }

  // Отображаем что кнопка заблокирована
  if (g_game_status == GAME_STATUS_PROGRESS) {
    button_draw_blocked(&game_mode_dst_rect);
  }

  // Рисуем флаг для обозначения того сколько осталось флажков
  SDL_Rect flag_icon_dst_rect = {.x = header->button_game_mode.rect.x +
                                      header->button_game_mode.rect.w + 20,
                                 .y = header->button_game_mode.rect.y,
                                 .h = BUTTON_SIZE,
                                 .w = BUTTON_SIZE};

  SDL_Rect flag_icon_src_rect = {(CELL_FOREGROUND_FLAG - 1) * CELL_SIZE, 0,
                                 CELL_SIZE, CELL_SIZE};
  SDL_RenderCopy(g_renderer, g_texture_cell_front, &flag_icon_src_rect,
                 &flag_icon_dst_rect);

  // Рисуем текст для отображения количества флажков
  char count[256] = {0};
  sprintf(count, "%02d", field->mines_count);
  SDL_Texture *count_flags =
      renderText(count, g_font, (SDL_Color){0xdb, 0x0d, 0x20, 0xFF}, g_renderer);
  int count_flags_size = 48;
  SDL_QueryTexture(count_flags, NULL, NULL, &count_flags_size,
                   &count_flags_size);
  SDL_Rect count_flags_dst_rect = {
      flag_icon_dst_rect.x + flag_icon_dst_rect.w + 20,
      flag_icon_dst_rect.y + 8, count_flags_size, count_flags_size};

  SDL_RenderCopy(g_renderer, count_flags, NULL, &count_flags_dst_rect);
  SDL_DestroyTexture(count_flags);

  // Рисуем кнопку статуса игры
  int game_s = 0;
  switch (g_game_status) {
  case GAME_STATUS_START:
  case GAME_STATUS_PROGRESS:
    game_s = 0;
    break;
  case GAME_STATUS_WIN:
    game_s = 3;
    break;
  case GAME_STATUS_LOSE:
    game_s = 2;
    break;
  }

  if (field->mouse_state.left_down || field->mouse_state.right_down) {
    game_s = 1;
  }

  SDL_Rect game_status_src_rect = {.x = game_s * BUTTON_SIZE,
                                   .y = 0,
                                   .h = BUTTON_SIZE,
                                   .w = BUTTON_SIZE};

  header->button_game_status.rect =
      (SDL_Rect){.x = count_flags_dst_rect.x + BUTTON_SIZE + 20,
                 .y = header->button_game_mode.rect.y,
                 .h = BUTTON_SIZE,
                 .w = BUTTON_SIZE};

  SDL_Rect game_status_dst_rect = header->button_game_status.rect;

  if (header->button_game_status.is_pressed) {
    button_draw_pressed(&game_status_dst_rect);
  }

  SDL_RenderCopy(g_renderer, g_texture_button_status_mode, &game_status_src_rect,
                 &game_status_dst_rect);

  // Отображаем наведение на кнопку
  if (header->button_game_status.is_hover) {
    button_draw_hover(&game_status_dst_rect);
  }

  // Рисуем время игры
  SDL_Rect timer_icon_dst_rect = {.x = header->button_game_status.rect.x +
                                       header->button_game_status.rect.w +
                                       BUTTON_SIZE,
                                  .y = header->button_game_status.rect.y,
                                  .w = BUTTON_SIZE,
                                  .h = BUTTON_SIZE};
  SDL_RenderCopy(g_renderer, g_texture_timer, NULL,
                 &timer_icon_dst_rect);

  // Рисуем текст для отображения времени с начала игры до 99
  char times[256] = {0};
  sprintf(times, "%03d", field->current_game_time);

  SDL_Texture *time_text =
      renderText(times, g_font, (SDL_Color){0xdb, 0x0d, 0x20, 0xFF}, g_renderer);
  int time_text_h = 48;
  int time_text_w = 60;
  SDL_QueryTexture(time_text, NULL, NULL, &time_text_h, &time_text_w);
  SDL_Rect text_time_dst_rect = {
      timer_icon_dst_rect.x + timer_icon_dst_rect.w + 20,
      timer_icon_dst_rect.y + 8, time_text_h, time_text_w};

  SDL_RenderCopy(g_renderer, time_text, NULL, &text_time_dst_rect);
  SDL_DestroyTexture(time_text);

#ifndef __EMSCRIPTEN__
  // Рисуем кнопку выхода
  SDL_Rect quit_button_dst_rect = { .x = text_time_dst_rect.x + BUTTON_SIZE + 20,
                                    .y = header->button_game_status.rect.y,
                                    .w = BUTTON_SIZE,
                                    .h = BUTTON_SIZE};
  header->button_quit.rect = quit_button_dst_rect;

  if (header->button_quit.is_pressed) {
    button_draw_pressed(&quit_button_dst_rect);
  }
  
  SDL_RenderCopy(g_renderer, g_texture_quit, NULL, &quit_button_dst_rect);

  if (header->button_quit.is_hover) {
    button_draw_hover(&quit_button_dst_rect);
  }
#endif
}

void button_handle_event(Button *btn, SDL_Event *event) {
  int mx = 0;
  int my = 0;
  SDL_GetMouseState(&mx, &my);

  // Проверяю вхождение клика
  if (mx < btn->rect.x || mx > btn->rect.w + btn->rect.x || my < btn->rect.y ||
      my > btn->rect.y + btn->rect.h) {
    btn->is_hover = false;
    return;
  }

  btn->is_hover = true;

  if (event->type == SDL_MOUSEBUTTONDOWN) {
    if (event->button.button == SDL_BUTTON_LEFT) {
      btn->is_pressed = true;
    }
  } else if (event->type == SDL_MOUSEBUTTONUP) {
    if (event->button.button == SDL_BUTTON_LEFT) {
      btn->on_click(btn->click_data);
      btn->is_pressed = false;
    }
  }
}

static inline bool graphics_init(void) {
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    DISPLAY_ERROR_SDL("sdl could not init");
    return false;
  }

  atexit(SDL_Quit);

  g_window = SDL_CreateWindow("Minesweeper", SDL_WINDOWPOS_UNDEFINED,
                              SDL_WINDOWPOS_UNDEFINED, g_screen_width,
                              g_screen_height, SDL_WINDOW_SHOWN);

  if (!g_window) {
    DISPLAY_ERROR_SDL("window could not be created");
    return false;
  }

  g_renderer = SDL_CreateRenderer(g_window, -1, SDL_RENDERER_PRESENTVSYNC);

  if (!g_renderer) {
    DISPLAY_ERROR_SDL("renderer could not be created");
    SDL_DestroyWindow(g_window);
    return false;
  }

  // Добавляем прозрачность
  SDL_SetRenderDrawBlendMode(g_renderer, SDL_BLENDMODE_BLEND);

  if (!SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1")) {
    SDL_Log("Warning: linear texture filtering not enabled!\n");
  }

  if (TTF_Init() < 0) {
    DISPLAY_ERROR_TTF("TTF could not init");
    return false;
  }

  atexit(TTF_Quit);

  return true;
}

static inline void grahics_free(void) {
  SDL_DestroyRenderer(g_renderer);
  SDL_DestroyWindow(g_window);
}

static inline void resources_init(void) {
  g_texture_cell_back = load_texture("./assets/images/back_cell_2.bmp");
  g_texture_cell_front = load_texture("./assets/images/front_cell_2.bmp");
  g_texture_button_game_mode =
      load_texture("./assets/images/game_mode_button.bmp");
  g_texture_button_status_mode =
      load_texture("./assets/images/game_status_button.bmp");
  g_texture_timer = load_texture("./assets/images/time.bmp");
#ifndef __EMSCRIPTEN__
  g_texture_quit = load_texture("./assets/images/quit.bmp");
#endif
  g_font = load_font("./assets/fonts/PixelifySans-Regular.ttf");
}

static inline void resources_free(void) {
  TTF_CloseFont(g_font);
#ifndef __EMSCRIPTEN__
  SDL_DestroyTexture(g_texture_quit);
#endif
  SDL_DestroyTexture(g_texture_timer);
  SDL_DestroyTexture(g_texture_button_status_mode);
  SDL_DestroyTexture(g_texture_button_game_mode);
  SDL_DestroyTexture(g_texture_cell_front);
  SDL_DestroyTexture(g_texture_cell_back);
}

void handle_events(Field *field, GameHeader *header) {
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    // Обрабатываем глобальные нажатия кнопок
    if (event.type == SDL_QUIT) {
      g_game_loop = false;
    } else if (event.type == SDL_KEYUP) {
      switch (event.key.keysym.sym) {
#ifndef __EMSCRIPTEN__
      case SDLK_q:
        g_game_loop = false;
        break;
#endif
      case SDLK_r:
        field_restart(field);
        break;
      case SDLK_n:
	button_game_mode_handle_click(field);
	break;
      }
    } else {
      if (g_game_status == GAME_STATUS_PROGRESS ||
          g_game_status == GAME_STATUS_START) {
        field_handle_event(field, &event);
      }
      if (g_game_status != GAME_STATUS_PROGRESS) {
        button_handle_event(&header->button_game_mode, &event);
      }
      button_handle_event(&header->button_game_status, &event);
#ifndef __EMSCRIPTEN__
      button_handle_event(&header->button_quit, &event);
#endif
    }
  }
}

typedef struct {
  Field *field;
  GameHeader *header;
  Uint32 frame_delay;
  Uint32 frame_start;
  Uint32 frame_time;
} LoopArgs;

LoopArgs *game_args = NULL;

#ifdef __EMSCRIPTEN__
EMSCRIPTEN_KEEPALIVE
#endif
void game_logic(void) {
  game_args->frame_start = SDL_GetTicks();
  
  handle_events(game_args->field, game_args->header);
  
  field_update(game_args->field);
  
  // Фон
  SDL_SetRenderDrawColor(g_renderer, BACKGROUND_COLOR, 0xFF);
  SDL_RenderClear(g_renderer);
  
  // Поле
  field_draw(game_args->field);
  
  // Шапка
  game_header_draw(game_args->header, game_args->field);
  
  SDL_RenderPresent(g_renderer);
  game_args->frame_time = SDL_GetTicks() - game_args->frame_start;
  if (game_args->frame_delay > game_args->frame_time) {
    SDL_Delay(game_args->frame_delay - game_args->frame_time);
  }
  
  if (g_game_status == GAME_STATUS_PROGRESS &&
      game_args->field->current_game_time < 999) {
    game_args->field->current_game_time =
      (SDL_GetTicks() - game_args->field->start_game_time) / 1000;
  }
}

int main(int argc, const char *argv[]) {
  (void)argc;
  (void)argv;
  
  bool graphics_status = graphics_init();
  if (!graphics_status) {
    return EXIT_FAILURE;
  }
  
  resources_init();
  
  Uint32 const frame_delay = 1000 / g_fps;
  Uint32 frame_start = 0;
  Uint32 frame_time = 0;
  
  Field field;
  GameHeader header;
  game_header_init(&header, &field, 20, 60);
  field_init(&field);
  g_game_loop = true;
  g_game_status = GAME_STATUS_START;
  srand((unsigned int)SDL_GetTicks());
  
  LoopArgs args = {
    .frame_delay = frame_delay,
    .frame_start = frame_start,
    .frame_time = frame_time,
    .field = &field,
    .header = &header
  };
  game_args = &args;
  
#ifdef __EMSCRIPTEN__
  emscripten_set_main_loop(game_logic, -1, 1);
#else
  while (g_game_loop) {
    game_logic();
  }
#endif

  resources_free();
  grahics_free();
  
  return EXIT_SUCCESS;
}
