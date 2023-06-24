/*
 * PNG Layer Image Vectorizer
 * Interactively generates rules and saves them for future runs
 *
 * Copyright 2023 QMT Productions
 */

#include <stdlib.h>
#include <stdio.h>
#include <setjmp.h>
#include <png.h>

#ifdef WIN32
#include <conio.h>
#define getch _getch
#else
#include <termios.h>
char getch(void)
{
	char buf = 0;
	struct termios old, cur;
	if (tcgetattr(0, &old) < 0)
		perror("tcsetattr");
	cur = old;
	cur.c_lflag &= ~(ICANON | ECHO);
	if (tcsetattr(0, TCSANOW, &cur) < 0)
		perror("tcsetattr");
	buf = getchar();
	if (tcsetattr(0, TCSANOW, &old) < 0)
		perror("tcsetattr");
	return buf;
}
#endif

#include <vector>
#include <map>
#include <set>

struct coord
{
	int x = -1;
	int y = -1;
};
std::set<int> unrec;

enum DIR { DIR_E, DIR_SE, DIR_S, DIR_SW, DIR_W, DIR_NW, DIR_N, DIR_NE, DIR_NONE };
const char *dirs[9] = {"east", "southeast", "south", "southwest", "west", "northwest", "north", "northeast", "indeterminate"};
const char dir_char[9] = {'6', '3', '2', '1', '4', '7', '8', '9', '?'};

std::map<int,DIR> rules;
bool discard_rules = false;

void load_rules()
{
	FILE *in = fopen("pngtrace.rules", "rb");
	if (!in)
		return;
	rules.clear();
	while (true)
	{
		unsigned long entry;
		if (fread(&entry, 4, 1, in) == 0)
			break;
		int mask = (entry & 0xFFFFFF);
		DIR dir = (DIR)((entry & 0xF000000) >> 24);
		rules[mask] = dir;
	}
	fclose(in);
}
void save_rules()
{
	FILE *out = fopen("pngtrace.rules", "wb");
	for (auto iter = rules.begin(); iter != rules.end(); iter++)
	{
		unsigned long entry = iter->first | ((int)iter->second << 24);
		fwrite(&entry, 4, 1, out);
	}
	fclose(out);
}

#define CHUNK_SIZE 256
struct img_chunk
{
	unsigned char pixels[CHUNK_SIZE][CHUNK_SIZE];
	unsigned char get(int dx, int dy)
	{
		if (dx < 0 || dx >= CHUNK_SIZE || dy < 0 || dy >= CHUNK_SIZE)
			return 0;
		return pixels[dx][dy];
	}
	void set (int dx, int dy, unsigned char val)
	{
		if (dx < 0 || dx >= CHUNK_SIZE || dy < 0 || dy >= CHUNK_SIZE)
			return;
		pixels[dx][dy] = val;
	}
};

struct img_data
{
	img_chunk **data;
	int cw, ch, pw, ph;

	img_data()
	{
		data = NULL;
		cw = ch = pw = ph = 0;
	}
	~img_data()
	{
		dealloc();
	}
	void dealloc()
	{
		if (!data)
			return;

		int cx, cy;
		for (cx = 0; cx < cw; cx++)
			delete[] data[cx];

		delete[] data;
		data = NULL;
		cw = ch = pw = ph = 0;
	}

	void alloc (int width, int height)
	{
		dealloc();

		pw = width;
		ph = height;
		cw = (pw / CHUNK_SIZE) + 1;
		ch = (ph / CHUNK_SIZE) + 1;

		int cx, cy, dx, dy;

		data = new img_chunk *[cw];
		for (cx = 0; cx < cw; cx++)
		{
			data[cx] = new img_chunk[ch];
			for (cy = 0; cy < ch; cy++)
			{
				for (dx = 0; dx < CHUNK_SIZE; dx++)
					for (dy = 0; dy < CHUNK_SIZE; dy++)
						data[cx][cy].set(dx, dy, 0);
			}
		}
	}
	void set (int x, int y, unsigned char val)
	{
		if (x < 0 || x >= pw || y < 0 || y >= ph)
			return;
		data[x / CHUNK_SIZE][y / CHUNK_SIZE].set(x % CHUNK_SIZE, y % CHUNK_SIZE, val);
	}
	unsigned char get (int x, int y)
	{
		if (x < 0 || x >= pw || y < 0 || y >= ph)
			return 0;
		return data[x / CHUNK_SIZE][y / CHUNK_SIZE].get(x % CHUNK_SIZE, y % CHUNK_SIZE);
	}

	void printmask (int cur_corner, DIR dir)
	{
		printf("0x%06X : %s\n", cur_corner, dirs[dir]);
		for (int dy = 0; dy < 5; dy++)
		{
			for (int dx = 0; dx < 5; dx++)
			{
				if (dx == 2 && dy == 2)
					printf("%c", dir_char[dir]);
				else
				{
					printf("%c", (cur_corner & (1 << 23)) ? '*' : '.');
					cur_corner <<= 1;
				}
			}
			printf("\n");
		}
	}

	bool is_corner(int x, int y, DIR &dir, int &cur_corner)
	{
		cur_corner = 0;
		for (int dy = 0; dy < 5; dy++)
		{
			for (int dx = 0; dx < 5; dx++)
			{
				// Skip the center cell
				if (dx == 2 && dy == 2)
					continue;
				cur_corner <<= 1;
				if (get(x + dx - 2, y + dy - 2))
					cur_corner |= 1;
			}
		}

		if (rules.count(cur_corner))
		{
			if (dir == rules.at(cur_corner))
				return false;
			dir = rules.at(cur_corner);
			return true;
		}

		printf("Region at %i,%i unrecognized!\n", x, y);
		printmask(cur_corner, dir);
		printf("Specify target dir on numpad, 'q' to save+exit, 'x' to abort: ");
		dir = DIR_NONE;
		do
		{
			char d = getch();
			switch (d)
			{
			case '1': dir = DIR_SW;	break;
			case '2': dir = DIR_S;	break;
			case '3': dir = DIR_SE;	break;
			case '4': dir = DIR_W;	break;
			case '6': dir = DIR_E;	break;
			case '7': dir = DIR_NW;	break;
			case '8': dir = DIR_N;	break;
			case '9': dir = DIR_NE;	break;
			case 'x': case 'X':
				discard_rules = true;
			case 'q': case 'Q':
				return false;
			}
		} while (dir == DIR_NONE);
		printf("\n");

		rules[cur_corner] = dir;
		return true;
	}

	bool find_corner (int &x, int &y, bool check_only = false)
	{
		int ox = x, oy = y;
		DIR dir = DIR_NONE;
		int last;
		if (!is_corner(x, y, dir, last))
			return false;
		if (check_only)
			return true;
		while (1)
		{
			switch (dir)
			{
			case DIR_E:	x++;		break;
			case DIR_SE:	x++; 	y++;	break;
			case DIR_S:		y++;	break;
			case DIR_SW:	x--;	y++;	break;
			case DIR_W:	x--;		break;
			case DIR_NW:	x--;	y--;	break;
			case DIR_N:		y--;	break;
			case DIR_NE:	x++;	y--;	break;
			}
			if (!get(x, y))
			{
				printf("Somehow, this movement to %i,%i landed at an empty cell:\n", x, y);
				printmask(last, dir);
				return false;
			}
			if (is_corner(x, y, dir, last))
				return true;
			if (dir == DIR_NONE)
			{
				printf("Aborted.\n");
				return false;
			}
			if (x < 0 || x >= pw || y < 0 || y >= ph)
			{
				printf("Walked off edge! (going %s from %i,%i)\n", dirs[dir], ox, oy);
				return false;
			}
		}
	}

	bool trace(FILE *out, int x, int y)
	{
		int ox, oy;
		if (!find_corner(x, y, true))
		{
			printf("Node at %i,%i did not start at recognized corner!\n", x, y);
			return false;
		}
		ox = x;
		oy = y;
		do
		{
			if (out)
				fprintf(out, "%i,%i\n", x, y);
			if (!find_corner(x, y))
				return false;
		} while (x != ox || y != oy);
		if (out)
			fprintf(out, "-1,-1\n");
		return true;
	}

	void floodErase(int x, int y)
	{
		std::vector<coord> flood;
		flood.push_back({x, y});
		while (!flood.empty())
		{
			coord cur = flood.back();
			flood.pop_back();
			if (!get(cur.x, cur.y))
				continue;
			set(cur.x, cur.y, 0);
			flood.push_back({cur.x - 1, cur.y});
			flood.push_back({cur.x + 1, cur.y});
			flood.push_back({cur.x, cur.y - 1});
			flood.push_back({cur.x, cur.y + 1});
		}
	}

	void doTrace(FILE *out)
	{
		if (!data)
			return;
		for (int py = 0; py < ph; py++)
		{
			for (int px = 0; px < pw; px++)
			{
				if (!get(px, py))
					continue;
				if (!trace(out, px, py))
					return;
				floodErase(px, py);
			}
		}
	}
} pixels;

unsigned int component(png_const_bytep row, png_uint_32 x, unsigned int c, unsigned int bit_depth, unsigned int channels)
{
	png_uint_32 bit_offset_hi = bit_depth * ((x >> 6) * channels);
	png_uint_32 bit_offset_lo = bit_depth * ((x & 0x3f) * channels + c);

	row = (png_const_bytep)(((const png_byte (*)[8])row) + bit_offset_hi);
	row += bit_offset_lo >> 3;
	bit_offset_lo &= 0x07;

	switch (bit_depth)
	{
	case 1: return (row[0] >> (7-bit_offset_lo)) & 0x01;
	case 2: return (row[0] >> (6-bit_offset_lo)) & 0x03;
	case 4: return (row[0] >> (4-bit_offset_lo)) & 0x0f;
	case 8: return row[0];
	case 16: return (row[0] << 8) + row[1];
	default:
		fprintf(stderr, "pngtrace: invalid bit depth %u\n", bit_depth);
		exit(1);
	}
}

unsigned char get_alpha(png_structp png_ptr, png_infop info_ptr, png_const_bytep row, png_uint_32 x)
{
	unsigned int bit_depth = png_get_bit_depth(png_ptr, info_ptr);

	switch (png_get_color_type(png_ptr, info_ptr))
	{
	case PNG_COLOR_TYPE_GRAY:
		return 255;

	case PNG_COLOR_TYPE_PALETTE:
	{
		int index = component(row, x, 0, bit_depth, 1);
		png_colorp palette = NULL;
		int num_palette = 0;
		png_bytep trans_alpha = NULL;
		int num_trans = 0;

		if (!((png_get_PLTE(png_ptr, info_ptr, &palette, &num_palette) & PNG_INFO_PLTE) && num_palette > 0 && palette != NULL))
			png_error(png_ptr, "pngtrace: invalid index");

		if ((png_get_tRNS(png_ptr, info_ptr, &trans_alpha, &num_trans, NULL) & PNG_INFO_tRNS) && num_trans > 0 && trans_alpha != NULL)
			return index < num_trans ? trans_alpha[index] : 255;
		else	return 255;
	}

	case PNG_COLOR_TYPE_RGB:
		return 255;

	case PNG_COLOR_TYPE_GRAY_ALPHA:
		return component(row, x, 1, bit_depth, 2);

	case PNG_COLOR_TYPE_RGB_ALPHA:
		return component(row, x, 3, bit_depth, 4);

	default:
		png_error(png_ptr, "pngtrace: invalid color type");
	}
	return 0;
}

int main(int argc, const char **argv)
{
	// volatile because of setjmp
	volatile int result = 1;
	long x, y;
	FILE *in = NULL;
	FILE *out = NULL;
	// see above
	volatile png_bytep row = NULL;
	png_structp png_ptr = NULL;
	png_infop info_ptr = NULL;

	if (argc < 2)
	{
		fprintf(stderr, "Usage: pngtrace <input.png> [output.txt]\n");
		return 1;
	}

	in = fopen(argv[1], "rb");
	if (!in)
	{
		fprintf(stderr, "pngtrace: could not open input file '%s', aborting.\n", argv[1]);
		goto done;
	}
	printf("Loading PNG file into memory...\n");

	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!png_ptr)
	{
		fprintf(stderr, "pngtrace: out of memory allocating png_struct\n");
		goto done;
	}

	info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr)
	{
		fprintf(stderr, "pngtrace: out of memory allocating png_info\n");
		goto done;
	}

	if (setjmp(png_jmpbuf(png_ptr)) == 0)
	{
		png_uint_32 width, height;
		int bit_depth, color_type, interlace_method, compression_method, filter_method;
		png_bytep row_tmp;

		png_init_io(png_ptr, in);
		png_read_info(png_ptr, info_ptr);
		row = (png_bytep)png_malloc(png_ptr, png_get_rowbytes(png_ptr, info_ptr));
		row_tmp = row;

		if (png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type, &interlace_method, &compression_method, &filter_method))
		{
			png_uint_32 px, py;
			if (interlace_method != PNG_INTERLACE_NONE)
				png_error(png_ptr, "pngtrace: unsupported interlace");

			pixels.alloc(width, height);

			png_start_read_image(png_ptr);

			for (py = 0; py < height; py++)
			{
				png_read_row(png_ptr, row_tmp, NULL);

				for (px = 0; px < width; px++)
					pixels.set(px, py, get_alpha(png_ptr, info_ptr, row_tmp, px));
			}
			row = NULL;
			png_free(png_ptr, row_tmp);
			result = 0;
		}
		else	png_error(png_ptr, "pngtrace: png_get_IHDR failed");
	}
	else
	{
		if (row != NULL)
		{
			png_bytep row_tmp = row;
			row = NULL;
			png_free(png_ptr, row_tmp);
		}
	}

done:
	if (info_ptr)
		png_destroy_info_struct(png_ptr, &info_ptr);
	if (png_ptr)
		png_destroy_read_struct(&png_ptr, NULL, NULL);
	if (in)
		fclose(in);

	if (result > 0)
		return result;

	printf("Extracting polygons...\n");

	if (argc > 2)
	{
		out = fopen(argv[2], "wt");
		if (!out)
		{
			fprintf(stderr, "pngtrace: could not create output file '%s'\n", argv[2]);
			return 1;
		}
	}

	load_rules();
	pixels.doTrace(out);
	if (!discard_rules)
		save_rules();

	if (out)
		fclose(out);
	printf("Done!\n");
	return 0;
}
