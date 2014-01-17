////
//// Downtown Cabby
//// (c) 2014 Matchy Games
//// 17-Jan-2014 v1.0
////
//// Description:
//// Game for the Arduino Gamby game shield.
////
//// Requirements:
//// * Arduino Uno & IDE
//// * Gamby shield & libraries
//// * dtc_data.h
////

#include <Gamby.h>
#include "dtc_data.h"

GambyGraphicsMode gamby;
extern prog_int32_t font[];

#define PIN_SPEAKER			9
#define MAP_WIDTH			47
#define MAP_HEIGHT			15
#define TILE_NONE			0
#define TILE_PICKUP			17
#define TILE_DROPOFF		18
#define DEBUG_ON			0
#define SCENE_PLAY			0
#define SCENE_PICKUP1		1
#define SCENE_PICKUP2		2
#define SCENE_PICKUP3		3
#define SCENE_PICKUP4		4
#define SCENE_DROPOFF1		5
#define SCENE_DROPOFF2		6
#define SCENE_DROPOFF3		7
#define SCENE_DROPOFF4		8
#define SCENE_SPLASH		9
#define SCENE_TITLE			10
#define SCENE_MENU			11
#define SCENE_SETTINGS		12
#define SCENE_STORY			13
#define SCENE_ABOUT			14
#define SCENE_HELP			15
#define SCENE_LEVEL			16
#define SCENE_QUIT			17

#define TURN_VACANT			0
#define TURN_PICKUP			1
#define TURN_DROPOFF		2
#define TURN_OFF			3

#define ALIGN_NONE			0
#define ALIGN_LEFT			1
#define ALIGN_CENTER		2
#define ALIGN_RIGHT			3

#define INPUT_BUTTON_N		0
#define INPUT_BUTTON_E		1
#define INPUT_BUTTON_W		2
#define INPUT_BUTTON_S		3
#define INPUT_DPAD_UP		4
#define INPUT_DPAD_DOWN		5
#define INPUT_DPAD_LEFT		6
#define INPUT_DPAD_RIGHT	7

#define MENU_START			0
#define MENU_SETTINGS		1
#define MENU_STORY			2
#define MENU_HELP			3
#define MENU_ABOUT			4
#define MENU_RESUME			5
#define MENU_QUIT			6
#define MENU_YES			7
#define MENU_NO				8

#define SOUND_MUSIC			0
#define SOUND_EFFECTS		1
#define SOUND_OFF			2

#define DIFFICULTY_EASY		0
#define DIFFICULTY_HARD		1
#define DIFFICULTY_EXPERT	2

#define TIMER_NONE			0
#define TIMER_SHOW			1
#define TIMER_ALERT			2
#define TIMER_END			3

int vector_auto[] = {
	-2, -2,   2, -2,   2,  2,  -2,  2,
	-4, -2,  -3, -3,  -1, -3,  -1, -2, 
	 1, -2,   1, -3,   3, -3,   4, -2,
	 4,  2,   3,  3,   1,  3,   1,  2, 
	-1,  2,  -1,  3,  -3,  3,  -4,  2
};

byte data_field[MAP_WIDTH][MAP_HEIGHT];
short dir_x[4];
short dir_y[4];

struct _bot {
	float size;
	int x;
	int y;
	int x0;
	int y0;
	int x1;
	int y1;
	unsigned int frame;
};
_bot bot;
_bot car;
_bot man;

struct _tile {
	int x;
	int y;
	byte width;
	byte height;
	byte width2;
	byte height2;
};
_tile tile;

struct _screen {
	byte width;
	byte height;
	byte width2;
	byte height2;
};
_screen screen;

struct _play {
	float dx;
	float dy;
	float last_dx;
	float last_dy;
	float angle;
	float speed;
};
_play play;

struct _song {
	unsigned short frequency[4];
	byte duration;
	byte delay;
	byte pos;
	byte bar;
	byte pattern;
};
_song song;

struct _score {
	unsigned int balance;
	unsigned int scene_timer;
	unsigned int scene_id;
	unsigned int menu_id;
	unsigned int menu_anim;
	int menu_anim_dir;
	byte quota;
	byte fill;
	byte level;
	byte turn;
	boolean is_yes;
	byte difficulty;
	byte sound;
	byte settings_id;
	byte magnify_size;
	byte map_view;
};
_score score;
byte button_state[5];

struct _timer {
	unsigned long current;
	unsigned long loop;
	byte minute;
	byte second;
	byte state;
	byte start_minute_alt;
	byte start_second_alt;
	byte minute_alt;
	byte second_alt;
};
_timer timer;

void setup_timer() {
	timer.current = millis();
	timer.loop = timer.current;
	timer.minute = 0;
	timer.second = 0;
	timer.start_minute_alt = 0;
	timer.start_second_alt = 59;
}

void update_timer() {
	timer.current = millis();
	if (timer.current < timer.loop + 1000) return;
	timer.loop = timer.current;
	timer.second++;
	if (timer.second > 59) {
		timer.second = 0;
		timer.minute++;
		if (timer.minute > 59) {
			timer.minute = 0;
		}
	}
	timer.second_alt = timer.start_second_alt - timer.second;
	timer.minute_alt = timer.start_minute_alt - timer.minute;
	timer.state = TIMER_NONE;
	if (timer.minute_alt == 0) {
		if (timer.second_alt == 0) {
			timer.state = TIMER_END;
		} else if (timer.second_alt < 11) {
			timer.state = TIMER_ALERT;
		} else if (timer.second_alt < 31) {
			timer.state = TIMER_SHOW;
		}
	}
}

void random_directions() {
	int randdirection;
	randdirection = (int)(random(0,4));
	for (byte i = 0; i < 4; i++) {
		dir_x[i] = 0;
		dir_y[i] = 0;
	}
	if (randdirection == 0) {
		dir_x[0] = -1;
		dir_x[1] = 1;
		dir_y[2] = -1;
		dir_y[3] = 1;
	}
	if (randdirection == 1) {
		dir_x[3] = -1;
		dir_x[2] = 1;
		dir_y[1] = -1;
		dir_y[0] = 1;
	}
	if (randdirection == 2)	{
		dir_x[2] = -1;
		dir_x[3] = 1;
		dir_y[0] = -1;
		dir_y[1] = 1;
	}
	if (randdirection == 3) {
		dir_x[1] = -1;
		dir_x[0] = 1;
		dir_y[3] = -1;
		dir_y[2] = 1;
	}
}

boolean is_free(int xpos, int ypos) {
	if ((xpos < MAP_WIDTH -1) && (xpos >= 1) && (ypos < MAP_HEIGHT -1) && (ypos >= 1)) {
		return !data_field[xpos][ypos];
	} else {
		return false;
	}
}

void generate_maze() {
	randomSeed(analogRead(0));
	int cell_nx = 0;
	int cell_ny = 0;
	int cell_sx = 0;
	int cell_sy = 0;
	int dir = 0;
	int done = 0;
	boolean filled = false;
	boolean blocked = false;
	do {
		do {  
			cell_sx = (int)random(1,(MAP_WIDTH)); 
		} while (cell_sx % 2 == 0); 
		do {
			cell_sy = (int)random(1,(MAP_HEIGHT)); 
		} while (cell_sy % 2 == 0); 
		if (done == 0) data_field[cell_sx][cell_sy] = true;   
		if (data_field[cell_sx][cell_sy]) {
			do {
				random_directions();
				blocked = true;
				filled = false;
				for (dir = 0; (dir < 4); dir++) {
					if (!filled) {
						cell_nx = cell_sx + (dir_x[dir] * 2);
						cell_ny = cell_sy + (dir_y[dir] * 2);
						if (is_free(cell_nx, cell_ny)) {
							data_field[cell_nx][cell_ny]  = true;
							data_field[cell_sx + dir_x[dir]][cell_sy + dir_y[dir]] = true;
							cell_sx = cell_nx;
							cell_sy = cell_ny;
							blocked = false;
							done++;
							filled = true;
						}
					}
				}
			} while(!blocked);
		}
	} while (((done + 1) < (((MAP_WIDTH - 1) * (MAP_HEIGHT - 1)) / 4)));
}

void invert_maze() {
	for (int y = 0; y < MAP_HEIGHT; y++) {
		for (int x = 0; x < MAP_WIDTH; x++) {
			if (data_field[x][y] != 0) {
				data_field[x][y] = 0;
			} else {
				data_field[x][y] = 1;
			}
		}
	}
}

void reset_maze() {
	for (int y = 0; y < MAP_HEIGHT; y++) {
		for (int x = 0; x < MAP_WIDTH; x++) {
			data_field[x][y] = 0;
		}
	}
}

void form_gap() {
	int plot_x, plot_y, block_x, block_y, block_width, block_height;
	block_width = MAP_WIDTH * 0.1;
	block_height = MAP_HEIGHT * 0.2;
	int suburb = score.level * (score.difficulty * 8);
	if (suburb < 0) suburb = 0;
	if (suburb > 58) suburb = 58;
	for (byte i = 0; i < 64 - suburb; i++) {
		block_x = random(MAP_WIDTH - block_width);
		block_y = random(MAP_HEIGHT - block_height);
		for (int plot_y = block_y; plot_y < block_y + block_height; plot_y++) {
			for (int plot_x = block_x; plot_x < block_x + block_width; plot_x++) {
				if (plot_x > 0 && plot_x < MAP_WIDTH - 1 && plot_y > 0 && plot_y < MAP_HEIGHT - 1) {
					data_field[plot_x][plot_y] = 0;
				}
			}
		}
	}
}

void place_pickup() {
	int last_x = bot.x;
	int last_y = bot.y;
	data_field[bot.x][bot.y] = TILE_NONE;
	boolean found_location = false;
	do {
		randomSeed(analogRead(0));
		bot.x = random(MAP_WIDTH - 4) + 2;
		bot.y = random(MAP_HEIGHT - 4) + 2;
		if (data_field[bot.x][bot.y] == 0) {
			if (abs(bot.x - last_x) > 6 && abs(bot.y - last_y) > 3) found_location = true;
		}
	} while (!found_location);
	data_field[bot.x][bot.y] = TILE_PICKUP;
	score.turn = TURN_PICKUP;
	setup_timer();
}

void place_dropoff() {
	int last_x = bot.x;
	int last_y = bot.y;
	data_field[bot.x][bot.y] = TILE_NONE;
	boolean found_location = false;
	do {
		randomSeed(analogRead(0));
		bot.x = random(MAP_WIDTH - 4) + 2;
		bot.y = random(MAP_HEIGHT - 4) + 2;
		if (data_field[bot.x][bot.y] == 0) {
			if (abs(bot.x - last_x) > 6 && abs(bot.y - last_y) > 3) {
				found_location = true;
			}
		}
	} while (!found_location);
	data_field[bot.x][bot.y] = TILE_DROPOFF;
	score.turn = TURN_DROPOFF;
	setup_timer();
}

void make_maze() {
	reset_maze();
	generate_maze();
	invert_maze();
	form_gap();
}

void setup_size() {
	if (score.map_view == 0) {
		score.magnify_size = 16;
	} else {
		score.magnify_size = 2;
	}
	tile.width = score.magnify_size;
	tile.height = score.magnify_size;
	tile.width2 = tile.width / 2;
	tile.height2 = tile.height / 2;
}

void display_scene() {
	boolean done_flag = false;
	float radius;
	switch (score.scene_id) {
		case SCENE_PLAY:
			draw_map();
			if (score.map_view == 0) draw_auto();
			break;
		case SCENE_PICKUP1:
			if (score.scene_timer == 1) {
				score.map_view = 1;
				setup_size();
				car.x = - 48;
			}
			if (car.x % 6 < 3) {
				car.frame = 0;
			} else {
				car.frame = 1;
			}
			gamby.drawSprite(0, 0, sprite_city);
			gamby.drawSprite(car.x, 46, sprite_auto);
			gamby.drawSprite(car.x, 46, sprite_wheel, car.frame, sprite_wheel, car.frame);
			gamby.drawSprite(car.x + 32, 46, sprite_wheel, car.frame, sprite_wheel, car.frame);
			if (car.x < 24) {
				car.x += 3;
			} else {
				car.x += 1;
			}
			done_flag = car.x > screen.width - 53;
			break;
		case SCENE_PICKUP2:
			if (score.scene_timer == 0) man.x = -16;
			gamby.drawSprite(0, 0, sprite_city);
			gamby.drawSprite(car.x, 46, sprite_auto);
			gamby.drawSprite(car.x, 46, sprite_wheel, car.frame, sprite_wheel, car.frame);
			gamby.drawSprite(car.x + 32, 46, sprite_wheel, car.frame, sprite_wheel, car.frame);
			gamby.drawSprite(man.x, 46, sprite_play, man.frame, sprite_play, man.frame);
			man.frame++;
			if (man.frame > 29) man.frame = 0;
			man.x = man.x + 2;
			done_flag = man.x > screen.width - 42;
			break;
		case SCENE_PICKUP3:
			score.map_view = 1;
			draw_map();
			gamby.drawSprite(car.x, 46, sprite_auto);
			gamby.drawSprite(car.x, 46, sprite_wheel, car.frame, sprite_wheel, car.frame);
			gamby.drawSprite(car.x + 32, 46, sprite_wheel, car.frame, sprite_wheel, car.frame);
			gamby.drawSprite(car.x + 14, 46, sprite_passenger, sprite_passenger);
			if (score.scene_timer == 8) place_dropoff();
			done_flag = score.scene_timer > 20;
			break;
		case SCENE_PICKUP4:
			if (car.x % 6 < 3) {
				car.frame = 0;
			} else {
				car.frame = 1;
			}
			gamby.drawSprite(0, 0, sprite_city);
			gamby.drawSprite(car.x, 46, sprite_auto);
			gamby.drawSprite(car.x, 46, sprite_wheel, car.frame, sprite_wheel, car.frame);
			gamby.drawSprite(car.x + 32, 46, sprite_wheel, car.frame, sprite_wheel, car.frame);
			gamby.drawSprite(car.x + 14, 46, sprite_passenger, sprite_passenger);
			if (car.x < 64) {
				car.x += 1;
			} else {
				car.x += 3;
			}
			done_flag = car.x > screen.width + 48;
			break;
		case SCENE_DROPOFF1:
			if (score.scene_timer == 1) {
				score.map_view = 1;
				setup_size();
				car.x = -48;
			}
			if (car.x % 6 < 3) {
				car.frame = 0;
			} else {
				car.frame = 1;
			}
			gamby.drawSprite(0, 0, sprite_city);
			if (car.x > -16) {
				gamby.drawSprite(car.x, 46, sprite_auto);
				gamby.drawSprite(car.x, 46, sprite_wheel, car.frame, sprite_wheel, car.frame);
				gamby.drawSprite(car.x + 32, 46, sprite_wheel, car.frame, sprite_wheel, car.frame);
				gamby.drawSprite(car.x + 14, 46, sprite_passenger, sprite_passenger);
			}
			if (car.x < 34) {
				car.x += 3;
			} else {
				car.x += 1;
			}
			done_flag = car.x > 46;
			break;
		case SCENE_DROPOFF2:
			if (score.scene_timer == 0) man.x = screen.width - 42;
			gamby.drawSprite(0, 0, sprite_city);
			gamby.drawSprite(car.x, 46, sprite_auto);
			gamby.drawSprite(car.x, 46, sprite_wheel, car.frame, sprite_wheel, car.frame);
			gamby.drawSprite(car.x + 32, 46, sprite_wheel, car.frame, sprite_wheel, car.frame);
			gamby.drawSprite(man.x, 46, sprite_play, man.frame, sprite_play, man.frame);
			man.frame++;
			if (man.frame > 29) man.frame = 0;
			man.x += 2;
			done_flag = man.x > screen.width + 16;
			if (done_flag) {
				bot.size = sqrt(bot.x1 + bot.y1) * 0.1;
				if (timer.minute < 1) {
					score.balance += 1.10 * bot.size;
					if (timer.second < 30) score.balance += timer.second;
				} else if (timer.minute < 2) {
					score.balance += 0.8 * bot.size;
				} else if (timer.minute < 3) {
					score.balance += 0.4 * bot.size;
				}
				score.quota += 1;
			}
			break;
		case SCENE_DROPOFF3:
			draw_map();
			gamby.drawSprite(car.x, 46, sprite_auto);
			gamby.drawSprite(car.x, 46, sprite_wheel, car.frame, sprite_wheel, car.frame);
			gamby.drawSprite(car.x + 32, 46, sprite_wheel, car.frame, sprite_wheel, car.frame);
			if (score.scene_timer == 10 && score.quota < score.fill) place_pickup();
			done_flag = score.scene_timer > 20;
			break;
		case SCENE_DROPOFF4:
			if (car.x % 6 < 3) {
				car.frame = 0;
			} else {
				car.frame = 1;
			}
			gamby.drawSprite(0, 0, sprite_city);
			gamby.drawSprite(car.x, 46, sprite_auto);
			gamby.drawSprite(car.x, 46, sprite_wheel, car.frame, sprite_wheel, car.frame);
			gamby.drawSprite(car.x + 32, 46, sprite_wheel, car.frame, sprite_wheel, car.frame);
			if (car.x < 48) {
				car.x += 1;
			} else {
				car.x += 3;
			}
			done_flag = car.x > screen.width + 49;
		case SCENE_SPLASH:
			gamby.drawSprite(0, (score.scene_timer * 4) - 64, sprite_title);
			gamby.drawSprite(0, score.scene_timer * 4, sprite_splash);
			done_flag = score.scene_timer > 15;
			break;
		case SCENE_TITLE:
			gamby.drawSprite(0, 0, sprite_title);
			break;
		case SCENE_MENU:
			if (score.menu_anim > 8) {
				score.menu_anim = score.menu_anim - 8;
			} else {
				score.menu_anim = 0;
				score.menu_anim_dir = 0;
			}
			if (score.level == 0) gamby.drawSprite(23, 0, sprite_caption);
			gamby.drawSprite(6, 32, sprite_cab);
			if (score.menu_anim_dir == 0 || score.menu_anim_dir == 1) {
				gamby.drawSprite(0, 20, sprite_arrow, (byte)0);
			}
			if (score.menu_anim_dir == 0 || score.menu_anim_dir == -1) {
				gamby.drawSprite(88, 20, sprite_arrow, (byte)1);
			}
			if (score.menu_id == MENU_START || score.menu_id == MENU_RESUME) {
				gamby.drawSprite(22 + (score.menu_anim * score.menu_anim_dir), (sin(score.scene_timer / 2.0) * 2.0) + 21, sprite_menu, score.menu_id);
			} else {
				gamby.drawSprite(22 + (score.menu_anim * score.menu_anim_dir), (sin(score.scene_timer / 4.0) * 1.0) + 21, sprite_menu, score.menu_id);
			}
			break;
		case SCENE_SETTINGS:
			gamby.drawSprite(0, 0, sprite_settings, sprite_settings);
			for (byte i = 0; i < 3; i++) {
				gamby.drawPattern = PATTERN_BLACK;
				if (i == score.difficulty) {
					gamby.box(10, 28 + (12 * i), 48, 40 + (12 * i));
				}
				if (i == score.sound) {
					gamby.box(50, 28 + (12 * i), 88, 40 + (12 * i));
				}
				if ((i == score.difficulty && score.settings_id == 0) || (i == score.sound && score.settings_id == 1))  {
					if (score.scene_timer % 10 < 5) {
						gamby.drawPattern = PATTERN_CHECKER_INV;
					} else {
						gamby.drawPattern = PATTERN_CHECKER;
					}
					gamby.box(11 + (score.settings_id * 40), 29 + (12 * i), 47 + (score.settings_id * 40), 39 + (12 * i));
				} 
			}
			gamby.drawPattern = PATTERN_BLACK;
			break;
		case SCENE_STORY:
			gamby.drawSprite(2, 4, sprite_story);
			break;
		case SCENE_HELP:
			gamby.drawSprite(1, 3, sprite_help);
			break;
		case SCENE_ABOUT:
			gamby.drawSprite(0, 0, sprite_qrcode);
			break;
		case SCENE_QUIT:
			gamby.drawSprite(0, 0, sprite_clipboard);
			gamby.drawSprite(28, 3, sprite_caption, sprite_caption);
			if ((score.is_yes && score.scene_timer % 10 < 5) || !score.is_yes) {
				gamby.drawSprite(10, 36, sprite_menu, MENU_YES, sprite_menu, MENU_YES);
			}
			if ((!score.is_yes && score.scene_timer % 10 < 5) || score.is_yes) {
				gamby.drawSprite(40, 36, sprite_menu, MENU_NO, sprite_menu, MENU_NO);
			}
			break;
		case SCENE_LEVEL:
			gamby.drawSprite(0, 0, sprite_clipboard);
			if (timer.state != TURN_OFF) gamby.drawSprite(28, 3, sprite_caption, sprite_caption);
			break;
	}
	if (done_flag) {
		switch (score.scene_id) {
			case SCENE_PICKUP4:		
				score.map_view = 0;
				setup_size();
				score.scene_id = SCENE_PLAY;
				break;
			case SCENE_DROPOFF3:
				if (score.quota == score.fill) {
					setup_level();
					place_pickup();
				} else {
					score.map_view = 0;
					setup_size();
					score.scene_id = SCENE_PLAY;
				}
				break;
			case SCENE_SPLASH:
				score.scene_id = SCENE_TITLE;
				break;
			case SCENE_TITLE:
				break;
			default:
				score.scene_id++;
				break;
		}
		score.scene_timer = 0;
	} else {
		score.scene_timer++;
		if (score.scene_timer > 10000) score.scene_timer = 2;
	}
}

void reset_all(byte level, byte scene_id) {
	score.level = level;
	if (score.level == 0) score.balance = 0;
	score.map_view = 0;
	score.menu_id = MENU_START;
	score.menu_anim = 0;
	score.scene_id = scene_id;
	score.scene_timer = 0;
	play.speed = 0.0;
	score.quota = 0;
	score.fill = (((score.difficulty + 2) / 2) + ((score.level + 5) / 3)) - 1;
	score.turn = TURN_VACANT;
	setup_timer();
}

void setup_level() {
	score.map_view = 0;
	setup_size();
	make_maze();
	play.dx = (tile.width * 3.5) - screen.width2;
	play.dy = (tile.height * 3.5) - screen.height2;	
	calc_auto();
	score.level++;
	reset_all(score.level, SCENE_LEVEL);
}

void setup_screen() {
	gamby.font = font;
	screen.width = NUM_COLUMNS;
	screen.height = NUM_ROWS;
	screen.width2 = screen.width / 2;
	screen.height2 = screen.height / 2;
}

void setup() {
	setup_screen();
	score.map_view = 0;
	score.scene_id = SCENE_SPLASH;
	score.sound = SOUND_EFFECTS;
	score.difficulty = DIFFICULTY_HARD;
	reset_all(0, SCENE_SPLASH);
}

void draw_map() {
	_tile plot;
	_bot temp;
	int id, pos;
	byte val;
	boolean toggle = score.scene_timer % 10 < 5;
	plot.width = (NUM_COLUMNS / tile.width) + 1;
	plot.height = (NUM_ROWS / tile.height) + 1;
	for (plot.y = 0; plot.y < plot.height; plot.y++) {
		for (plot.x = 0; plot.x < plot.width; plot.x++) {
			temp.x = plot.x;
			temp.y = plot.y;
			if (score.map_view == 0) {
				temp.x += (int)play.dx / tile.width;
				temp.y += (int)play.dy / tile.height;
			}
			if (temp.x > -1 && temp.x < MAP_WIDTH && temp.y > -1 && temp.y < MAP_HEIGHT) {
				val = data_field[temp.x][temp.y];
			} else {
				val = 255;
			}
			tile.x = (plot.x * tile.width);
			tile.y = (plot.y * tile.height);
			tile.x -= (int)play.dx % tile.width;
			tile.y -= (int)play.dy % tile.height;
			if (val == 1) {
				if (score.map_view == 0) {
					pos = temp.x + (temp.y * plot.width);
					if (pos % 3 == 0) {
						gamby.drawPattern = PATTERN_DK_GRID_DOTS;						
					} else {
						gamby.drawPattern = PATTERN_DK_GRAY;
					}
				} else {
					gamby.drawPattern = PATTERN_BLACK;
				}
			} else if (val == 0 || val == 17 || val == 18) {
				gamby.drawPattern = PATTERN_WHITE;;
				if (score.map_view == 0) {
					pos = temp.x + (temp.y * plot.width);
					if (pos % 3 == 0) {
						gamby.drawPattern = PATTERN_LT_GRID_DOTS;						
					} else {
						gamby.drawPattern = PATTERN_LT_GRAY;
					}
				} else {
					gamby.drawPattern = PATTERN_WHITE;;
				}
			}
			if (score.map_view == 1) {
				temp.x0 = (((int)play.dx + screen.width2) / 16);
				temp.y0 = (((int)play.dy + screen.height2) / 16);
				if (temp.x0 == temp.x & temp.y0 == temp.y) {
					temp.x1 = (tile.x + tile.width2) - 1;
					temp.y1 = (tile.y + tile.height2) - 1;
					if (score.scene_timer % 10 < 5) {
						gamby.drawPattern = PATTERN_BLACK;
					} else {
						gamby.drawPattern = PATTERN_LT_GRID_SOLID;
					}
				}
			}
			if ((val == 17 || val == 18)) {
				if (score.map_view == 0) {
					gamby.rect(tile.x, tile.y, tile.x + tile.width, tile.y + tile.height);
					if (val == 17) {
						gamby.drawSprite(tile.x, tile.y, sprite_play, 30 + toggle, sprite_play, 30 + toggle);
					} else if (val == 18) {
						gamby.drawSprite(tile.x, tile.y, sprite_flag, toggle, sprite_flag, toggle);	
					}
				} else {
					bot.x0 = (tile.x + tile.width2) - 1;
					bot.y0 = (tile.y + tile.height2) - 1;
					gamby.rect(tile.x, tile.y, tile.x + tile.width, tile.y + tile.height);
				}
			} else if (val != 255) {
				gamby.rect(tile.x, tile.y, tile.x + tile.width, tile.y + tile.height);
			}
		}
	}
	if (score.scene_id == SCENE_PICKUP1 || score.scene_id == SCENE_PICKUP2) return;
	gamby.drawPattern = PATTERN_BLACK;
	if (score.map_view == 1) {
		gamby.circle(bot.x0, bot.y0, (score.scene_timer % (tile.width * 4)) + 1);
		gamby.box(temp.x1 - (score.scene_timer % (tile.width * 4)), temp.y1 - (score.scene_timer % (tile.height * 4)),temp.x1 + (score.scene_timer % (tile.width * 4)), temp.y1 + (score.scene_timer % (tile.height * 4)));
		if (score.scene_timer % 6 < 3) {
			gamby.drawPattern = PATTERN_DK_GRID_DOTS;
			gamby.line(bot.x0, bot.y0, temp.x1, temp.y1);
			gamby.drawPattern = PATTERN_BLACK;
		}
	}
}

void calc_auto() {
	_bot rect;
	play.last_dx = play.dx;
	play.last_dy = play.dy;
	play.dx += cos(play.angle) * play.speed;
	play.dy += sin(play.angle) * play.speed;
	rect.x = (int)play.dx + (cos(play.angle) * tile.width2);
	rect.y = (int)play.dy + (sin(play.angle) * tile.height2);
	rect.x0 = (rect.x + screen.width2) / tile.width;
	rect.y0 = (rect.y + screen.height2) / tile.height;
	byte val = data_field[rect.x0][rect.y0];
	if (val == 17) {
		score.scene_id = SCENE_PICKUP1;
		score.scene_timer = 0;
	}
	if (val == 18) {
		score.scene_id = SCENE_DROPOFF1;
		score.scene_timer = 0;
	}
	if (val != 0 && val != 16 && 1 == 1) {
		play.speed = 0.0;
		play.dx = play.last_dx;
		play.dy = play.last_dy;
	}
}
void draw_vector(byte x, byte y, int *vect, byte vect_points, float angle, float size) {
	byte i;
	_play rect;
	_bot plot;
	for (byte j = 0; j < vect_points; j += 2) {
		i = j;
		rect.dx = (cos(angle) * vect[i] * size)	- (sin(angle) * vect[i + 1] * size);
		rect.dy = (cos(angle) * vect[i + 1] * size) + (sin(angle) * vect[i] * size);
		plot.x0 = (int)rect.dx + x;
		plot.y0 = (int)rect.dy + y;
		if (j < vect_points - 2) {
			i = j + 2;
		} else {
			i = 0;
		}
		rect.dx = (cos(angle) * vect[i] * size)	- (sin(angle) * vect[i + 1] * size);
		rect.dy = (cos(angle) * vect[i + 1] * size) + (sin(angle) * vect[i] * size);
		plot.x1 = (int)rect.dx + x;
		plot.y1 = (int)rect.dy + y;
		gamby.line(plot.x0, plot.y0, plot.x1, plot.y1);
	}
}

void input_update() {
	gamby.readInputs();
	byte b;
	b = INPUT_BUTTON_N;
	if (gamby.inputs & BUTTON_1) {
		if (button_state[b] < 255) button_state[b]++;
	} else {
		button_state[b] = 0;
	}
	b = INPUT_BUTTON_E;
	if (gamby.inputs & BUTTON_2) {
		if (button_state[b] < 255) button_state[b]++;
	} else {
		button_state[b] = 0;
	}
	b = INPUT_BUTTON_W;
	if (gamby.inputs & BUTTON_3) {
		if (button_state[b] < 255) button_state[b]++;
	} else {
		button_state[b] = 0;
	}
	b = INPUT_BUTTON_S;
	if (gamby.inputs & BUTTON_4) {
		if (button_state[b] < 255) button_state[b]++;
	} else {
		button_state[b] = 0;
	}
	b = INPUT_DPAD_UP;
	if (gamby.inputs & DPAD_UP) {
		if (button_state[b] < 255) button_state[b]++;
	} else {
		button_state[b] = 0;
	}
	b = INPUT_DPAD_DOWN;
	if (gamby.inputs & DPAD_DOWN) {
		if (button_state[b] < 255) button_state[b]++;
	} else {
		button_state[b] = 0;
	}
	b = INPUT_DPAD_LEFT;
	if (gamby.inputs & DPAD_LEFT) {
		if (button_state[b] < 255) button_state[b]++;
	} else {
		button_state[b] = 0;
	}
	b = INPUT_DPAD_RIGHT;
	if (gamby.inputs & DPAD_RIGHT) {
		if (button_state[b] < 255) button_state[b]++;
	} else {
		button_state[b] = 0;
	}
}

void read_input() {
	input_update();
	switch (score.scene_id) {
		case SCENE_PLAY:
			if (score.map_view == 0) {
				if (button_state[INPUT_BUTTON_N] == 1) {
					if (play.speed == 0) {
						if (score.map_view == 0) {
							score.map_view = 1;
						}
						setup_size();
					}
				}
				if (button_state[INPUT_BUTTON_E] > 0) {
					if (play.speed < ((score.difficulty + 2) / 2) + 3) play.speed += 0.2;
				} else {
					if (play.speed > 0.0) {
						play.speed -= 0.75;
						if (play.speed < 0.0) play.speed = 0.0;
					}
				}
				if (button_state[INPUT_BUTTON_W] == 1) {
					score.scene_id = SCENE_MENU;
					score.scene_timer = 0;
					score.menu_id = MENU_RESUME;
				}
				if (gamby.inputs & DPAD_LEFT) {
					play.angle -= PI * 0.1;
				} else if (gamby.inputs & DPAD_RIGHT) {
					play.angle += PI * 0.1;
				}
			} else {
				if (button_state[INPUT_BUTTON_N] == 1) {
					score.map_view = 0;
					setup_size();
					score.scene_timer = 0;
				}
			}
			break;
		case SCENE_SPLASH:
			if (button_state[INPUT_BUTTON_E] == 1) {
				score.scene_id = SCENE_TITLE;
				score.scene_timer = 0;
			}
			break;
		case SCENE_TITLE:
			if (button_state[INPUT_BUTTON_E] == 1) {
				score.scene_id = SCENE_MENU;
				score.scene_timer = 0;
			}
			break;
		case SCENE_LEVEL:
			if (button_state[INPUT_BUTTON_E] == 1) {
				if (score.turn == TURN_OFF) {
					timer.state == TIMER_NONE;
					score.turn == TURN_VACANT;
					reset_all(0, SCENE_TITLE);
				} else {
					score.scene_id = SCENE_PLAY;
					score.scene_timer = 0;
				}
			}
			break;
		case SCENE_MENU:
			if (button_state[INPUT_DPAD_LEFT] == 1) {
				score.menu_anim = 72;
				score.menu_anim_dir = -1;
				if (score.level != 0) {
					if (score.menu_id == MENU_RESUME) {
						score.menu_id = MENU_QUIT;
					} else {
						score.menu_id--;
					}
				} else {
					if (score.menu_id == MENU_START) {
						score.menu_id = MENU_ABOUT;
					} else {
						score.menu_id--;
					}
				}
			}
			if (button_state[INPUT_DPAD_RIGHT] == 1) {
				score.menu_anim = 72;
				score.menu_anim_dir = 1;
				if (score.level != 0) {
					if (score.menu_id == MENU_QUIT) {
						score.menu_id = MENU_RESUME;
					} else {
						score.menu_id++;
					}
				} else {
					if (score.menu_id == MENU_ABOUT) {
						score.menu_id = MENU_START;
					} else {
						score.menu_id++;
					}
				}
			}
			if (button_state[INPUT_BUTTON_E] == 1) {
				switch (score.menu_id) {
					case MENU_START:
						setup_level();
						place_pickup();
						score.scene_id = SCENE_LEVEL;
						score.scene_timer = 0;
						break;
					case MENU_SETTINGS:
						score.scene_id = SCENE_SETTINGS;
						score.scene_timer = 0;
						break;
					case MENU_STORY:
						score.scene_id = SCENE_STORY;
						score.scene_timer = 0;
						break;
					case MENU_HELP:
						score.scene_id = SCENE_HELP;
						score.scene_timer = 0;
						break;
					case MENU_ABOUT:
						score.scene_id = SCENE_ABOUT;
						score.scene_timer = 0;
						break;
					case MENU_RESUME:
						score.scene_id = SCENE_PLAY;
						score.scene_timer = 0;
						break;
					case MENU_QUIT:
						score.scene_id = SCENE_QUIT;
						score.scene_timer = 0;
						break;
				}
			}
			break;
		case SCENE_SETTINGS:
			if (button_state[INPUT_DPAD_LEFT] == 1 && score.settings_id == 1)	score.settings_id = 0;
			if (button_state[INPUT_DPAD_RIGHT] == 1 && score.settings_id == 0) score.settings_id = 1;
			if (score.settings_id == 0) {
				if (button_state[INPUT_DPAD_UP] == 1) {
					if (score.difficulty == 0) {
						score.difficulty = 2;
					} else {
						score.difficulty--;
					}
				}
				if (button_state[INPUT_DPAD_DOWN] == 1) {
					if (score.difficulty == 2) {
						score.difficulty = 0;
					} else {
						score.difficulty++;
					}
				}
			} else {
				if (button_state[INPUT_DPAD_UP] == 1) {
					if (score.sound == 0) {
						score.sound = 2;
					} else {
						score.sound--;
					}
				}
				if (button_state[INPUT_DPAD_DOWN] == 1) {
					if (score.sound == 2) {
						score.sound = 0;
					} else {
						score.sound++;
					}
				}
			}
			if (button_state[INPUT_BUTTON_E] == 1) {
				score.scene_id = SCENE_MENU;
				score.scene_timer = 0;
			}
			break;
		case SCENE_STORY:
			if (button_state[INPUT_BUTTON_E] == 1) {
				score.scene_id = SCENE_MENU;
				score.scene_timer = 0;
			}
			break;
		case SCENE_ABOUT:
			if (button_state[INPUT_BUTTON_E] == 1) {
				score.scene_id = SCENE_MENU;
				score.scene_timer = 0;
			}
			break;
		case SCENE_HELP:
			if (button_state[INPUT_BUTTON_E] == 1) {
				score.scene_id = SCENE_MENU;
				score.scene_timer = 0;
			}
			break;
		case SCENE_QUIT:
			if (button_state[INPUT_DPAD_LEFT] == 1) score.is_yes = true;
			if (button_state[INPUT_DPAD_RIGHT] == 1) score.is_yes = false;
			if (button_state[INPUT_BUTTON_E] == 1) {
				if (score.is_yes) {
					score.is_yes = false;
					reset_all(0, SCENE_TITLE);
				} else {
					score.scene_id = SCENE_MENU;
				}
				score.scene_timer = 0;
			}
			break;
	}
}

void draw_radar() {
	_play plot;
	_bot radar;
	radar.x0 = ((int)play.dx + screen.width2) / tile.width;
	radar.y0 = ((int)play.dy + screen.height2) / tile.height;
	double angle = atan2(bot.y - radar.y0, bot.x - radar.x0) - (PI * 2);
	plot.angle = (float)angle;
	plot.dx = cos(plot.angle) * tile.width2;
	plot.dy = sin(plot.angle) * tile.height2;
	radar.x = (int)plot.dx + screen.width2;
	radar.y = (int)plot.dy + screen.height2;
	gamby.rect(radar.x - 1, radar.y - 1, radar.x + 1, radar.y + 1); 
}

void draw_auto() {
	calc_auto();
	gamby.drawPattern = PATTERN_WHITE;
	gamby.disc(screen.width2, screen.height2, screen.width2 / 12);
	gamby.drawPattern = PATTERN_BLACK;
	draw_vector(screen.width2, screen.height2, vector_auto, sizeof(vector_auto) / sizeof(int), play.angle, tile.width2 * 0.1625);
	if (score.scene_timer % 6 < 3) draw_radar();
}

void print_time(byte x, byte y) {
	gamby.setPos(x, y);
	if (timer.state == TIMER_ALERT && score.scene_timer % 4 < 2 == 0) gamby.textDraw = TEXT_INVERSE;
	gamby.print(" ");
	gamby.print(timer.minute_alt);
	gamby.print(":");
	if (timer.second_alt < 10) gamby.print("0");
	gamby.print(timer.second_alt);
	gamby.print(" ");
	gamby.textDraw = TEXT_NORMAL;
}

void print_location(byte x, byte y) {
	if (y < MAP_HEIGHT * 0.5) {
		gamby.print("North");
	} else {
		gamby.print("South");
	}
	if (x < MAP_WIDTH * 0.3) {
		gamby.print("west");
	} else if (x < MAP_WIDTH * 0.6) {
		gamby.print("-mid");
	} else {
		gamby.print("east");
	}
}

void print_dash() {
	_bot temp;
	temp.x = (((int)play.dx + screen.width2) / 16);
	temp.y = (((int)play.dy + screen.height2) / 16);
	if (score.turn == TURN_PICKUP) {
		gamby.setPos(0, 4);
		print_location(bot.x, bot.y);
		if (bot.x != temp.x && bot.y != temp.y) {
			gamby.print(" to ");
			print_location(temp.x, temp.y);
		}
		gamby.print(".");
		gamby.setPos(0, 5);
		gamby.print("Pickup");
	} else {
		gamby.setPos(0, 4);
		print_location(temp.x, temp.y);
		gamby.print(" to ");
		print_location(bot.x, bot.y);
		gamby.print(".");
		gamby.setPos(0, 5);
		gamby.print("Dropoff");
	}
	if (score.scene_id == SCENE_PLAY || (score.scene_id >= SCENE_PICKUP3 && score.scene_id <= SCENE_PICKUP4) ||
		(score.scene_id >= SCENE_DROPOFF3 && score.scene_id <= SCENE_DROPOFF4)) {
		bot.x0 = bot.x - (((int)play.dx + screen.width2) / 16);
		bot.y0 = bot.y - (((int)play.dy + screen.height2) / 16);
		bot.x1 = bot.x0 * bot.x0;
		bot.y1 = bot.y0 * bot.y0;
		gamby.setPos(0, 6);
		gamby.print(score.quota);
		gamby.print("/");
		gamby.print(score.fill);
		gamby.setPos(0, 7);
		gamby.print("$");
		gamby.print(score.balance);
	}
	if (score.scene_id == SCENE_PLAY) {
		gamby.setPos(48, 7);
		gamby.print(sqrt(bot.x1 + bot.y1) * 0.1);
		gamby.print("km");
		gamby.setPos(48, 6);
		gamby.print("Day: ");
		gamby.print(score.level);
	}
}

void print_preview_level() {
	if (timer.state == TIMER_END) {
		if (score.scene_id % 100 < 50 == 0) gamby.textDraw = TEXT_INVERSE;
		gamby.setPos(26, 1);
		gamby.print(" GAME OVER ");
		gamby.textDraw = TEXT_NORMAL;
		gamby.setPos(22, 3);
		gamby.print("Total Days: ");
	} else {
		gamby.setPos(22, 3);
		gamby.print("Day: ");
	}
	gamby.print(score.level);
	gamby.setPos(22, 4);
	gamby.print("Difficulty: ");
	switch (score.difficulty) {
		case DIFFICULTY_EASY:
			gamby.print("Easy");
			break;
		case DIFFICULTY_HARD:
			gamby.print("Hard");
			break;
		case DIFFICULTY_EXPERT:
			gamby.print("Expert");
			break;
	}
	gamby.setPos(22, 5);
	gamby.print("Quota: ");
	gamby.print(score.quota);
	gamby.print("/");
	gamby.print(score.fill);
	gamby.setPos(22, 6);
	gamby.print("$: ");
	gamby.print(score.balance);
}

void print_status() {
	if (score.map_view == 0) {
		switch (score.scene_id) {
			case SCENE_PLAY:
				update_timer();
				if (timer.state != TIMER_NONE) {
					gamby.setPos(37, 7);
					print_time(37, 7);
				}
				break;
			case SCENE_LEVEL:
				print_preview_level();
				break;
			case SCENE_QUIT:
				gamby.setPos(40, 3);
				gamby.print("Quit?");
				break;
			case SCENE_TITLE:
				gamby.textDraw = TEXT_INVERSE;
				if (score.scene_timer % 30 < 20) {
					gamby.setPos(22, 7);
					gamby.print("PRESS START");
				}
				gamby.textDraw = TEXT_NORMAL;
				break;
		}
	} else {
		switch (score.scene_id) {
			case SCENE_PLAY:
				update_timer();
				print_dash();
				print_time(45, 5);
				break;
		}
		if (score.scene_id >= SCENE_PICKUP1 && score.scene_id <= SCENE_PICKUP4) print_dash();
		if (score.scene_id >= SCENE_DROPOFF1 && score.scene_id <= SCENE_DROPOFF4) print_dash();
	}
	if (score.scene_id == SCENE_PLAY && timer.state == TIMER_END) {
		score.scene_id = SCENE_LEVEL;
		score.scene_timer = 0;
		score.map_view = 0;
		score.turn = TURN_OFF;
	}
}
	
void update_sound() {
	song.delay = 0;
	int frequency = 0;
	switch (score.scene_id) {
		case SCENE_SPLASH:
			if (score.scene_timer == 1) frequency = 800;
			song.delay = 1;
			break;
		case SCENE_LEVEL:
			if (score.scene_timer == 1) frequency = 800;
			song.delay = 1;
			break;
		case SCENE_TITLE:
			if (score.scene_timer == 1) frequency = 800;
			song.delay = 1;
			break;
		case SCENE_PLAY:
			if (score.map_view == 0 && score.sound == SOUND_EFFECTS) frequency = 50 + (play.speed * 10);
			if (score.scene_timer == 1)	frequency = 800;
			if (timer.state == TIMER_ALERT) frequency = 400;
			song.delay = 1;
			break;
		case SCENE_MENU:
			if (score.menu_anim == 64) frequency = 800;
			if (score.scene_timer == 1) frequency = 800;
			song.delay = 3;
			break;
		case SCENE_SETTINGS:
			if (score.scene_timer == 1) frequency = 800;
			song.delay = 3;
			break;
		case SCENE_PICKUP1:
			frequency = 40;
			if (score.scene_timer == 1) frequency = 500;
			break;
		case SCENE_PICKUP4:
			frequency = 80;
			break;
		case SCENE_DROPOFF1:
			frequency = 80;
			break;
		case SCENE_DROPOFF3:
			if (score.scene_timer == 1) frequency = 1000;
			break;
		case SCENE_DROPOFF4:
			frequency = 40;
			break;
	}
	if (frequency > 0 && score.sound != SOUND_OFF) tone(PIN_SPEAKER, frequency, 50);
	if (score.sound == SOUND_MUSIC) update_song();
}

void setup_song() {
	for (byte i = 0; i < 4; i++) {
		if (song.pattern == 4) {
			song.frequency[i] = ((4 - i) * 100) + 100 - random(100);
		} else {
			song.frequency[i] = (i * 100) + 200 - random(100);
		}
		song.duration = 50;
	}
}

void update_song() {
	if (score.scene_timer % song.delay == 0) {
		tone(PIN_SPEAKER, song.frequency[song.pos], song.duration);
		song.pos++;
		if (song.pos % 4 == 0) {
			tone(PIN_SPEAKER, 50, 50);
		}
		if (song.pos == 4) {
			song.bar++;
			if (song.bar == 4) {
				song.pattern++;
				setup_song();
				if (song.pattern == 4) {
					song.pattern = 0;
				}
				song.bar = 0;
			}
			song.pos = 0;
		}
	}
}

void loop() {
	read_input();
	gamby.clearScreen();
	display_scene();
	gamby.update();
	print_status();
	update_sound();
}
