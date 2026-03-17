/*
 * PNG Layer Image Vectorizer
 * Interactively generates rules and saves them for future runs
 *
 * Copyright 2023-2026 QMT Productions
 */

#include <stdlib.h>
#include <string.h>
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

enum DIR { DIR_E, DIR_SE, DIR_S, DIR_SW, DIR_W, DIR_NW, DIR_N, DIR_NE, DIR_NONE };
const char *dirs[9] = {"east", "southeast", "south", "southwest", "west", "northwest", "north", "northeast", "indeterminate"};
const char dir_char[9] = {'6', '3', '2', '1', '4', '7', '8', '9', '?'};
// coordinate adjustments for going around corners
// 0 = upper-left
// 1 = upper-right
// 2 = bottom-left
// 3 = bottom-right
// 4 = invalid
const int8_t dir_delta[8][8] = {
	//E SE  S SW  W NW  N  NE
	{ 4, 1, 1, 3, 4, 0, 0, 0}, // from E
	{ 1, 4, 1, 3, 3, 4, 0, 0}, // from SE
	{ 1, 1, 4, 3, 3, 2, 4, 3}, // from S
	{ 1, 1, 3, 4, 3, 2, 2, 4}, // from SW
	{ 4, 3, 3, 3, 4, 2, 2, 0}, // from W
	{ 0, 4, 3, 3, 3, 4, 2, 0}, // from NW
	{ 0, 1, 4, 2, 2, 2, 4, 0}, // from N
	{ 0, 1, 1, 4, 2, 2, 0, 4}  // from NE
};

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
	unsigned char pixels[CHUNK_SIZE][CHUNK_SIZE / 8];
	unsigned char get(int dx, int dy)
	{
		if (dx < 0 || dx >= CHUNK_SIZE || dy < 0 || dy >= CHUNK_SIZE)
			return 0;
		return (pixels[dx][dy / 8] >> (dy & 7)) & 1;
	}
	void set (int dx, int dy, bool val)
	{
		if (dx < 0 || dx >= CHUNK_SIZE || dy < 0 || dy >= CHUNK_SIZE)
			return;
		pixels[dx][dy / 8] &= ~(1 << (dy & 7));
		if (val)
			pixels[dx][dy / 8] |= 1 << (dy & 7);
	}
	void invert(int dx, int dy)
	{
		if (dx < 0 || dx >= CHUNK_SIZE || dy < 0 || dy >= CHUNK_SIZE)
			return;
		pixels[dx][dy / 8] ^= 1 << (dy & 7);
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
	void set (int x, int y, bool val)
	{
		if (x < 0 || x >= pw || y < 0 || y >= ph)
			return;
		data[x / CHUNK_SIZE][y / CHUNK_SIZE].set(x % CHUNK_SIZE, y % CHUNK_SIZE, val);
	}
	bool get (int x, int y)
	{
		if (x < 0 || x >= pw || y < 0 || y >= ph)
			return 0;
		return data[x / CHUNK_SIZE][y / CHUNK_SIZE].get(x % CHUNK_SIZE, y % CHUNK_SIZE);
	}
	void invert(int x, int y)
	{
		if (x < 0 || x >= pw || y < 0 || y >= ph)
			return;
		data[x / CHUNK_SIZE][y / CHUNK_SIZE].invert(x % CHUNK_SIZE, y % CHUNK_SIZE);
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

	bool find_corner (int &x, int &y, int &dx, int &dy, bool check_only = false)
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
			DIR dir_prev = dir;
			if (is_corner(x, y, dir, last))
			{
				int8_t delta = dir_delta[dir_prev][dir];
				dx = (delta & 1) ? 1 : 0;
				dy = (delta & 2) ? 1 : 0;
				return true;
			}
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
		int dx, dy;
		if (!find_corner(x, y, dx, dy, true))
		{
			printf("Node at %i,%i did not start at recognized corner!\n", x, y);
			return false;
		}
		ox = x;
		oy = y;
		do
		{
			if (!find_corner(x, y, dx, dy))
				return false;
			if (out)
				fprintf(out, "%i,%i\n", x + dx, y + dy);
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
		int total = 0;
		if (!data)
			return;
		for (int py = 0; py < ph; py++)
		{
			for (int px = 0; px < pw; px++)
			{
				if (!get(px, py))
					continue;
				total++;
				if (!trace(out, px, py))
					return;
				printf("%i...\r", total);
				floodErase(px, py);
			}
		}
	}

	void doHollow()
	{
		doInvert();
		printf("Searching for holes...\n");
		for (int py = 0; py < ph; py++)
		{
			for (int px = 0; px < pw; px++)
			{
				if (!get(px, py))
					continue;
				printf("Hole found at %i,%i\n", px, py);
				floodErase(px, py);
			}
		}
	}

	void doInvert()
	{
		printf("Inverting image...\n");
		for (int px = 0; px < pw; px++)
			for (int py = 0; py < ph; py++)
				invert(px, py);
		printf("Clearing background...\n");
		floodErase(0, 0);
	}
} pixels;

unsigned int get_rgb(int channels, png_const_bytep row, png_uint_32 x)
{
	unsigned char alpha = 0xFF;
	// If we have 4 channels, there's an Alpha channel
	if (channels == 4)
		alpha = row[x * channels + 3];
	// If it's fully transparent, just return black
	if (alpha == 0)
		return 0;

	// Read colors
	unsigned int rgb = (row[x * channels + 0] << 16) | (row[x * channels + 1] << 8) | (row[x * channels + 2] << 0);
	// If it's not fully opaque, add an error marker
	if (alpha != 0xFF)
		rgb |= 0xF0000000;
	return rgb;
}

// libpng helper functions
#define LIBPNG_SETJMP(ret) if (setjmp(png_jmpbuf(png_ptr))) return ret;

bool readpng_init (FILE *infile, png_structp &png_ptr, png_infop &info_ptr, png_uint_32 &width, png_uint_32 &height, int &channels, png_bytep &row)
{
	// Make sure this is actually a PNG file
	unsigned char header[8];
	if (fread(header, 1, 8, infile) != 8)
		return false;
	if (!png_check_sig(header, 8))
		return false;

	// Create PNG reader
	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!png_ptr)
		return false;

	// Create PNG info
	info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr)
		return false;

	// Setup error handler, in case anything below fails
	LIBPNG_SETJMP(false)

	// Open the file
	png_init_io(png_ptr, infile);
	// Skip the header, since we already read that above
	png_set_sig_bytes(png_ptr, 8);
	// Read file info
	png_read_info(png_ptr, info_ptr);

	// Read the image data header to get general characteristics
	int bit_depth, color_type, interlace;
	png_uint_32 ihdr = png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type, &interlace, NULL, NULL);
	if (!ihdr)
	{
		fprintf(stderr, "pngtrace: failed to read image header\n");
		return false;
	}
	// Don't mess with interlaced images, not worth the trouble
	if (interlace != PNG_INTERLACE_NONE)
	{
		fprintf(stderr, "pngtrace: interlaced images are not supported\n");
		return false;
	}

	// Request some transformations to make things easier for us
	// Expand paletted images to RGB
	if (color_type == PNG_COLOR_TYPE_PALETTE)
		png_set_palette_to_rgb(png_ptr);
	// Expand grayscale to 8-bit
	if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
		png_set_expand_gray_1_2_4_to_8(png_ptr);
	// Convert palette transparency to alpha channel
	if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
		png_set_tRNS_to_alpha(png_ptr);
	// Promote grayscale to RGB
	if (color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
		png_set_gray_to_rgb(png_ptr);
	// Reduce RGB-48 down to RGB-24
	if (bit_depth == 16)
		png_set_strip_16(png_ptr);

	// Apply the above transformations
	png_read_update_info(png_ptr, info_ptr);

	// Make sure we have a proper channel count - either 3 (RGB) or 4 (RGBA)
	channels = png_get_channels(png_ptr, info_ptr);
	if (channels < 2 || channels > 4)
	{
		fprintf(stderr, "pngtrace: image transform failed, got unexpected channel count %i\n", channels);
		return false;
	}

	// Allocate the row buffer
	row = (png_bytep)png_malloc(png_ptr, png_get_rowbytes(png_ptr, info_ptr));
	return true;
}

bool readpng_read_row(png_structp &png_ptr, png_infop &info_ptr, png_bytep row)
{
	LIBPNG_SETJMP(false)

	png_read_row(png_ptr, row, NULL);
	return true;
}

bool readpng_finish(png_structp &png_ptr, png_infop &info_ptr)
{
	LIBPNG_SETJMP(false)

	png_read_end(png_ptr, NULL);
	return true;
}

void readpng_cleanup(png_structp &png_ptr, png_infop &info_ptr, png_bytep &row)
{
	if (row)
	{
		png_free(png_ptr, row);
		row = NULL;
	}
	if (png_ptr)
	{
		if (info_ptr)
			png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
		else	png_destroy_read_struct(&png_ptr, NULL, NULL);
		png_ptr = NULL;
		info_ptr = NULL;
	}
}

int main(int argc, const char **argv)
{
	// File handles
	FILE *in = NULL;
	FILE *out = NULL;

	// PNG parsing data
	int result = 1;
	png_bytep row = NULL;
	png_structp png_ptr = NULL;
	png_infop info_ptr = NULL;
	png_uint_32 width, height;
	int channels;

	// Color filter data
	unsigned int color = 0xFFFFFFFF;
	std::set<unsigned int> colors;

	if (argc < 2)
	{
		fprintf(stderr, "Usage: pngtrace <input.png> <RRGGBB> [output.txt]\n");
		fprintf(stderr, "Specify a filename of '--hollow' to scan for hollow nodes.\n");
		fprintf(stderr, "Specify a filename of '--holes' to trace inside all holes.\n");
		return 1;
	}

	in = fopen(argv[1], "rb");
	if (!in)
	{
		fprintf(stderr, "pngtrace: could not open input file '%s', aborting.\n", argv[1]);
		goto done;
	}
	printf("Loading image file...\n");

	if (argc > 2)
		color = std::strtoul(argv[2], nullptr, 16);

	if (!readpng_init(in, png_ptr, info_ptr, width, height, channels, row))
	{
		fprintf(stderr, "pngtrace: error reading file\n");
		goto done;
	}

	pixels.alloc(width, height);
	for (int py = 0; py < height; py++)
	{
		if (!readpng_read_row(png_ptr, info_ptr, row))
		{
			fprintf(stderr, "pngtrace: error reading image data\n");
			goto done;
		}
		for (int px = 0; px < width; px++)
		{
			unsigned int c = get_rgb(channels, row, px);
			if (c == 0xFFFFFFFF)
			{
				printf("Partially transparent pixel %06X detected at %i,%i\n", c & 0xFFFFFF, px, py);
				// clear out input color so we don't actually start tracing - make them fix it first
				color = 0xFFFFFFFF;
			}
			// skip black pixels
			if (c == 0)
				continue;
			// add it to our color list, and mark it in our canvas
			colors.insert(c);
			pixels.set(px, py, c == color);
		}
	}
	// read and discard PNG footer
	if (readpng_finish(png_ptr, info_ptr))
		result = 0;
done:
	// clean up everything
	readpng_cleanup(png_ptr, info_ptr, row);
	if (in)
		fclose(in);

	// if we didn't reach the footer successfully, bail out now
	if (result > 0)
		return result;

	// if we didn't specify a color (or if we found a partially transparent pixel),
	// print them all out so a proper one can be selected next run
	if (color == 0xFFFFFFFF)
	{
		printf("Colors found:\n");
		for (auto iter = colors.begin(); iter != colors.end(); iter++)
			printf("* %06X\n", *iter);
		return 0;
	}

	if (argc > 3)
	{
		if (!strcmp(argv[3], "--hollow"))
		{
			printf("Scanning for hollow nodes...\n");
			pixels.doHollow();
			printf("Done!\n");
			return 0;
		}
		if (!strcmp(argv[3], "--holes"))
		{
			printf("Tracing holes...\n");
			pixels.doInvert();
		}
		else
		{
			out = fopen(argv[3], "wt");
			if (!out)
			{
				fprintf(stderr, "pngtrace: could not create output file '%s'\n", argv[2]);
				return 1;
			}
		}
	}

	printf("Extracting polygons...\n");

	load_rules();
	pixels.doTrace(out);
	if (!discard_rules)
		save_rules();

	if (out)
		fclose(out);
	printf("\nDone!\n");
	return 0;
}
